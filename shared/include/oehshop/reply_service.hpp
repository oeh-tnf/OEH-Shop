#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>

typedef struct nng_socket_s nng_socket;

namespace pugi {
typedef struct xml_node xml_node;
}

namespace oehshop {
class ReplyService
{
  public:
  using ProcessingFunc = std::function<void(ReplyService*, const std::string&)>;
  using MessageTypeProcessingFunc =
    std::function<void(ReplyService*, const std::string&, pugi::xml_node&)>;

  ReplyService(int port, ProcessingFunc func);
  ReplyService(int port, MessageTypeProcessingFunc func);
  ~ReplyService();

  void service();

  void sendAnswer(const std::string& answer);
  void fatal(const char* location, int code);

  private:
  std::unique_ptr<nng_socket> m_sock;
  bool m_initialized = false;
  ProcessingFunc m_func;
};
}
