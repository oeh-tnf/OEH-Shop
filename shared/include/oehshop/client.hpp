#pragma once

#include <string>

namespace oehshop {
class Client
{
  public:
  Client();
  ~Client();

  void setUsername(const std::string& username) { m_username = username; }
  void setDeskURL(const std::string& deskURL) { m_deskURL = deskURL; }
  void setPrinterPage(const std::string& pp) { m_printerPage = pp; }

  bool open_session();
  bool close_session();

  private:
  std::string m_username;
  std::string m_deskURL;
  std::string m_printerPage;
};
}
