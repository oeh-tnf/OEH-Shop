#pragma once

#include <string>

namespace oehshop {
class Requester
{
  public:
  struct Answer
  {
    bool success = false;
    std::string message = "";
  };

  Requester(const std::string& url);
  ~Requester();

  Answer request(const std::string& msg);

  private:
  Answer fatal(const char* location, int code);

  std::string m_url;
};
}
