#pragma once

#include <string>

namespace oehshop {
class UserDB;

class UserView
{
  public:
  explicit UserView(UserDB& userDB)
    : m_userDB(userDB)
  {}
  ~UserView() {}

  virtual void clear() {}

  virtual void addUser(const std::string& username, const std::string& hostname)
  {}

  virtual void updateUser(const std::string& username,
                          const std::string& timestamp,
                          const std::string& status,
                          int bwPages,
                          int clPages)
  {}

  virtual void removeUser(const std::string& username) {}

  private:
  UserDB& m_userDB;
};
}
