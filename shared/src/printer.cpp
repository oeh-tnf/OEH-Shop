#include <filesystem>
#include <iostream>
#include <oehshop/printer.hpp>
#include <pugixml.hpp>

namespace oehshop {
Printer::Printer(const std::string& path1, const std::string& path2)
{
  m_oldStats = parsePrinterXML(path1);
  m_newStats = parsePrinterXML(path2);
  m_diffStats = m_newStats - m_oldStats;
  std::cout << "B/W Pages: " << m_diffStats.bwPages
            << ", Color Pages: " << m_diffStats.clPages << std::endl;
}
Printer::~Printer() {}

bool
Printer::downloadPage(const std::string& page, const std::string& destFile)
{
  std::cout << "Download Page: " << page << " to " << destFile << std::endl;

  if(std::filesystem::exists(destFile)) {
    std::filesystem::remove(destFile);
  }

  // This is not as bad as it looks. Because this is only running in a
  // constrained environment that is tightly controlled. Shelling-Out is not too
  // bad here.

  // 1. Downloads the printer status page.
  // 2. Uses HTML-Tidy to convert it into XML
  // 3. Remove namespace declaration to ease parsing in pugixml.
  // 4. Save in destFile.

  std::string command =
    "wget --timeout=5 --tries=3 --no-check-certificate \"" + page +
    "\" -qO- | tidy -q -asxml -utf8 "
    "--quote-nbsp 0 --show-errors 0 | sed -e 's/xmlns=\".*\"//g' > \"" +
    destFile + "\"";
  system(command.c_str());

  if(std::filesystem::file_size(destFile) < 500) {
    std::cerr << "COULD NOT DOWNLOAD PRINTER PAGE!" << std::endl;
    return false;
  }
  return true;
}

Printer::PrinterStats
Printer::parsePrinterXML(const std::string& parse)
{
  PrinterStats stats;
  pugi::xml_document d;
  d.load_file(parse.c_str());
  stats.bwPages =
    d.select_node("/html/body/div[2]/div[1]/table/tbody/tr[2]/td[2]/text()")
      .node()
      .text()
      .as_int();
  stats.clPages =
    d.select_node("/html/body/div[2]/div[1]/table/tbody/tr[5]/td[2]/text()")
      .node()
      .text()
      .as_int();

  return stats;
}
}
