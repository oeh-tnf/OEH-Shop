#pragma once

namespace oehshop {
class Client
{
  public:
  enum State
  {
    LOGGED_OUT,
    LOGGED_IN
  };

  Client(State state);
  ~Client();

  private:
};
}
