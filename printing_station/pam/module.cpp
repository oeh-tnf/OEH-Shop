extern "C"
{
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

extern "C" int
pam_sm_authenticate(pam_handle_t* handle,
                    int flags,
                    int argc,
                    const char** argv);

extern "C" int
pam_sm_acct_mgmt(pam_handle_t* pamh, int flags, int argc, const char** argv);

extern "C" int
pam_sm_setcred(pam_handle_t* pamh, int flags, int argc, const char** argv);

extern "C" int
pam_sm_open_session(pam_handle_t* pamh, int flags, int argc, const char** argv);

extern "C" int
pam_sm_close_session(pam_handle_t* pamh,
                     int flags,
                     int argc,
                     const char** argv);

extern "C" int
pam_sm_chauthtok(pam_handle_t* pamh, int flags, int argc, const char** argv);

// THIS SOURCECODE IS BASED ON THIS TUTORIAL:
// https://github.com/fedetask/pam-tutorials

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t* handle,
                    int flags,
                    int argc,
                    const char** argv)
{
  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_acct_mgmt(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_setcred(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_open_session(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_chauthtok(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
  // Auth token cannot be changed!
  return PAM_PERM_DENIED;
}
