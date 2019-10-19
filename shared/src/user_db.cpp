#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <iostream>
#include <oehshop/config.hpp>
#include <oehshop/user_db.hpp>
#include <pugixml.hpp>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>

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
                                "last_change DATETIME DEFAULT CURRENT_TIMESTAMP"
                                ");";

namespace oehshop {
void
fatal(const char* func, int rv)
{
  std::cerr << "FATAL: " << std::string(func) + ": " + nng_strerror(rv)
            << std::endl;
}
UserDB::UserDB(const char* dbPath)
  : m_db(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
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
  static nng_socket sock;
  static bool initialized = false;
  int rv;
  char* buf = NULL;

  std::string url = "tcp://*:" + std::to_string(OEHSHOP_USER_PORT);

  if(!initialized) {
    if((rv = nng_rep0_open(&sock)) != 0) {
      fatal("nng_rep0_open", rv);
    }
    if((rv = nng_listen(sock, url.c_str(), NULL, 0)) != 0) {
      fatal("nng_listen", rv);
    }

    initialized = true;
  }

  for(;;) {
    size_t sz = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK);
    if(sz == NNG_EAGAIN) {
      break;
    } else if(sz == 0) {
      std::string answer = "NONE";
      if((rv = nng_send(sock, (void*)answer.c_str(), answer.size() + 1, 0)) !=
         0) {
        fatal("nng_send", rv);
        break;
      }
    } else {
      fatal("nng_recv", rv);
      return;
    }
    nng_free(buf, sz);
  }
}
}
