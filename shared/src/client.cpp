#include <filesystem>
#include <iostream>
#include <oehshop/client.hpp>
#include <oehshop/printer.hpp>
#include <oehshop/user.hpp>

namespace oehshop {

const char* SKELETON_DIR = "/home/print";
const char* USER_DIR = "/home/user";

const char* printerXMLBefore = "/home/print/printer-status-before.xml";
const char* printerXMLAfter = "/home/print/printer-status-after.xml";

Client::Client() {}

Client::~Client() {}

bool
Client::open_session()
{
  if(m_username == "maximaximal" || m_username == "" || m_username == "root" ||
     m_username == "print")
    return false;

  oehshop::User user(m_username);
  bool allowedToLogIn = user.allowedToLogIn(m_deskURL);
  if(allowedToLogIn) {
    std::cout << "User is allowed to log in!" << std::endl;
  } else {
    std::cerr << "User is NOT allowed to log in!" << std::endl;
    return false;
  }

  oehshop::Printer::downloadPage(m_printerPage, printerXMLBefore);

  std::string userDir = "/home/generic";

  // Copy user skeleton and init session.
  std::error_code e;

  if(std::filesystem::exists(userDir)) {
    std::cout << "DELETING \"" << userDir << "\"!" << std::endl;
    //std::filesystem::remove_all(userDir, e);
    if(e) {
      std::cerr << "Dir " << USER_DIR
                << " already existed. Could not delete old dir! Error: "
                << e.message();
      return false;
    }
  }

  //std::filesystem::copy(SKELETON_DIR, userDir, e);

  if(e) {
    std::cerr << "Could not create dir " << userDir << " from " << SKELETON_DIR
              << "! Error: " << e.message() << std::endl;
    return false;
  }

  return true;
}
bool
Client::close_session()
{
  oehshop::Printer::downloadPage(m_printerPage, printerXMLAfter);
  oehshop::Printer printer(printerXMLBefore, printerXMLAfter);

  // Get usage data, send to master and delete session.
  std::error_code e;
  std::string userDir = "/home/generic";
  std::cout << "REMOVING: " << userDir << std::endl;
  //std::filesystem::remove_all(userDir, e);

  if(e) {
    std::cerr << "Could not remove dir " << userDir
              << "! Error: " << e.message() << std::endl;
  }

  oehshop::User user(m_username);
  user.logout(m_deskURL, printer.getDiff());

  return true;
}
}
