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
                               "last_change DATETIME DEFAULT CURRENT_TIMESTAMP,"
                               "bwPages INTEGER DEFAULT 0,"
                               "clPages INTEGER DEFAULT 0"
                               ");";

const char* createTablePrints = "CREATE TABLE prints ("
                                "id INTEGER PRIMARY KEY,"
                                "hostname TEXT,"
                                "bwPages INTEGER,"
                                "clPages INTEGER,"
                                "last_change DATETIME DEFAULT CURRENT_TIMESTAMP"
                                ");";

const char* userInDB = "SELECT * FROM users WHERE username = ?;";
const char* insertUserIntoDB =
  "INSERT INTO users (username, hostname) VALUES (?, ?);";

const char* deleteUserFromDB = "DELETE FROM users WHERE username = ?;";
const char* logPrintData =
  "INSERT INTO prints (bwPages, clPages, hostname) VALUES (?, ?, ?);";

const char* userAddPages = "UPDATE users SET bwPages = bwPages + ?, clPages = "
                           "clPages + ? WHERE username = ?;";

const char* allUsersInDB = "SELECT * FROM users;";

namespace oehshop {
void
fatal(const char* func, int rv)
{
  std::cerr << "FATAL: " << std::string(func) + ": " + nng_strerror(rv)
            << std::endl;
}

UserDB::UserDB(const char* dbPath, UserView* view)
  : m_replyService(OEHSHOP_USER_PORT,
                   (ReplyService::MessageTypeProcessingFunc)
                     std::bind(&UserDB::processMessage, this, _1, _2, _3))
  , m_db(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
  , m_userView(view)
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

  if(m_userView) {
    m_userView->addUser(username, hostname);
  }

  return { true, "" };
}

void
UserDB::refreshUsers()
{
  if(m_userView) {
    m_userView->clear();
  }

  std::string username;
  std::string hostname;
  std::string timestamp;
  int bwPages;
  int clPages;
  SQLite::Statement queryStmt(m_db, allUsersInDB);
  // Should have exactly one result.
  while(queryStmt.executeStep()) {
    username = queryStmt.getColumn(1).getString();
    hostname = queryStmt.getColumn(2).getString();
    timestamp = queryStmt.getColumn(3).getString();
    bwPages = queryStmt.getColumn(4).getInt();
    clPages = queryStmt.getColumn(5).getInt();

    if(m_userView) {
      m_userView->addUser(username, hostname);
      m_userView->updateUser(username, timestamp, "Loaded", bwPages, clPages);
    }
  }
}

void
UserDB::removeUserFromDB(const std::string& username)
{
  std::string hostname = "";
  int bwPages = 0;
  int clPages = 0;

  {
    SQLite::Statement queryStmt(m_db, userInDB);
    queryStmt.bind(1, username);
    // Should have exactly one result.
    while(queryStmt.executeStep()) {
      hostname = queryStmt.getColumn(2).getString();
      bwPages = queryStmt.getColumn(4).getInt();
      clPages = queryStmt.getColumn(5).getInt();
    }
  }

  SQLite::Statement logoutStmt(m_db, deleteUserFromDB);
  logoutStmt.bind(1, username);
  int res = logoutStmt.exec();
  if(res != 1) {
    std::cerr << "Could not logout user " << username << "!" << std::endl;
  } else {
    std::cout << "User successfully logged out and deleted! Page count: bw: "
              << bwPages << ", cl: " << clPages << std::endl;
    if(m_userView) {
      m_userView->removeUser(username);
    }
  }

  {
    SQLite::Statement logStmt(m_db, logPrintData);
    logStmt.bind(1, bwPages);
    logStmt.bind(2, clPages);
    logStmt.bind(3, hostname);
    logStmt.exec();
  }
}

void
UserDB::addPagesToUser(const std::string& username, Printer::PrinterStats stats)
{
  SQLite::Statement stmt(m_db, userAddPages);
  stmt.bind(1, stats.bwPages);
  stmt.bind(2, stats.clPages);
  stmt.bind(3, username);
  stmt.exec();
}

void
UserDB::logout(const std::string& username,
               const std::string& hostname,
               Printer::PrinterStats stats)
{
  addPagesToUser(username, stats);

  if(m_userView) {
    std::string timestamp = "";
    int bwPages = 0;
    int clPages = 0;
    SQLite::Statement queryStmt(m_db, userInDB);
    queryStmt.bind(1, username);
    // Should have exactly one result.
    while(queryStmt.executeStep()) {
      timestamp = queryStmt.getColumn(3).getString();
      bwPages = queryStmt.getColumn(4).getInt();
      clPages = queryStmt.getColumn(5).getInt();
    }
    m_userView->updateUser(username, timestamp, "Logged Out", bwPages, clPages);
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
