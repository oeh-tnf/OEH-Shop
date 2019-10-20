#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <iostream>
#include <oehshop/config.hpp>
#include <oehshop/user_db.hpp>
#include <pugixml.hpp>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>

using namespace std::placeholders;

const char* createTableUsers = "CREATE TABLE users ("
                               "id INTEGER PRIMARY KEY,"
                               "username TEXT,"
                               "hostname TEXT,"
                               "last_change DATETIME DEFAULT CURRENT_TIMESTAMP"
                               ");";

const char* createTablePrints = "CREATE TABLE prints ("
                                "id INTEGER PRIMARY KEY,"
                                "bwPages INTEGER,"
                                "colorPages INTEGER,"
                                "hostname TEXT,"
                                "last_change DATETIME DEFAULT CURRENT_TIMESTAMP"
                                ");";

const char* userInDB = "SELECT * FROM users WHERE username = ?;";
const char* insertUserIntoDB =
  "INSERT INTO users (username, hostname) VALUES (?, ?);";

const char* deleteUserFromDB = "DELETE FROM users WHERE username = ?;";
const char* logPrintData =
  "INSERT INTO prints (bwPages, colorPages, hostname) VALUES (?, ?, ?);";

namespace oehshop {
void
fatal(const char* func, int rv)
{
  std::cerr << "FATAL: " << std::string(func) + ": " + nng_strerror(rv)
            << std::endl;
}

UserDB::UserDB(const char* dbPath)
  : m_replyService(OEHSHOP_USER_PORT,
                   (ReplyService::MessageTypeProcessingFunc)
                     std::bind(&UserDB::processMessage, this, _1, _2, _3))
  , m_db(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
  // Initialise tables.
  if(!m_db.tableExists("users")) {
    m_db.exec(createTableUsers);
  }
  if(!m_db.tableExists("prints")) {
    m_db.exec(createTablePrints);
  }
}
UserDB::~UserDB() {}

void
UserDB::serve()
{
  m_replyService.service();
}

std::pair<bool, std::string>
UserDB::login(const std::string& username, const std::string& hostname)
{
  // Check if the user is already in the database.
  {
    SQLite::Statement queryStmt(m_db, userInDB);
    queryStmt.bind(1, username);
    while(queryStmt.executeStep()) {
      return { false,
               "User already in DB, not logged out! Hostname: " +
                 queryStmt.getColumn(2).getString() +
                 ", time: " + queryStmt.getColumn(3).getString() };
    }
  }

  {
    SQLite::Statement insertStmt(m_db, insertUserIntoDB);
    insertStmt.bind(1, username);
    insertStmt.bind(2, hostname);
    int res = insertStmt.exec();
    if(res != 1) {
      std::cerr << "Could not insert user into DB!" << std::endl;
      return { false, "Could not insert user into DB!" };
    }
  }

  return { true, "" };
}
void
UserDB::logout(const std::string& username,
               const std::string& hostname,
               Printer::PrinterStats stats)
{
  {
    SQLite::Statement logoutStmt(m_db, deleteUserFromDB);
    logoutStmt.bind(1, username);
    int res = logoutStmt.exec();
    if(res != 1) {
      std::cerr << "Could not logout user " << username << " from host "
                << hostname << "!" << std::endl;
    }
  }

  {
    SQLite::Statement logStmt(m_db, logPrintData);
    logStmt.bind(1, stats.bwPages);
    logStmt.bind(2, stats.clPages);
    logStmt.bind(3, hostname);
  }
}

void
UserDB::processMessage(ReplyService* replyService,
                       const std::string& type,
                       pugi::xml_node& node)
{
  std::string answer = "";
  if(type == "login") {
    std::string username = node.child("username").text().as_string();
    std::string hostname = node.child("hostname").text().as_string();
    auto [allowed, reason] = login(username, hostname);
    answer = "<message type=\"allowance\"><allow>";
    answer += std::to_string(allowed);
    answer += "</allow><reason>";
    answer += reason;
    answer += "</reason></message>";
    replyService->sendAnswer(answer);
  } else if(type == "logout") {
    std::string username = node.child("username").text().as_string();
    std::string hostname = node.child("hostname").text().as_string();
    Printer::PrinterStats stats;
    stats.bwPages = node.child("bwPages").text().as_int();
    stats.clPages = node.child("clPages").text().as_int();
    logout(username, hostname, stats);
    answer = "<message type=\"logout-answer\"></message>";
    replyService->sendAnswer(answer);
  } else {
    std::cerr << "[UserDB] Unknown message type: " << type << std::endl;
  }
}
}
