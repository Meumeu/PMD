/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Guillaume Meunier <guillaume.meunier@centraliens.net>

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

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char *argv[])
#endif
{
	// Create application object
	//pmd::Game app;
	pmd::AppStateManager manager;

	try
	{
		try
		{
			if (!manager.setup())
				return 1;

			manager.MainLoop(new pmd::Game);
		} catch(Ogre::Exception& e)
		{
			throw std::exception(e.getFullDescription().c_str());
		}
	} catch(std::exception& e) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		if (manager.GetWindow())
		{
			manager.GetWindow()->destroy();
			manager.GetWindow() = 0;
		}
		MessageBoxA(NULL, e.what(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
		std::cerr << "An exception has occurred: " << e.what() << std::endl;
#endif
	}


	return 0;
}
