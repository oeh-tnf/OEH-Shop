/*  Based on mingetty.c taken from
 *  https://github.com/ystk/debian-mingetty/blob/master/mingetty.c
 *  and modified by Max Heisinger <mail@maximaximal.com>
 *
 *  Copyright (C) 1996 Florian La Roche  <laroche@redhat.com>
 *  Copyright (C) 2002, 2003 Red Hat, Inc
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE 1 /* Needed to get setsid() */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>

#include <linux/vt.h>
#include <sys/ioctl.h>

#include <filesystem>

#include <libevdev-1.0/libevdev/libevdev.h>

/* name of this program (argv[0]) */
static char* progname;
/* on which tty line are we sitting? (e.g. tty1) */
static char* tty;
/* what is the input device of the card reader? */
static char* hidevice = "";
/* some information about this host */
static struct utsname uts;
/* the hostname */
static char hn[MAXHOSTNAMELEN + 1];
/* process and session ID of this program */
static pid_t pid, sid;
/* login program invoked */
static char* loginprog = "/bin/login";
/* Do not send a reset string to the terminal. */
static int noclear = 0;
/* Do not print a newline. */
static int nonewline = 0;
/* Do not print /etc/issue. */
static int noissue = 0;
/* Do not call vhangup() on the tty. */
static int nohangup = 0;
/* Do not print any hostname. */
static int nohostname = 0;
/* Print the whole string of gethostname() instead of just until the next "." */
static int longhostname = 0;
/* time to wait, seconds */
static int delay = 0;
/* chroot directory */
static int priority = 0;
/* automatic login with this user */
static char* autologin = NULL;

static short vtno_of_this_process = 0;

#include <iostream>

using std::cout;
using std::endl;

std::string
getPathOfCardReader()
{
  struct libevdev* dev = NULL;
  int fd;
  int rc;

  for(auto& p : std::filesystem::directory_iterator("/dev/input/")) {
    std::string path = p.path().string();
    if(path.find("event") != std::string::npos) {
      fd = open(path.c_str(), O_RDONLY);
      rc = libevdev_new_from_fd(fd, &dev);
      if(rc < 0) {
        fprintf(
          stderr, "[ERROR] Failed to init libevdev (%s)\n", strerror(-rc));
        exit(1);
      }
      std::string name = libevdev_get_name(dev);
      if(name.find("card reader") != std::string::npos) {
        std::cout << "[INFO] Found card reader with name \"" + name +
                       "\" with path \"" + path + "\""
                  << std::endl;
        ;
        return path;
      }
    }
  }
  std::cout << "[ERROR] Could not find a fitting card reader! Attach a correct "
               "card reader or change software."
            << std::endl;
  return "";
}

/* error() - output error messages */
static void
error(const char* fmt, ...)
{
  va_list va_alist;

  va_start(va_alist, fmt);
  openlog(progname, LOG_PID, LOG_AUTH);
  vsyslog(LOG_ERR, fmt, va_alist);
  /* no need, we exit anyway: closelog (); */
  va_end(va_alist);
  sleep(5);
  exit(EXIT_FAILURE);
}

/* update_utmp() - update our utmp entry */
static void
update_utmp(void)
{
  struct utmp ut;
  struct utmp* utp;
  time_t cur_time;

  setutent();
  while((utp = getutent()))
    if(utp->ut_type == INIT_PROCESS && utp->ut_pid == pid)
      break;

  if(utp) {
    memcpy(&ut, utp, sizeof(ut));
  } else {
    /* some inits don't initialize utmp... */
    const char* x = tty;
    memset(&ut, 0, sizeof(ut));
    if(strncmp(x, "tty", 3) == 0)
      x += 3;
    if(strlen(x) > sizeof(ut.ut_id))
      x += strlen(x) - sizeof(ut.ut_id);
    strncpy(ut.ut_id, x, sizeof(ut.ut_id));
  }

  strncpy(ut.ut_user, "LOGIN", sizeof(ut.ut_user));
  strncpy(ut.ut_line, tty, sizeof(ut.ut_line));
  time(&cur_time);
  ut.ut_time = cur_time;
  ut.ut_type = LOGIN_PROCESS;
  ut.ut_pid = pid;
  ut.ut_session = sid;

  pututline(&ut);
  endutent();

  updwtmp(_PATH_WTMP, &ut);
}

