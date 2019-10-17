#include <iostream>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>

void
but_cb(Fl_Widget* o, void*)
{
  Fl_Button* b = (Fl_Button*)o;
  b->label("Good job");// redraw not necessary

  b->resize(10, 150, 140, 30);// redraw needed
  b->redraw();
}

int
main(int argc, char* argv[])
{
  std::cout << "Starting Print Desk" << std::endl;

  Fl_Window win(300, 200, "Testing");
  win.begin();
  Fl_Button but(10, 150, 70, 30, "Click me");
  win.end();
  but.callback(but_cb);
  win.show();
  return Fl::run();
}
