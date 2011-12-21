/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Guillaume Meunier <guillaume.meunier@centraliens.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GAME_H
#define GAME_H


#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>
#include <OgreRenderWindow.h>
#include <OgreRoot.h>
#include <OISInputManager.h>
#include <OISKeyboard.h>
#include <OISMouse.h>


namespace pmd
{
	class Game :
		public Ogre::FrameListener,
		public Ogre::WindowEventListener,
		public OIS::KeyListener,
		public OIS::MouseListener
	{
	public:
		Game(void);
		~Game(void);
		
		void go(void);
	protected:
		// Ogre::FrameListener
		virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);

		// OIS::KeyListener
		virtual bool keyPressed(const OIS::KeyEvent &arg);
		virtual bool keyReleased(const OIS::KeyEvent &arg);
		// OIS::MouseListener
		virtual bool mouseMoved(const OIS::MouseEvent &arg);
		virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
		virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

		virtual void windowResized(Ogre::RenderWindow * rw);
		virtual void windowClosed(Ogre::RenderWindow* rw);

	private:
		bool setup(void);
		void cleanup(void);
		void setupResources(void);
		void setupFrameListener(void);

		Ogre::Root *           _Root;
		Ogre::Camera *         _Camera;
		Ogre::SceneManager *   _SceneMgr;
		Ogre::RenderWindow *   _Window;
		Ogre::Viewport *       _Viewport;

		//OIS Input devices
		OIS::InputManager *    _InputManager;
		OIS::Mouse *           _Mouse;
		OIS::Keyboard *        _Keyboard;

		Ogre::String           _ResourcesCfg;
		Ogre::String           _PluginsCfg;
		Ogre::String           _OgreCfg;
		Ogre::String           _OgreLog;

		bool                   _Shutdown;

		Ogre::SceneNode *      _Player;
		Ogre::Radian           _Heading;
		Ogre::Radian           _Pitch;
	};
}

#endif // GAME_H
