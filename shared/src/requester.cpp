#include <iostream>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <oehshop/requester.hpp>

namespace oehshop {
Requester::Requester(const std::string& url)
  : m_url(url)
{}
Requester::~Requester() {}

Requester::Answer
Requester::request(const std::string& msg)
{
  nng_socket sock;
  int rv;
  size_t sz;
  char* buf = NULL;

  if((rv = nng_req0_open(&sock)) != 0) {
    return fatal("nng_socket", rv);
  }
  if((rv = nng_dial(sock, m_url.c_str(), NULL, 0)) != 0) {
    return fatal("nng_dial", rv);
  }

  if((rv = nng_send(sock, (void*)msg.c_str(), msg.size() + 1, 0)) != 0) {
    nng_close(sock);
    return fatal("nng_send", rv);
  }
  if((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
    nng_free(buf, sz);
    nng_close(sock);
    return fatal("nng_recv", rv);
  }

  Answer answer{ true, std::string(buf) };

  nng_free(buf, sz);
  nng_close(sock);

  return answer;
}

Requester::Answer
Requester::fatal(const char* location, int code)
{
  std::cerr << "FATAL: " << std::string(location) + ": " + nng_strerror(code)
            << std::endl;
  return { false, "" };
}
}
