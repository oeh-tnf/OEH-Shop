#include <iostream>

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>

#include <oehshop/finder.hpp>
#include <oehshop/user_db.hpp>

bool running = true;

void
but_cb(Fl_Widget* o, void*)
{
  Fl_Button* b = (Fl_Button*)o;
  b->label("Good job");// redraw not necessary

  b->resize(10, 150, 140, 30);// redraw needed
  b->redraw();
}

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

  Fl_Window win(300, 200, "Testing");
  win.begin();
  Fl_Button but(10, 150, 70, 30, "Click me");
  win.end();
  but.callback(but_cb);
  win.show();

  win.callback(window_cb);

  oehshop::Finder finder;
  finder.provideDesk(argv[1]);

  while(running) {
    Fl::wait(0.01);
    finder.provideDesk(argv[1]);
    userDB.serve();
  }
}
