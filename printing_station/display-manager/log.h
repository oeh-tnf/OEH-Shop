#ifndef _LOG_H_
#define _LOG_H_

#ifdef USE_CONSOLEKIT
#include "Ck.h"
#endif
#ifdef USE_PAM
#include "PAM.h"
#endif
#include "const.h"
#include <fstream>
#include <iostream>

using namespace std;

static class LogUnit {
	ofstream logFile;
public:
	bool openLog(const char * filename);
	void closeLog();

	~LogUnit() { closeLog(); }

	template<typename Type>
	LogUnit & operator<<(const Type & text) {
    std::clog << text << std::endl;
		logFile << text; logFile.flush();
		return *this;
	}

	LogUnit & operator<<(ostream & (*fp)(ostream&)) {
    std::clog << fp << std::endl;
		logFile << fp; logFile.flush();
		return *this;
	}

	LogUnit & operator<<(ios_base & (*fp)(ios_base&)) {
    std::clog << fp << std::endl;
		logFile << fp; logFile.flush();
		return *this;
	}
} logStream;

#endif /* _LOG_H_ */