/* open_tty - set up tty as standard { input, output, error } */
static void
open_tty(void)
{
  struct sigaction sa, sa_old;
  char buf[40];
  int fd;

  /* Reset permissions on the console device */
  if((strncmp(tty, "tty", 3) == 0) && (isdigit(tty[3]))) {
    strcpy(buf, "/dev/vcs");
    strcat(buf, &tty[3]);
    if(chown(buf, 0, 3) || chmod(buf, 0600))
      if(errno != ENOENT)
        error("%s: %s", buf, strerror(errno));

    strcpy(buf, "/dev/vcsa");
    strcat(buf, &tty[3]);
    if(chown(buf, 0, 3) || chmod(buf, 0600))
      if(errno != ENOENT)
        error("%s: %s", buf, strerror(errno));
  }

  /* Set up new standard input. */
  if(tty[0] == '/') {
    strncpy(buf, tty, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
  } else {
    strcpy(buf, "/dev/");
    strncat(buf, tty, sizeof(buf) - strlen(buf) - 1);
  }
  /* There is always a race between this reset and the call to
     vhangup() that s.o. can use to get access to your tty. */
  if(chown(buf, 0, 0) || chmod(buf, 0600))
    if(errno != EROFS)
      error("%s: %s", tty, strerror(errno));

  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, &sa_old);

  /* vhangup() will replace all open file descriptors in the kernel
     that point to our controlling tty by a dummy that will deny
     further reading/writing to our device. It will also reset the
     tty to sane defaults, so we don't have to modify the tty device
     for sane settings. We also get a SIGHUP/SIGCONT.
   */
  if((fd = open(buf, O_RDWR, 0)) < 0)
    error("%s: cannot open tty: %s", tty, strerror(errno));
  if(ioctl(fd, TIOCSCTTY, (void*)1) == -1)
    error("%s: no controlling tty: %s", tty, strerror(errno));
  if(!isatty(fd))
    error("%s: not a tty", tty);

  if(nohangup == 0) {
    if(vhangup())
      error("%s: vhangup() failed", tty);
    /* Get rid of the present stdout/stderr. */
    close(2);
    close(1);
    close(0);
    close(fd);
    if((fd = open(buf, O_RDWR, 0)) != 0)
      error("%s: cannot open tty: %s", tty, strerror(errno));
    if(ioctl(fd, TIOCSCTTY, (void*)1) == -1)
      error("%s: no controlling tty: %s", tty, strerror(errno));
  }
  /* Set up stdin/stdout/stderr. */
  if(dup2(fd, 0) != 0 || dup2(fd, 1) != 1 || dup2(fd, 2) != 2)
    error("%s: dup2(): %s", tty, strerror(errno));
  if(fd > 2)
    close(fd);

  /* Write a reset string to the terminal. This is very linux-specific
     and should be checked for other systems. */
  if(noclear == 0)
    write(0, "\033c", 2);

  sigaction(SIGHUP, &sa_old, NULL);

  if(isdigit(tty[3])) {
    vtno_of_this_process = tty[3] - '0';
  } else {
    std::cout << "[ERROR] COULD NOT PARSE TTY NUMBER!!" << std::endl;
  }
}

