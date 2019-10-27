#pragma once

#include "printer.hpp"
#include <string>

namespace oehshop {
class User
{
  public:
  explicit User(const std::string &username);
  ~User();

  bool allowedToLogIn(const std::string& deskAddress);
  void logout(const std::string& deskAddress, Printer::PrinterStats stats);

  private:
  bool parseAllowanceAnswerMsg(const char* answer);

  const char* m_username;
  std::string m_hostname;
};
}
