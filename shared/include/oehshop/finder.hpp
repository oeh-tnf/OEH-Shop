#pragma once

#include <string>

typedef struct nng_socket_s nng_socket;

namespace oehshop {
class Finder
{
  public:
  Finder();
  ~Finder();

  std::pair<std::string, bool> findDesk();

  void provideDesk();

  private:
  std::string analyzeSurveyResponse(const char* buf);
  std::string buildDeskAnswer(nng_socket* sock);
};
}
