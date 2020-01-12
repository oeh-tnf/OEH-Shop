extern "C"
{
#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define PAM_SM_SESSION
#define PAM_SM_PASSWORD

#include <security/_pam_types.h>
#include <security/pam_ext.h>
#include <security/pam_modules.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
}

#include <filesystem>
#include <iostream>
#include <set>

// The original idea for this code is from this tutorial:
// https://github.com/fedetask/pam-tutorials
//
// Additional resources and information from:
// http://www.linux-pam.org/Linux-PAM-html/Linux-PAM_MWG.html

const std::set<std::string> disallowed_users = { "maximaximal",
                                                 "root",
                                                 "print",
                                                 "user",
                                                 "" };

/** This function returns true if the given user is allowed to login. This
 * depends on a blacklist and length of string. */
bool
isUserAllowed(const std::string& user)
{
  return disallowed_users.count(user) == 0 && user.length() > 8;
}

std::string
getUsernameFromPAM(pam_handle_t* pamh)
{
  const char* username;
  int status = pam_get_user(pamh, &username, nullptr);
  if(status != PAM_SUCCESS || username == nullptr)
    return "";
  return std::string(username);
}

struct Configuration
{
  bool production = false;
  std::string printer_url = "https://10.78.129.11/counters/usage.php";
  std::string desk_url = "tcp://10.14.21.2";
  std::string mapUser = "activeprintuser";
  std::string loginUser = "";
};

Configuration
getConfigurationFromCLI(int argc,
                        const char** argv,
                        const std::string& username)
{
  Configuration config;
  config.loginUser = username;
  switch(argc) {
    case 5:
      config.mapUser = argv[4];
    case 4:
      config.desk_url = argv[3];
    case 3:
      config.printer_url = argv[2];
    case 2:
      config.production = std::string(argv[1]) == "PROD";
  }
  return std::move(config);
}

void
printConfiguration(const std::string from, const Configuration& config)
{
  std::cout << "    [" << from << "] CONFIG: production=" << config.production
            << ", printer_url=" << config.printer_url
            << ", desk_url=" << config.desk_url
            << ", mapUser=" << config.mapUser
            << ", loginUser=" << config.loginUser << std::endl;
}

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t* handle,
                    int flags,
                    int argc,
                    const char** argv)
{
  std::string user = getUsernameFromPAM(handle);
  if(!isUserAllowed(user))
    return PAM_USER_UNKNOWN;

  Configuration config = getConfigurationFromCLI(argc, argv, user);
  printConfiguration("authenticate", config);

  pam_set_item(handle, PAM_USER, config.mapUser.c_str());

  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_acct_mgmt(pam_handle_t* handle, int flags, int argc, const char** argv)
{
  std::string user = getUsernameFromPAM(handle);
  if(!isUserAllowed(user))
    return PAM_USER_UNKNOWN;

  Configuration config = getConfigurationFromCLI(argc, argv, user);
  printConfiguration("acct_mgmt", config);

  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_setcred(pam_handle_t* handle, int flags, int argc, const char** argv)
{
  std::string user = getUsernameFromPAM(handle);
  if(!isUserAllowed(user))
    return PAM_USER_UNKNOWN;

  Configuration config = getConfigurationFromCLI(argc, argv, user);
  printConfiguration("setcred", config);

  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_open_session(pam_handle_t* handle,
                    int flags,
                    int argc,
                    const char** argv)
{
  std::string user = getUsernameFromPAM(handle);
  if(!isUserAllowed(user))
    return PAM_USER_UNKNOWN;

  Configuration config = getConfigurationFromCLI(argc, argv, user);
  printConfiguration("open_session", config);

  if(user != config.mapUser)
    return PAM_SESSION_ERR;

  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t* handle,
                     int flags,
                     int argc,
                     const char** argv)
{
  std::string user = getUsernameFromPAM(handle);
  if(!isUserAllowed(user))
    return PAM_USER_UNKNOWN;

  Configuration config = getConfigurationFromCLI(argc, argv, user);
  printConfiguration("close_session", config);

  if(user != config.mapUser)
    return PAM_SESSION_ERR;

  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_chauthtok(pam_handle_t* handle, int flags, int argc, const char** argv)
{
  std::string user = getUsernameFromPAM(handle);
  if(!isUserAllowed(user))
    return PAM_USER_UNKNOWN;

  Configuration config = getConfigurationFromCLI(argc, argv, user);
  printConfiguration("chkauthtok", config);

  return PAM_PERM_DENIED;
}
