#pragma once

#include "printer.hpp"
#include "reply_service.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>

namespace oehshop {
class UserDB
{
  public:
  explicit UserDB(const char* dbPath);
  ~UserDB();

  void serve();

  void processMessage(ReplyService* replyService,
                      const std::string& type,
                      pugi::xml_node& node);

  std::pair<bool, std::string> login(const std::string& username,
                                     const std::string& hostname);
  void logout(const std::string& username,
              const std::string& hostname,
              Printer::PrinterStats stats);

  private:
  ReplyService m_replyService;
  SQLite::Database m_db;
};
}
