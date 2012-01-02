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

#include <OgreConfigFile.h>
#include "AppStateManager.h"
#include <assert.h>

namespace pmd
{
AppStateManager * AppStateManager::Singleton;
	
AppStateManager::AppStateManager()
{
	Singleton = this;
}

bool AppStateManager::setup(void)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	_OgreRoot = new Ogre::Root("/etc/OGRE/plugins.cfg", "../etc/ogre.cfg", "../ogre.log");
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	_OgreRoot = new Ogre::Root("../plugins.cfg", "../etc/ogre.cfg", "../ogre.log");
#endif
	
	if (!_OgreRoot->restoreConfig() && !_OgreRoot->showConfigDialog())
	{
		return false;
	}
	
	_Window = _OgreRoot->initialise(true, "Ponies Must Die");
	_Timer = _OgreRoot->getTimer();
	
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

	setupResources();
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	
	setupOIS();
	setupWindowEventListener();

	return true;
}

void AppStateManager::setupResources(void)
{
	// Load resource paths from config file
	Ogre::ConfigFile cf;
	cf.load("../etc/resources.cfg");

	// Go through all sections & settings in the file
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
				archName, typeName, secName);
		}
	}
}

void AppStateManager::setupOIS(void)
{
	OIS::ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;

	_Window->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

	_InputManager = OIS::InputManager::createInputSystem(pl);

	_Keyboard = static_cast<OIS::Keyboard*>(_InputManager->createInputObject( OIS::OISKeyboard, true ));
	_Mouse = static_cast<OIS::Mouse*>(_InputManager->createInputObject( OIS::OISMouse, true ));
}

void AppStateManager::setupWindowEventListener(void)
{
	//Set initial mouse clipping size
	windowResized(_Window);

	//Register as a Window listener
	Ogre::WindowEventUtilities::addWindowEventListener(_Window, this);
}

AppStateManager::~AppStateManager()
{

}

void AppStateManager::Enter(AppState* NewState)
{
	assert(!StateStack.empty());

	StateStack.back()->Pause();
	StateStack.push_back(NewState);
	NewState->Enter();
}

void AppStateManager::Exit(void)
{
	assert(!StateStack.empty());

	AppState * LastState = StateStack.back();
	LastState->Exit();
	StateStack.pop_back();
	delete LastState;
	
	if (!StateStack.empty())
	{
		StateStack.back()->Resume();
	}
}

void AppStateManager::SwitchTo(AppState* NewState)
{
	assert(!StateStack.empty());

	AppState * LastState = StateStack.back();
	LastState->Exit();
	StateStack.pop_back();
	delete LastState;

	StateStack.push_back(NewState);
	NewState->Enter();
}


void AppStateManager::MainLoop(AppState* InitialState)
{
	StateStack.push_back(InitialState);
	InitialState->Enter();

	float TimeSinceLastFrame = 0.001;
	
	_Shutdown = false;
	while (!_Shutdown)
	{
		Ogre::WindowEventUtilities::messagePump();
		if (_Window->isClosed()) _Shutdown = true;
		if (_Window->isActive())
		{
			int StartTime = _Timer->getMilliseconds();
			_Keyboard->capture();
			_Mouse->capture();
			
			StateStack.back()->Update(TimeSinceLastFrame);
			_OgreRoot->renderOneFrame(TimeSinceLastFrame);
			
			TimeSinceLastFrame = (_Timer->getMilliseconds() - StartTime) * 0.001f;
			
			if (StateStack.empty()) _Shutdown = true;
		}
		else
		{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
		}
	}
}

//Adjust mouse clipping area
void AppStateManager::windowResized(Ogre::RenderWindow* rw)
{
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	const OIS::MouseState &ms = _Mouse->getMouseState();
	ms.width = width;
	ms.height = height;
}

//Unattach OIS before window shutdown (very important under Linux)
void AppStateManager::windowClosed(Ogre::RenderWindow* rw)
{
	//Only close for window that created OIS
	if( rw == _Window )
	{
		if( _InputManager )
		{
			_InputManager->destroyInputObject(_Mouse);
			_InputManager->destroyInputObject(_Keyboard);

			OIS::InputManager::destroyInputSystem(_InputManager);
			_InputManager = 0;
		}
	}
}

AppState::~AppState()
{
}

AppState::AppState(void)
{
}

}
