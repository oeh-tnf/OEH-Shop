#pragma once

#include <string>

namespace oehshop {
class Printer
{
  public:
  Printer(const std::string& path1, const std::string& path2);
  ~Printer();

  static bool downloadPage(const std::string& page,
                           const std::string& destFile);

  struct PrinterStats
  {
    int32_t bwPages = -1;
    int32_t clPages = -1;

    inline PrinterStats operator-(PrinterStats o)
    {
      return { bwPages - o.bwPages, clPages - o.clPages };
    }
  };

  PrinterStats parsePrinterXML(const std::string& parse);

  PrinterStats getDiff() { return m_diffStats; };

  private:
  PrinterStats m_oldStats;
  PrinterStats m_newStats;
  PrinterStats m_diffStats;
};
}
