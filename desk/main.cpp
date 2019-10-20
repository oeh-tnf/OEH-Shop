#include <iostream>

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>

#include <oehshop/finder.hpp>
#include <oehshop/user_db.hpp>

#include "user_table.hpp"

bool running = true;

static void
window_cb(Fl_Widget* widget, void*)
{
  Fl_Window* window = (Fl_Window*)widget;
  switch(fl_choice("Really close printer workdesk? No more prints can be done!",
                   "Yes, Close!",
                   "No, Do not close.",
                   0)) {
    case 0:
      running = false;
      break;
    default:
      break;
  }
}

int
main(int argc, char* argv[])
{
  std::cout << "Starting Print Desk" << std::endl;

  if(argc != 3) {
    std::cout
      << "2 Arguments required: Reachable Address of this computer and DB path."
      << std::endl;
    return 1;
  }

  oehshop::UserDB userDB(argv[2]);

  const int windowWidth = 800;
  const int windowHeight = 800;

  Fl_Window win(windowWidth, windowHeight, "PrintingStation - Desk");
  win.begin();

  Fl_Output statusLabel(0, 50, 800, 50, "Status");
  statusLabel.textsize(50);
  statusLabel.show();

  UserTable userTable(userDB, &statusLabel);
  userDB.setView(&userTable);
  userDB.refreshUsers();

  Fl_Input userInput(0, 0, 800, 50, "User Input");
  userInput.textsize(50);
  userInput.take_focus();
  userInput.callback(
    [](Fl_Widget* w, void* p) {
      Fl_Input* input = static_cast<Fl_Input*>(w);
      UserTable* tbl = static_cast<UserTable*>(p);
      tbl->startPayment(input->value());
    },
    static_cast<void*>(&userTable));
  userInput.when(FL_WHEN_ENTER_KEY_ALWAYS);
  userInput.show();

  win.end();
  win.show();

  win.callback(window_cb);
  win.resizable(userTable);

  oehshop::Finder finder;
  finder.provideDesk(argv[1]);

  while(running) {
    Fl::wait(0.01);
    finder.provideDesk(argv[1]);
    userDB.serve();
  }
}
