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

#include <oehshop/user.hpp>

#include <filesystem>
#include <iostream>
#include <set>

// The original idea for this code is from this tutorial:
// https://github.com/fedetask/pam-tutorials
//
// Additional resources and information from:
// http://www.linux-pam.org/Linux-PAM-html/Linux-PAM_MWG.html

const std::string printerXMLBefore{ "/root/oeh-shop-printer-xml-before.xml" };
const std::string printerXMLAfter{ "/root/oeh-shop-printer-xml-after.xml" };

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
  return disallowed_users.count(user) == 0 && user.length() >= 8;
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
  std::string skeletonDir = "/home/print";
  std::string colorXPath =
    "/html/body/div[2]/div[1]/table/tbody/tr[5]/td[2]/text()";
  std::string bwXPath =
    "/html/body/div[2]/div[1]/table/tbody/tr[2]/td[2]/text()";
  std::string loginUser = "";
};

Configuration
getConfigurationFromCLI(int argc,
                        const char** argv,
                        const std::string& username)
{
  Configuration config;
  config.loginUser = username;

  if(argc != 7) {
    std::cout
      << "WARNING: Invalid param usage! Only found " << argc
      << " params of 7. Change pam config file for this module "
         "to libprintingstation.so <PROD> <PRINTER URL> <PRINTER_COLOR_XPATH> "
         "<PRINTER_BW_XPATH> <DESK_URL> <MAP USER> <SKELETON DIR>"
      << std::endl;

    return std::move(config);
  }

  config.production = std::string(argv[0]) == "PROD";
  config.printer_url = argv[1];
  config.colorXPath = argv[2];
  config.bwXPath = argv[3];
  config.desk_url = argv[4];
  config.mapUser = argv[5];
  config.skeletonDir = argv[6];
  return std::move(config);
}

void
printConfiguration(const std::string& from, const Configuration& config)
{
  if(config.production)
    return;

  std::cout << "    [" << from << "] CONFIG: production=" << config.production
            << ", printer_url=" << config.printer_url
            << ", desk_url=" << config.desk_url
            << ", color_xpath=" << config.colorXPath
            << ", bw_xpath=" << config.bwXPath << ", mapUser=" << config.mapUser
            << ", loginUser=" << config.loginUser
            << ", skeletonDir=" << config.skeletonDir << std::endl;
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

  // Now is the time to check if the user is allowed to login. This requires the
  // remote desk.

  oehshop::User shopUser(user);
  if(!shopUser.allowedToLogIn(config.desk_url))
    return PAM_AUTH_ERR;

  // Assign user id to internal data.
  char* staticUsername =
    static_cast<char*>(malloc((user.length() + 1) * sizeof(char)));
  memcpy(staticUsername, user.c_str(), user.length() + 1);
  pam_set_data(handle,
               "oehshop-username",
               staticUsername,
               [](pam_handle_t* pamh, void* data, int error_status) {
                 char* strData = static_cast<char*>(data);
                 free(strData);
               });

  // Re-assign username, to use an existing local user instead of the account
  // ID.

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

  oehshop::Printer::downloadPage(config.printer_url, printerXMLBefore);

  // Copy from skeleton dir recursively to the mapped user directory.
  std::string sourceDir = config.skeletonDir;
  std::string targetDir = "/home/" + user;
  if(config.production) {
    std::cout << " ->> (PROD) OPERATION FROM \"" << sourceDir << "\" TO \""
              << targetDir << "\"" << std::endl;
    std::filesystem::copy(sourceDir, targetDir);

    // Run CHOWN for complete user directory.
    system((std::string("chown -R ") + config.mapUser + ":" + config.mapUser +
            " " + targetDir)
             .c_str());
  } else {
    std::cout << " ->> COPY OPERATION FROM \"" << sourceDir << "\" TO \""
              << targetDir << "\"" << std::endl;
  }

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

  std::string oehshopUsername;
  const void* oehshopUsernameCache;
  if(pam_get_data(handle, "oehshop-username", &oehshopUsernameCache) !=
     PAM_SUCCESS) {
    std::cerr << "Could not receive username from PAM!" << std::endl;
    return PAM_SESSION_ERR;
  }
  oehshopUsername = std::string(static_cast<const char*>(oehshopUsernameCache));

  oehshop::User shopUser(oehshopUsername);

  oehshop::Printer::downloadPage(config.printer_url, printerXMLAfter);
  oehshop::Printer printer(
    printerXMLBefore, printerXMLAfter, config.colorXPath, config.bwXPath);
  oehshop::Printer::PrinterStats printerStats = printer.getDiff();

  std::string dirToBeRemoved = "/home/" + user;
  if(config.production) {
    std::cout << " ->> (PROD) REMOVE ALL OPERATION FOR \"" << dirToBeRemoved
              << "\"" << std::endl;
    std::filesystem::remove_all(dirToBeRemoved);
  } else {
    std::cout << " ->> REMOVE ALL OPERATION FOR \"" << dirToBeRemoved << "\""
              << std::endl;
  }

  shopUser.logout(config.desk_url, printerStats);

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
