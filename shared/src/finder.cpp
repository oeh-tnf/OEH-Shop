#include <oehshop/config.hpp>
#include <oehshop/finder.hpp>

#include <cstring>
#include <iostream>
#include <nng/nng.h>
#include <nng/protocol/survey0/respond.h>
#include <nng/protocol/survey0/survey.h>
#include <pugixml.hpp>

#define DISCOVERY_REQUEST "discover"

namespace oehshop {
std::pair<std::string, bool>
fatal(const char* func, int rv)
{
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
        break;
      }
      if(rv != 0) {
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
}
