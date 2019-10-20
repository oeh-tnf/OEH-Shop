#include <cstring>
#include <iostream>
#include <oehshop/config.hpp>
#include <oehshop/requester.hpp>
#include <oehshop/user.hpp>
#include <pugixml.hpp>

#include <limits.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>
#include <unistd.h>

namespace oehshop {

User::User(const char* name)
  : m_username(name)
{
  char hostnameCStr[HOST_NAME_MAX];
  int result = gethostname(hostnameCStr, HOST_NAME_MAX);
  if(result) {
    std::cerr << "Could not get hostname!" << std::endl;
  }
  m_hostname = hostnameCStr;
}
User::~User() {}

bool
User::allowedToLogIn(const std::string& deskAddress)
{
  std::string url = deskAddress + ":" + std::to_string(OEHSHOP_USER_PORT);
  std::cout << "Asking " << url << " for login allowance..." << std::endl;

  std::string message = "<message type=\"login\"><username>";
  message += m_username;
  message += "</username><hostname>";
  message += m_hostname;
  message += "</hostname></message>";

  Requester req(url);
  auto reply = req.request(message);

  if(reply.success) {
    return parseAllowanceAnswerMsg(reply.message.c_str());
  } else {
    return false;
  }
}

void
User::logout(const std::string& deskAddress, Printer::PrinterStats stats)
{
  std::string url = deskAddress + ":" + std::to_string(OEHSHOP_USER_PORT);
  std::cout << "Asking " << url << " for login allowance..." << std::endl;

  std::string message = "<message type=\"logout\"><username>";
  message += m_username;
  message += "</username><hostname>";
  message += m_hostname;
  message += "</hostname><bwPages>";
  message += std::to_string(stats.bwPages);
  message += "</bwPages><clPages>";
  message += std::to_string(stats.clPages);
  message += "</clPages></message>";

  Requester req(url);
  auto reply = req.request(message);

  if(!reply.success) {
    std::cerr << "Could not send logout message!" << std::endl;
  }
}

bool
User::parseAllowanceAnswerMsg(const char* msg)
{
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(msg);

  if(!result) {
    std::cerr << "XML Document parsed with errors! Document: " << msg
              << ", error: " << result.description() << " at " << result.offset
              << "." << std::endl;
    return false;
  }

  if(pugi::xml_node message = doc.child("message")) {
    if(pugi::xml_attribute type = message.attribute("type")) {
      if(std::strncmp(type.as_string(), "allowance", 9) == 0) {
        if(pugi::xml_node allow = message.child("allow")) {
          bool allowed = allow.text().as_bool();
          if(allowed) {
            return true;
          } else {
            std::cout << "Not allowed to login! Reason: "
                      << message.child("reason").text().as_string()
                      << std::endl;
            return false;
          }
        } else {
          std::cout
            << "XML Document message node of type desk does not contain "
               "address node!"
            << std::endl;
          return false;
        }
      }
    } else {
      std::cout << "XML Document message node does not contain type attribute!"
                << std::endl;
      return false;
    }
  } else {
    std::cout << "XML Document does not contain message node!" << std::endl;
    return false;
  }

  std::cout << "Other Error in XML Parsing." << std::endl;

  return false;
}
}
