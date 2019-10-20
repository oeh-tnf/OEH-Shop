#include <cstring>
#include <iostream>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>
#include <oehshop/reply_service.hpp>
#include <pugixml.hpp>

namespace oehshop {

ReplyService::ProcessingFunc
messageToXMLObject(ReplyService::MessageTypeProcessingFunc msgProcesser)
{
  ReplyService::ProcessingFunc func =
    [msgProcesser](ReplyService* s, const std::string& message) {
      pugi::xml_document doc;
      pugi::xml_parse_result result = doc.load_string(message.c_str());

      if(!result) {
        std::cerr << "XML Document parsed with errors! Document: " << message
                  << ", error: " << result.description() << " at "
                  << result.offset << "." << std::endl;
        return;
      }

      if(pugi::xml_node message = doc.child("message")) {
        if(pugi::xml_attribute type = message.attribute("type")) {
          std::string strType = type.as_string();
          msgProcesser(s, strType, message);
          return;
        } else {
          std::cout
            << "XML Document message node does not contain type attribute!"
            << std::endl;
          return;
        }
      } else {
        std::cout << "XML Document does not contain service node!" << std::endl;
        return;
      }

      std::cout << "Other Error in XML Parsing." << std::endl;

      return;
    };
  return func;
}

void
ReplyService::fatal(const char* location, int code)
{
  std::cerr << "ReplyService FATAL: "
            << std::string(location) + ": " + nng_strerror(code) << std::endl;
}

ReplyService::ReplyService(int port, ProcessingFunc func)
  : m_func(func)
  , m_sock(std::make_unique<nng_socket>())
{
  int rv;
  if((rv = nng_rep0_open(m_sock.get())) != 0) {
    fatal("nng_rep0_open", rv);
    exit(1);
  }
  if((rv = nng_listen(
        *m_sock, ("tcp://*:" + std::to_string(port)).c_str(), NULL, 0)) != 0) {
    fatal("nng_listen", rv);
    exit(1);
  }

  m_initialized = true;
}
ReplyService::ReplyService(int port, MessageTypeProcessingFunc func)
  : ReplyService(port, messageToXMLObject(func))
{}
ReplyService::~ReplyService()
{
  nng_close(*m_sock);
}

void
ReplyService::service()
{
  if(!m_initialized) {
    std::cerr << "Cannot service ReplyService that is not initialized!"
              << std::endl;
    return;
  }

  int rv = 0;
  char* buf = nullptr;
  size_t sz = 0;

  for(;;) {
    sz = 0;
    sz = nng_recv(*m_sock, &buf, &sz, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK);
    if(sz == NNG_EAGAIN) {
      break;
    } else if(sz == 0) {
      std::string msg(buf);
      try {
        m_func(this, msg);
      } catch(std::exception& e) {
        std::cerr << "Exception during message processing: " << e.what();
      }
    } else {
      fatal("nng_recv", rv);
      return;
    }
    nng_free(buf, sz);
    buf = nullptr;
  }
  if(buf != nullptr) {
    nng_free(buf, sz);
  }
}

void
ReplyService::sendAnswer(const std::string& answer)
{
  int rv;
  if((rv = nng_send(*m_sock, (void*)answer.c_str(), answer.size() + 1, 0)) !=
     0) {
    fatal("nng_send", rv);
  }
}
}
