#include <oehshop/config.hpp>
#include <oehshop/finder.hpp>

#include <chrono>
#include <cstring>
#include <iostream>
#include <nng/nng.h>
#include <nng/protocol/survey0/respond.h>
#include <nng/protocol/survey0/survey.h>
#include <pugixml.hpp>
#include <string>
#include <thread>

#define DISCOVERY_REQUEST "discover"

// The finder is generally following the example from
// https://nanomsg.org/gettingstarted/nng/survey.html

namespace oehshop {
std::pair<std::string, bool>
fatal(const char* func, int rv)
{
  std::cerr << "FATAL: " << std::string(func) + ": " + nng_strerror(rv)
            << std::endl;
  return std::make_pair(std::string(func) + ": " + nng_strerror(rv), false);
}

Finder::Finder() {}
Finder::~Finder() {}

std::pair<std::string, bool>
Finder::findDesk()
{
  nng_socket sock;
  std::string url =
    std::string("tcp://*:") + std::to_string(OEHSHOP_DISCOVERY_PORT);
  int rv;

  if((rv = nng_surveyor0_open(&sock)) != 0) {
    return fatal("nng_surveyor0_open", rv);
  }
  if((rv = nng_listen(sock, url.c_str(), NULL, 0)) != 0) {
    return fatal("nng_listen", rv);
  }
  for(;;) {
    std::cout << "SERVER: SENDING DATE SURVEY REQUEST TO " << url << std::endl;
    if((rv = nng_send(
          sock, (void*)DISCOVERY_REQUEST, strlen(DISCOVERY_REQUEST) + 1, 0)) !=
       0) {
      return fatal("nng_send", rv);
    }

    for(;;) {
      char* buf = NULL;
      size_t sz;
      rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC);
      if(rv == NNG_ETIMEDOUT) {
        std::cout << "TIMEOUT" << std::endl;
        break;
      } else if(rv == NNG_EAGAIN) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        break;
      } else if(rv != 0) {
        return fatal("nng_recv", rv);
      }
      printf("SERVER: RECEIVED \"%s\" SURVEY RESPONSE\n", buf);

      std::string analyzed = analyzeSurveyResponse(buf);
      nng_free(buf, sz);

      if(analyzed != "") {
        return { "tcp://", true };
      }
    }
  }

  return { "tcp://", false };
}

std::string
Finder::analyzeSurveyResponse(const char* buf)
{
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(buf);

  if(!result) {
    std::cerr << "XML Document parsed with errors! Document: " << buf
              << ", error: " << result.description() << " at " << result.offset
              << "." << std::endl;
    return "";
  }

  if(pugi::xml_node service = doc.child("service")) {
    if(pugi::xml_attribute type = service.attribute("type")) {
      if(type.value() == "desk") {
        if(pugi::xml_node address = doc.child("service")) {
          return address.value();
        } else {
          std::cerr
            << "XML Document service node of type desk does not contain "
               "address node!"
            << std::endl;
          return "";
        }
      }
    } else {
      std::cerr << "XML Document service node does not contain type attribute!"
                << std::endl;
      return "";
    }
  } else {
    std::cerr << "XML Document does not contain service node!" << std::endl;
    return "";
  }

  return "";
}

void
Finder::provideDesk()
{
  static bool initialized = false;
  static nng_socket sock = NNG_SOCKET_INITIALIZER;
  static std::string url =
    std::string("tcp://0.0.0.0:") + std::to_string(OEHSHOP_DISCOVERY_PORT);
  int rv;
  if(!initialized) {

    if((rv = nng_respondent0_open(&sock)) != 0) {
      fatal("nng_respondent0_open", rv);
    }
    if((rv = nng_dial(sock, url.c_str(), NULL, NNG_FLAG_NONBLOCK)) != 0) {
      fatal("nng_dial", rv);
    }
    initialized = true;
  }

  char* buf = NULL;
  size_t sz;
  std::cout << "Receiving..." << std::to_string(rand() % 100) << std::endl;
  while((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK)) ==
        0) {
    std::string request(buf);
    nng_free(buf, sz);

    if(request == DISCOVERY_REQUEST) {
      // This is a discovery request. Must provide fitting response!
      std::string answer = buildDeskAnswer(&sock);
      std::cout << "Received!" << std::endl;

      if((rv = nng_send(sock, (void*)answer.c_str(), answer.size() + 1, 0)) !=
         0) {
        fatal("nng_send", rv);
      }
    }
  }
}
std::string
Finder::buildDeskAnswer(nng_socket* sock)
{
  return "<test></test>";
}
}