static void
output_special_char(unsigned char c)
{
  switch(c) {
    case 's':
      printf("%s", uts.sysname);
      break;
    case 'n':
      printf("%s", uts.nodename);
      break;
    case 'r':
      printf("%s", uts.release);
      break;
    case 'v':
      printf("%s", uts.version);
      break;
    case 'm':
      printf("%s", uts.machine);
      break;
    case 'o':
      printf("%s", uts.domainname);
      break;
    case 'd':
    case 't': {
      time_t cur_time;
      struct tm* tm;
#if 0
			char buff[20];

			time (&cur_time);
			tm = localtime (&cur_time);
			strftime (buff, sizeof (buff),
				c == 'd'? "%a %b %d %Y" : "%X", tm);
			fputs (buff, stdout);
			break;
#else
      time(&cur_time);
      tm = localtime(&cur_time);
      if(c == 'd') /* ISO 8601 */
        printf("%d-%02d-%02d", 1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday);
      else
        printf("%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
      break;
#endif
    }

    case 'l':
      printf("%s", tty);
      break;
    case 'u':
    case 'U': {
      int users = 0;
      struct utmp* ut;
      setutent();
      while((ut = getutent()))
        if(ut->ut_type == USER_PROCESS)
          users++;
      endutent();
      printf("%d", users);
      if(c == 'U')
        printf(" user%s", users == 1 ? "" : "s");
      break;
    }
    default:
      putchar(c);
  }
}

/* do_prompt - show login prompt, optionally preceded by /etc/issue contents */
static void
do_prompt(int showlogin)
{
  FILE* fd;
  int c;

  // clang-format off
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "ÖH-Shop Printing Station, Host " << longhostname << std::endl;
  std::cout << "  -> See https://github.com/oeh-tnf/OEH-Shop" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "   (Start by touching the card reader with your compatible NFC card / JKU Card)" << std::endl;
  std::cout << "   (Start durch Berühren des Kartenlesers mit geeigneter NFC Karte / JKU Card)" << std::endl;
  std::cout << "" << std::endl;
  // clang-format on

  fflush(stdout);
}

#define KEY_DOWN_CASE(KEY, RETURN) \
  case KEY_##KEY:                  \
    return RETURN

static char
switch_handle_key_down(const struct input_event& ev)
{
  if(ev.type == EV_KEY && ev.value == 0) {
    switch(ev.code) {
      KEY_DOWN_CASE(A, 'A');
      KEY_DOWN_CASE(B, 'B');
      KEY_DOWN_CASE(C, 'C');
      KEY_DOWN_CASE(D, 'D');
      KEY_DOWN_CASE(E, 'E');
      KEY_DOWN_CASE(F, 'F');
      KEY_DOWN_CASE(G, 'G');
      KEY_DOWN_CASE(H, 'H');
      KEY_DOWN_CASE(I, 'I');
      KEY_DOWN_CASE(J, 'J');
      KEY_DOWN_CASE(K, 'K');
      KEY_DOWN_CASE(L, 'L');
      KEY_DOWN_CASE(M, 'M');
      KEY_DOWN_CASE(N, 'N');
      KEY_DOWN_CASE(O, 'O');
      KEY_DOWN_CASE(P, 'P');
      KEY_DOWN_CASE(Q, 'Q');
      KEY_DOWN_CASE(R, 'R');
      KEY_DOWN_CASE(S, 'S');
      KEY_DOWN_CASE(T, 'T');
      KEY_DOWN_CASE(U, 'U');
      KEY_DOWN_CASE(V, 'V');
      KEY_DOWN_CASE(W, 'W');
      KEY_DOWN_CASE(X, 'X');
      KEY_DOWN_CASE(Y, 'Y');
      KEY_DOWN_CASE(Z, 'Z');

      KEY_DOWN_CASE(KP0, '0');
      KEY_DOWN_CASE(KP1, '1');
      KEY_DOWN_CASE(KP2, '2');
      KEY_DOWN_CASE(KP3, '3');
      KEY_DOWN_CASE(KP4, '4');
      KEY_DOWN_CASE(KP5, '5');
      KEY_DOWN_CASE(KP6, '6');
      KEY_DOWN_CASE(KP7, '7');
      KEY_DOWN_CASE(KP8, '8');
      KEY_DOWN_CASE(KP9, '9');

      KEY_DOWN_CASE(0, '0');
      KEY_DOWN_CASE(1, '1');
      KEY_DOWN_CASE(2, '2');
      KEY_DOWN_CASE(3, '3');
      KEY_DOWN_CASE(4, '4');
      KEY_DOWN_CASE(5, '5');
      KEY_DOWN_CASE(6, '6');
      KEY_DOWN_CASE(7, '7');
      KEY_DOWN_CASE(8, '8');
      KEY_DOWN_CASE(9, '9');

      KEY_DOWN_CASE(ENTER, '\n');
    }
  }
  return ' ';
}

static short
get_current_vtno()
{
  FILE* fp;
  char out[1035];
  short vtno = 0;

  /* Open the command for reading. */
  fp = popen("/usr/bin/fgconsole", "r");
  if(fp == NULL) {
    std::cout << "[ERROR] Could not run fgconsole command!" << std::endl;
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while(fgets(out, sizeof(out), fp) != NULL) {
    if(!isdigit(out[0])) {
      std::cout << "[ERROR] Returned value of fgconsole is not a digit!"
                << std::endl;
      exit(1);
    }
    vtno = out[0] - '0';
  }

  /* close */
  pclose(fp);

  return vtno;
}

static int
read_from_cardreader(unsigned char* c, struct libevdev* evDevice)
{
  int fd;
  int rc = 1;
  do {
    struct input_event ev;
    rc = libevdev_next_event(evDevice, LIBEVDEV_READ_FLAG_NORMAL, &ev);
    if(rc == 0) {
      if(ev.type == EV_KEY && ev.value == 0 &&
         get_current_vtno() == vtno_of_this_process) {
        char inp = switch_handle_key_down(ev);
        if(inp != ' ') {
          *c = inp;
          return 1;
        }
      }
    }
  } while(rc == 1 || rc == 0 || rc == -EAGAIN);
}

static char*
get_logname(struct libevdev* evDevice)
{
  static char logname[40];
  char* bp;
  unsigned char c;

  tcflush(0, TCIFLUSH); /* flush pending input */
  for(*logname = 0; *logname == 0;) {
    do_prompt(1);
    for(bp = logname;;) {
      if(read_from_cardreader(&c, evDevice) < 1) {
        if(errno == EINTR || errno == EIO || errno == ENOENT)
          exit(EXIT_SUCCESS);
        error("%s: read: %s", tty, strerror(errno));
      }
      if(c == '\n' || c == '\r') {
        *bp = 0;
        break;
      } else if(!isprint(c))
        error("%s: invalid character 0x%x in login"
              " name",
              tty,
              c);
      else if((size_t)(bp - logname) >= sizeof(logname) - 1)
        error("%s: too long login name", tty);
      else
        *bp++ = c;
    }
  }
  return logname;
}

int
main(int argc, char** argv)
{
  char *logname, *s;
  int c;

  progname = argv[0];
  if(!progname)
    progname = "cardlogin";
  uname(&uts);
  gethostname(hn, MAXHOSTNAMELEN);
  hn[MAXHOSTNAMELEN] = '\0';
  pid = getpid();
  sid = getsid(0);
#if defined(s390) || defined(__s390__)
  putenv("TERM=dumb");
#else
  putenv("TERM=linux");
#endif

  if(longhostname == 0 && (s = strchr(hn, '.')))
    *s = '\0';
  tty = argv[optind];

  if(tty) {
    if(strncmp(tty, "/dev/", 5) == 0) /* ignore leading "/dev/" */
      tty += 5;
    update_utmp();
    if(delay)
      sleep(delay);
    open_tty();
  } else {
    // No tty given, this means the program is called via shell directly. Only
    // usable during testing.
    std::cout << "[WARN] No tty given to cardlogin! This can only be used "
                 "during testing."
              << std::endl;
  }

  // Find cardreader.
  std::string cardreaderDevice = getPathOfCardReader();
  struct libevdev* dev = NULL;
  int fd;
  int rc = 1;
  fd = open(cardreaderDevice.c_str(), O_RDONLY | O_NONBLOCK);
  rc = libevdev_new_from_fd(fd, &dev);

  if(rc < 0) {
    fprintf(stderr, "[ERROR] Failed to init libevdev (%s)\n", strerror(-rc));
    exit(1);
  }

  while((logname = get_logname(dev)) == 0)
    /* do nothing */;

  std::cout << "Loginname: " << logname << std::endl;

  execl(loginprog, loginprog, "--", logname, NULL);
  error("%s: can't exec %s: %s", tty, loginprog, strerror(errno));
  sleep(5);
  exit(EXIT_FAILURE);
}
