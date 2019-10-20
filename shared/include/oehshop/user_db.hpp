#pragma once

#include "printer.hpp"
#include "reply_service.hpp"
#include "user_view.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>

namespace oehshop {
class UserDB
{
  public:
  explicit UserDB(const char* dbPath, UserView* view = nullptr);
  ~UserDB();

  void setView(UserView* view) { m_userView = view; }

  void serve();
  void refreshUsers();

  void processMessage(ReplyService* replyService,
                      const std::string& type,
                      pugi::xml_node& node);

  std::pair<bool, std::string> login(const std::string& username,
                                     const std::string& hostname);
  void logout(const std::string& username,
              const std::string& hostname,
              Printer::PrinterStats stats);

  void removeUserFromDB(const std::string& username);
  void addPagesToUser(const std::string& username, Printer::PrinterStats stats);

  private:
  ReplyService m_replyService;
  SQLite::Database m_db;
  UserView* m_userView;
};
}
