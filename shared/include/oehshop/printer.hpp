#pragma once

#include <string>

namespace oehshop {
class Printer
{
  public:
  Printer(const std::string& path1,
          const std::string& path2,
          const std::string& colorXPath =
            "/html/body/div[2]/div[1]/table/tbody/tr[5]/td[2]/text()",
          const std::string& bwXPath =
            "/html/body/div[2]/div[1]/table/tbody/tr[2]/td[2]/text()");
  ~Printer();

  static bool downloadPage(const std::string& page,
                           const std::string& destFile);

  struct PrinterStats
  {
    PrinterStats() {}
    PrinterStats(int32_t bw, int32_t cl)
      : bwPages(bw)
      , clPages(cl)
    {}
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

  std::string m_colorXPath;
  std::string m_bwXPath;
};
}
