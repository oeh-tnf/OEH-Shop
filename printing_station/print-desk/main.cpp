#include <iostream>

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>

#include <fstream>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <oehshop/client.hpp>

bool running = true;
bool logged_in = false;

Fl_Window* root_window;
oehshop::Client* root_client;

static void
window_cb(Fl_Widget* widget, void*)
{
  root_client->close_session();
  root_window->fullscreen();
  logged_in = false;
}

static void
signal_handler(int s)
{
  return;
}

static void
input_cb(Fl_Widget* w, void* p)
{
  if(logged_in) return;
  Fl_Input* input = static_cast<Fl_Input*>(w);
  root_client->setUsername(input->value());
  if(root_client->open_session()) {
    root_window->fullscreen_off();
    logged_in = true;
  }
  input->value("");
}

int
main(int argc, char* argv[])
{
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = signal_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  // sigaction(SIGINT, &sigIntHandler, NULL);

  std::cout << "Starting Client Desk" << std::endl;

  if(argc != 3) {
    std::cout << "2 Arguments required: Printer Address and Desk URL"
              << std::endl;
    return 1;
  }

  oehshop::Client client;
  client.setPrinterPage(argv[1]);
  client.setDeskURL(argv[2]);
  root_client = &client;

  Fl_Window win(200, 200, "PrintingStation - Client");
  root_window = &win;
  win.begin();

  Fl_Output statusLabel(0, 0, 200, 100, "Status");
  statusLabel.textsize(50);
  statusLabel.show();

  Fl_Input idInput(0, 100, 200, 100, "ID");
  idInput.textsize(50);
  idInput.show();
  idInput.take_focus();
  idInput.callback(&input_cb, static_cast<void*>(&client));

  win.end();
  win.show();

  win.callback(window_cb);

  // TODO: Use proper evdev handling.

  win.fullscreen();

  while(running) {
    Fl::wait(0.01);
  }
}
