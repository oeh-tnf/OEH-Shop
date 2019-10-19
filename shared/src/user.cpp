#include <cstring>
#include <iostream>
#include <oehshop/config.hpp>
#include <oehshop/user.hpp>
#include <pugixml.hpp>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>

namespace oehshop {

void
fatal(const char* func, int rv)
{
  std::cerr << "FATAL: " << std::string(func) + ": " + nng_strerror(rv)
            << std::endl;
}

User::User(const char* name)
  : m_username(name)
{}
User::~User() {}

bool
User::allowedToLogIn(const std::string& deskAddress)
{
  nng_socket sock;
  int rv;
  size_t sz;
  char* buf = NULL;

  std::string url = deskAddress + ":" + std::to_string(OEHSHOP_USER_PORT);

  if((rv = nng_req0_open(&sock)) != 0) {
    fatal("nng_socket", rv);
    return false;
  }
  if((rv = nng_dial(sock, url.c_str(), NULL, 0)) != 0) {
    fatal("nng_dial", rv);
    return false;
  }
  std::cout << "Asking " << url << " for login allowance..." << std::endl;

  std::string message = "<message type=\"login\"><username>";
  message += m_username;
  message += "</username></message>";

  if((rv = nng_send(sock, (void*)message.c_str(), message.size() + 1, 0)) !=
     0) {
    fatal("nng_send", rv);
    return false;
  }
  if((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
    fatal("nng_recv", rv);
    return false;
  }
  bool allowed = parseAllowanceAnswerMsg(buf);
  nng_free(buf, sz);
  nng_close(sock);
  return allowed;
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
            << "XML Document service node of type desk does not contain "
               "address node!"
            << std::endl;
          return false;
        }
      }
    } else {
      std::cout << "XML Document service node does not contain type attribute!"
                << std::endl;
      return false;
    }
  } else {
    std::cout << "XML Document does not contain service node!" << std::endl;
    return false;
  }

  std::cout << "Other Error in XML Parsing." << std::endl;

  return false;
}
}
