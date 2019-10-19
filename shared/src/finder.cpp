#include <ctime>
#include <fstream>
#include <list>
#include <oehshop/config.hpp>
#include <oehshop/finder.hpp>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <nng/nng.h>
#include <nng/protocol/pipeline0/pull.h>
#include <nng/protocol/pipeline0/push.h>
#include <pugixml.hpp>
#include <ratio>
#include <string>
#include <thread>

#define DISCOVERY_REQUEST "discover"
#define CACHE_FILE "/tmp/print_desk_address"

// The finder is generally following the example from
// https://nanomsg.org/gettingstarted/nng/pipeline.html

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
  // Check for cache.
  if(std::filesystem::exists(CACHE_FILE)) {
    auto now = std::chrono::high_resolution_clock::now();
    auto nowSinceEpoch = now.time_since_epoch().count();
    std::ifstream in(CACHE_FILE);
    std::string address;
    int64_t epoch;
    in >> epoch;
    in >> address;
    auto duration = std::chrono::nanoseconds(nowSinceEpoch - epoch);
    if(duration < std::chrono::hours(20)) {
      std::cout << "Address from cache file." << std::endl;
      return { address, true };
    } else {
      std::cout << "Cache invalidated, waiting for ping from desk."
                << std::endl;
      std::filesystem::remove(CACHE_FILE);
    }
  }

  nng_socket sock;
  std::string url =
    std::string("tcp://*:") + std::to_string(OEHSHOP_DISCOVERY_PORT);
  int rv;

  if((rv = nng_pull0_open(&sock)) != 0) {
    return fatal("nng_surveyor0_open", rv);
  }
  if((rv = nng_listen(sock, url.c_str(), NULL, 0)) != 0) {
    return fatal("nng_listen", rv);
  }
  for(;;) {
    char* buf = NULL;
    size_t sz;
    if((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
      return fatal("nng_recv", rv);
    }
    std::string remote = analyzeSurveyResponse(buf);
    if(remote != "") {
      // Remote must be cached for next login!
      std::ofstream outFile(CACHE_FILE);
      if(outFile)
        outFile << std::to_string(std::chrono::high_resolution_clock::now()
                                    .time_since_epoch()
                                    .count())
                << remote;
      else
        std::cout << "Cannot write address cache!" << std::endl;

      return { remote, true };
    } else {
      return { "Invalid Response!", false };
    }
    nng_free(buf, sz);
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
      if(std::strncmp(type.as_string(), "desk", 4) == 0) {
        if(pugi::xml_node address = service.child("address")) {
          return address.text().as_string();
        } else {
          std::cout
            << "XML Document service node of type desk does not contain "
               "address node!"
            << std::endl;
          return "";
        }
      }
    } else {
      std::cout << "XML Document service node does not contain type attribute!"
                << std::endl;
      return "";
    }
  } else {
    std::cout << "XML Document does not contain service node!" << std::endl;
    return "";
  }

  std::cout << "Other Error in XML Parsing." << std::endl;

  return "";
}

void
Finder::provideDesk(const char* address)
{
  static bool initialized = false;
  static nng_socket sock = NNG_SOCKET_INITIALIZER;
  static std::string url =
    std::string("tcp://0.0.0.0:") + std::to_string(OEHSHOP_DISCOVERY_PORT);
  static auto last_transmission = std::chrono::system_clock::now();
  int rv;
  if(!initialized) {

    if((rv = nng_push0_open(&sock)) != 0) {
      fatal("nng_push0_open", rv);
    }
    if((rv = nng_dial(sock, url.c_str(), NULL, NNG_FLAG_NONBLOCK)) != 0) {
      fatal("nng_dial", rv);
    }
    initialized = true;
  }

  auto current_system_time = std::chrono::system_clock::now();
  if(current_system_time - last_transmission >=
     std::chrono::milliseconds(10000)) {
    last_transmission = current_system_time;

    std::string answer = buildDeskAnswer(address);
    rv = nng_send(
      sock, (void*)answer.c_str(), answer.size() + 1, NNG_FLAG_NONBLOCK);
    if(rv != 0 && rv != NNG_EAGAIN) {
      fatal("nng_send", rv);
    }
  }
}
std::string
Finder::buildDeskAnswer(const char* address)
{
  return std::string("<service ") + "type=\"desk\"><address>" + address +
         "</address></service>";
}
}
