/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  Guillaume Meunier <guillaume.meunier@centraliens.net>

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


#ifndef MAINMENU_H
#define MAINMENU_H

#include <CEGUIInputEvent.h>

#include "AppState.h"

namespace Ogre
{
	class Root;
	class Camera;
	class SceneManager;
	class RenderWindow;
	class Viewport;
}

namespace CEGUI
{
	class OgreRenderer;
}

class MainMenu : public AppState
{
public:
	MainMenu(void);
	virtual ~MainMenu();
	virtual void Enter(void);
	virtual void Exit(void);
	virtual void Pause(void);
	virtual void Resume(void);
	virtual void Update(float dt);
	virtual bool keyPressed(const OIS::KeyEvent&);
	virtual bool keyReleased(const OIS::KeyEvent&);
	virtual bool mouseMoved(const OIS::MouseEvent&);
	virtual bool mousePressed(const OIS::MouseEvent&, OIS::MouseButtonID);
	virtual bool mouseReleased(const OIS::MouseEvent&, OIS::MouseButtonID);
	
private:
	Ogre::Root *           _Root;
	Ogre::Camera *         _Camera;
	Ogre::SceneManager *   _SceneMgr;
 	Ogre::RenderWindow *   _Window;
	Ogre::Viewport *       _Viewport;
	
	CEGUI::OgreRenderer *  _Renderer;
	
	static CEGUI::MouseButton convertButton(OIS::MouseButtonID buttonID);
	
	bool _Shutdown;
	
	bool Quit(const CEGUI::EventArgs &e)
	{
		_Shutdown = true;
		return true;
	}
	
	bool Options(const CEGUI::EventArgs &e);
	bool StartGame(const CEGUI::EventArgs &e);
};

#endif // MAINMENU_H
