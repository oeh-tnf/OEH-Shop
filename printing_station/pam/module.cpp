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

const char* SKELETON_DIR = "/home/print";
const char* USER_DIR = "/home/user";

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
  int pam_code;

  return PAM_SUCCESS;

  const char* username = NULL;

  /* Asking the application for an  username */
  pam_code = pam_get_user(handle, &username, "USERNAME: ");
  if(pam_code != PAM_SUCCESS) {
    fprintf(stderr, "Can't get username");
    return PAM_PERM_DENIED;
  }

  std::cout << "PAM user: " << username << std::endl;

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
  /* Environment variable name */
  const char* env_var_name = "USER_FULL_NAME";

  /* User full name */
  const char* name = "Printing User";

  /* String in which we write the assignment expression */
  char env_assignment[100];

  /* If application asks for establishing credentials */
  if(flags & PAM_ESTABLISH_CRED)
    /* We create the assignment USER_FULL_NAME=John Smith */
    sprintf(env_assignment, "%s=%s", env_var_name, name);
  /* If application asks to delete credentials */
  else if(flags & PAM_DELETE_CRED)
    /* We create the assignment USER_FULL_NAME, withouth equal,
     * which deletes the environment variable */
    sprintf(env_assignment, "%s", env_var_name);

  /* In this case credentials do not have an expiry date,
   * so we won't handle PAM_REINITIALIZE_CRED */

  pam_putenv(pamh, env_assignment);

  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_open_session(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
  // Copy user skeleton and init session.
  std::error_code e;

  if(std::filesystem::exists(USER_DIR)) {
    std::filesystem::remove_all(USER_DIR, e);
    if(e) {
      std::cerr << "Dir " << USER_DIR
                << " already existed. Could not delete old dir! Error: "
                << e.message();
      return PAM_PERM_DENIED;
    }
  }

  std::filesystem::copy(SKELETON_DIR, USER_DIR, e);

  if(e) {
    std::cerr << "Could not create dir " << USER_DIR << " from " << SKELETON_DIR
              << "! Error: " << e.message() << std::endl;
    return PAM_PERM_DENIED;
  }

  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
  // Get usage data, send to master and delete session.

  std::error_code e;
  std::filesystem::remove_all(USER_DIR, e);

  if(e) {
    std::cerr << "Could not remove dir " << USER_DIR
              << "! Error: " << e.message() << std::endl;
  }

  return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_chauthtok(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
  // Auth token cannot be changed!
  return PAM_PERM_DENIED;
}
