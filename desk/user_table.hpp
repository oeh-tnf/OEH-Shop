#pragma once

#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Window.H>
#include <memory>
#include <oehshop/user_db.hpp>
#include <oehshop/user_view.hpp>
#include <vector>

// Table is following this example:
// https://github.com/IngwiePhoenix/FLTK/blob/master/examples/table-simple.cxx

class UserTable
  : public Fl_Table
  , public oehshop::UserView
{
  public:
  explicit UserTable(oehshop::UserDB& db, Fl_Output* o);
  ~UserTable();

  void DrawHeader(const char* s, int X, int Y, int W, int H);
  void DrawData(const char* s, int X, int Y, int W, int H);
  void draw_cell(TableContext context,
                 int ROW = 0,
                 int COL = 0,
                 int X = 0,
                 int Y = 0,
                 int W = 0,
                 int H = 0);

  struct User
  {
    ~User()
    {
      if(paymentWindow) {
        Fl::delete_widget(paymentWindow);
      }
    }

    std::string username;
    std::string hostname;
    std::string timestamp;
    int bwPages;
    int clPages;
    std::string status = "New";

    Fl_Window* paymentWindow = nullptr;
  };

  void update();

  virtual void clear();

  virtual void addUser(const std::string& username,
                       const std::string& hostname);

  virtual void updateUser(const std::string& username,
                          const std::string& timestamp,
                          const std::string& status,
                          int bwPages,
                          int clPages);

  virtual void removeUser(const std::string& username);

  User* getUser(const std::string& username);

  int getCostsInCents(int bwPages, int clPages)
  {
    return bwPages * 6 + clPages * 20;
  };
  std::string getCostsString(int bwPages, int clPages)
  {
    std::string costs = "0" + std::to_string(getCostsInCents(bwPages, clPages));
    costs.insert(costs.length() - 2, ".");
    costs += "€";
    return costs;
  }

  void startPayment(const std::string& username);

  void displayStatus(const std::string& status);

  private:
  std::vector<User> m_users;
  Fl_Output* m_statusOutput;
};
