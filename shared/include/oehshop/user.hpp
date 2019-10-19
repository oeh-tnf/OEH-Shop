#pragma once

#include <string>

namespace oehshop {
class User
{
  public:
  explicit User(const char* name);
  ~User();

  bool allowedToLogIn(const std::string& deskAddress);

  private:
  bool parseAllowanceAnswerMsg(const char* answer);

  const char* m_username;
};
}
