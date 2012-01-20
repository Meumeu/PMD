/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011-2012  Guillaume Meunier <guillaume.meunier@centraliens.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pmd.h"
#include "Game.h"
#include "AppStateManager.h"

#include <boost/filesystem.hpp>
#include <stdexcept>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#include <shlobj.h>

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char *argv[])
#endif
{
	AppStateManager manager;

	try
	{
#ifdef _WINDOWS
		char buf[MAX_PATH];
		if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf)))
			throw std::runtime_error("SHGetFolderPath failed");
		std::string HomeDir = buf;
		HomeDir += "\\Ponies Must Die\\";
#else
		std::string HomeDir = getenv("HOME");
		HomeDir += "/.PoniesMustDie/";
#endif
	
		boost::filesystem::create_directories(HomeDir);
		
		if (!manager.setup(HomeDir))
			return 1;

		manager.MainLoop(new Game);
	}
	catch(std::exception& e)
	{
		if (manager.GetWindow())
		{
			manager.GetWindow()->destroy();
			manager.GetWindow() = 0;
		}
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		MessageBoxA(NULL, e.what(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
		std::cerr << "An exception has occurred: " << e.what() << std::endl;
#endif
	}


	return 0;
}
