#pragma once

#include <string>

namespace oehshop {
class Finder
{
  public:
  Finder();
  ~Finder();

  std::pair<std::string, bool> findDesk();

  void startProvidingDesk();

  /** @brief Update for asynchronously providing the desk.
   *
   * Not required for printing station.
   */
  void update();

  private:

  std::string analyzeSurveyResponse(const char *buf);
};
}
