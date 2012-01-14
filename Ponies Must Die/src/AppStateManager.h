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

#ifndef APPSTATEMANAGER_H
#define APPSTATEMANAGER_H

#include "AppState.h"
#include <OgreWindowEventUtilities.h>
#include <OgreFrameListener.h>
#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreTimer.h>
#include <OISInputManager.h>
#include <deque>

class AppStateManager :
	public Ogre::WindowEventListener,
	public Ogre::FrameListener
{
private:
	void setupResources(void);
	void setupOIS(void);
	void cleanupOIS(void);

	std::deque<AppState *> StateStack;
	
	Ogre::Root * _OgreRoot;
	Ogre::RenderWindow * _Window;
	Ogre::Timer * _Timer;
	
	//OIS Input devices
	OIS::InputManager *    _InputManager;
	OIS::Mouse *           _Mouse;
	OIS::Keyboard *        _Keyboard;
	
	bool                   _Shutdown;
	static AppStateManager * Singleton;
		
public:
	AppStateManager(void);
	~AppStateManager();

	bool setup(void);
	void Enter(AppState * NewState);
	void SwitchTo(AppState * NewState);
	void Exit(void);
	void MainLoop(AppState * InitialState);
	static AppStateManager& GetSingleton(void)
	{
		return *Singleton;
	};
	
	Ogre::Root *         &GetOgreRoot(void)     { return _OgreRoot; };
	Ogre::RenderWindow * &GetWindow(void)       { return _Window; };
	OIS::InputManager *  &GetInputManager(void) { return _InputManager; };
	OIS::Mouse *         &GetMouse(void)        { return _Mouse; };
	OIS::Keyboard *      &GetKeyboard(void)     { return _Keyboard; };
	
protected:
	virtual void windowResized(Ogre::RenderWindow * rw);
	virtual void windowClosed(Ogre::RenderWindow* rw);
	virtual bool frameRenderingQueued(const Ogre::FrameEvent &evt);
};

#endif // APPSTATEMANAGER_H
