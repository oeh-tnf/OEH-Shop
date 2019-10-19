#pragma once

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>

namespace oehshop {
class UserDB
{
  public:
  explicit UserDB(const char* dbPath);
  ~UserDB();

  void serve();

  private:
  SQLite::Database m_db;
};
}
