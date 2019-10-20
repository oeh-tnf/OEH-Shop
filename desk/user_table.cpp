#include "user_table.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include <algorithm>

#define COLS 7

#define COL_USERNAME 0
#define COL_HOSTNAME 1
#define COL_TIMESTAMP 2
#define COL_BWPAGES 3
#define COL_CLPAGES 4
#define COL_COSTS 5
#define COL_STATUS 6

const char* collumn_headers[] = { "Username", "Hostname", "Last Change", "BW",
                                  "CL",       "Cost",     "Status" };

static oehshop::UserDB* userDB = nullptr;

UserTable::UserTable(oehshop::UserDB& db, Fl_Output* o)
  : oehshop::UserView(db)
  , Fl_Table(0, 100, 800, 700, "")
  , m_statusOutput(o)
{
  userDB = &db;

  update();
  row_header(0);
  row_height_all(20);
  row_resize(0);

  cols(COLS);
  col_header(1);
  col_width_all(100);
  col_width(1, 200);
  col_width(2, 200);

  col_width(3, 40);
  col_width(4, 40);
  col_width(5, 70);
  col_width(6, 150);
  col_resize(1);

  end();
}

UserTable::~UserTable() {}

void
UserTable::DrawHeader(const char* s, int X, int Y, int W, int H)
{
  fl_push_clip(X, Y, W, H);
  fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, row_header_color());
  fl_color(FL_BLACK);
  fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
  fl_pop_clip();
}
void
UserTable::DrawData(const char* s, int X, int Y, int W, int H)
{
  fl_push_clip(X, Y, W, H);
  // Draw cell bg
  fl_color(FL_WHITE);
  fl_rectf(X, Y, W, H);
  // Draw cell data
  fl_color(FL_GRAY0);
  fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
  // Draw box border
  fl_color(color());
  fl_rect(X, Y, W, H);
  fl_pop_clip();
}
void
UserTable::draw_cell(TableContext context,
                     int ROW,
                     int COL,
                     int X,
                     int Y,
                     int W,
                     int H)
{
  User* user = nullptr;
  std::string data = "";
  if(ROW > 0) {
    if(m_users.size() < ROW) {
      return;
    }
    user = &(*(m_users.begin() + (ROW - 1)));
  }

  switch(context) {
    case CONTEXT_STARTPAGE:     // before page is drawn..
      fl_font(FL_HELVETICA, 16);// set the font for our drawing operations
      return;
    case CONTEXT_COL_HEADER:// Draw column headers
      DrawHeader(collumn_headers[COL], X, Y, W, H);
      return;
    case CONTEXT_ROW_HEADER:// Draw row headers
    case CONTEXT_CELL:      // Draw data in cells
      if(!user) {
        break;
      }
      switch(COL) {
        case COL_USERNAME:
          data = user->username;
          break;
        case COL_HOSTNAME:
          data = user->hostname;
          break;
        case COL_TIMESTAMP:
          data = user->timestamp;
          break;
        case COL_BWPAGES:
          data = std::to_string(user->bwPages);
          break;
        case COL_CLPAGES:
          data = std::to_string(user->clPages);
          break;
        case COL_COSTS:
          data = getCostsString(user->bwPages, user->clPages);
          break;
        case COL_STATUS:
          data = user->status;
          break;
      }
      DrawData(data.c_str(), X, Y, W, H);
      return;
    default:
      return;
  }
}

void
UserTable::update()
{
  rows(m_users.size() + 1);
  redraw();
}

void
UserTable::clear()
{
  m_users.clear();
  update();
}

void
UserTable::addUser(const std::string& username, const std::string& hostname)
{
  User* u = getUser(username);
  if(!u) {
    m_users.push_back(User{ username, hostname });
  } else {
    u->hostname = hostname;
  }

  update();
}

void
UserTable::updateUser(const std::string& username,
                      const std::string& timestamp,
                      const std::string& status,
                      int bwPages,
                      int clPages)
{
  User* u = getUser(username);
  if(!u) {
    return;
  } else {
    u->timestamp = timestamp;
    u->status = status;
    u->bwPages = bwPages;
    u->clPages = clPages;
  }

  update();
}

void
UserTable::removeUser(const std::string& username)
{
  m_users.erase(
    std::find_if(m_users.begin(), m_users.end(), [username](auto e) {
      return e.username == username;
    }));
  update();
}

UserTable::User*
UserTable::getUser(const std::string& username)
{
  auto it = std::find_if(m_users.begin(), m_users.end(), [username](auto e) {
    return e.username == username;
  });
  if(it == m_users.end()) {
    return nullptr;
  }
  return &(*it);
}

void
UserTable::startPayment(const std::string& username)
{
  User* u = getUser(username);
  if(!u) {
    displayStatus("User \"" + username + "\" not found!");
    return;
  }

  if(u->status == "New") {
    displayStatus("User \"" + username + "\" not logged out!");
    return;
  }

  u->status = "Payment in Progress";
  update();

  u->paymentWindow = new Fl_Window(400, 200, "Payment Window");
  u->paymentWindow->begin();

  std::string costs = "";
  costs += "BW: ";
  costs += std::to_string(u->bwPages);
  costs += ", CL: ";
  costs += std::to_string(u->clPages);
  costs += " => ";
  costs += getCostsString(u->bwPages, u->clPages);

  Fl_Output* payAmount = new Fl_Output(0, 0, 400, 100, "Cost");
  Fl_Button* payedButton = new Fl_Button(0, 100, 400, 100, "Payment");

  payAmount->value(costs.c_str());
  payAmount->textsize(22);

  payedButton->callback(
    [](Fl_Widget* w, void* p) {
      User* user = static_cast<User*>(p);
      userDB->removeUserFromDB(user->username);
    },
    static_cast<void*>(u));
  payedButton->take_focus();
  payedButton->shortcut(FL_Enter);

  u->paymentWindow->end();
  u->paymentWindow->show();
}

void
UserTable::displayStatus(const std::string& status)
{
  m_statusOutput->value(status.c_str());
}
