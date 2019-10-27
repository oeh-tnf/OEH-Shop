/* SLiM - Simple Login Manager
	Copyright (C) 1997, 1998 Per Liden
	Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
	Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "app.h"
#include "const.h"

#include <iostream>

App* LoginApp = 0;

int main(int argc, char** argv)
{
  std::cout << "PrintingLogin starting." << std::endl;
	LoginApp = new App(argc, argv);
	LoginApp->Run();
  std::cout << "PrintingLogin exit." << std::endl;
	return 0;
}
