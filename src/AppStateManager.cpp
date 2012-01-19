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
#include "pmd.h"

#ifndef PATH_RenderSystem_GL
#ifdef _DEBUG
#	define PATH_RenderSystem_GL "RenderSystem_GL_d"
#else
#	define PATH_RenderSystem_GL "RenderSystem_GL"
#endif
#endif

#ifndef PATH_Plugin_OctreeSceneManager
#ifdef _DEBUG
#	define PATH_Plugin_OctreeSceneManager "Plugin_OctreeSceneManager_d"
#else
#	define PATH_Plugin_OctreeSceneManager "Plugin_OctreeSceneManager"
#endif
#endif

AppStateManager * AppStateManager::Singleton;
	
AppStateManager::AppStateManager(void) :
	_OgreRoot(0),
	_Window(0),
	_Timer(0),
	_InputManager(0),
	_Mouse(0),
	_Keyboard(0),
	_Shutdown(false)
{
	Singleton = this;
}

AppStateManager::~AppStateManager()
{
	if (_OgreRoot)
		_OgreRoot->removeFrameListener(this);

	if (_Window)
		Ogre::WindowEventUtilities::removeWindowEventListener(_Window, this);

	cleanupOIS();

	if (_Window)
	{
		_Window->destroy();
		_Window = 0;
	}

	if (_OgreRoot)
	{
		delete _OgreRoot;
		_OgreRoot = 0;
	}
}

bool AppStateManager::setup(std::string HomeDir)
{
	_OgreRoot = new Ogre::Root("", HomeDir + "ogre.cfg", HomeDir + "ogre.log");
	
	_OgreRoot->loadPlugin(PATH_RenderSystem_GL);
	_OgreRoot->loadPlugin(PATH_Plugin_OctreeSceneManager);
	
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

	//Set initial mouse clipping size
	windowResized(_Window);

	Ogre::WindowEventUtilities::addWindowEventListener(_Window, this);
	_OgreRoot->addFrameListener(this);

	return true;
}

void AppStateManager::setupResources(void)
{
	// Load resource paths from config file
	Ogre::ConfigFile cf;
	cf.load(PATH_RESOURCES "/resources.cfg");

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

	_Keyboard = static_cast<OIS::Keyboard*>(_InputManager->createInputObject(OIS::OISKeyboard, true));
	_Mouse = static_cast<OIS::Mouse*>(_InputManager->createInputObject(OIS::OISMouse, true));
}

void AppStateManager::cleanupOIS(void)
{
	if( _InputManager )
	{
		if (_Mouse)
		{
			_InputManager->destroyInputObject(_Mouse);
			_Mouse = 0;
		}

		if (_Keyboard)
		{
			_InputManager->destroyInputObject(_Keyboard);
			_Keyboard = 0;
		}

		OIS::InputManager::destroyInputSystem(_InputManager);
		_InputManager = 0;
	}

}

void AppStateManager::Enter(AppState* NewState)
{
	assert(!StateStack.empty());

	StateStack.back()->Pause();
	StateStack.push_back(NewState);
	_Keyboard->setEventCallback(NewState);
	_Mouse->setEventCallback(NewState);
	NewState->Enter();
}

void AppStateManager::Exit(void)
{
	assert(!StateStack.empty());

	AppState * LastState = StateStack.back();
	LastState->Exit();
	StateStack.pop_back();
	
	if (!StateStack.empty())
	{
		AppState * NewState = StateStack.back();
		
		_Keyboard->setEventCallback(NewState);
		_Mouse->setEventCallback(NewState);
		NewState->Resume();
	}
	else
	{
		_Keyboard->setEventCallback(NULL);
		_Mouse->setEventCallback(NULL);
	}

	delete LastState;
}

void AppStateManager::SwitchTo(AppState* NewState)
{
	assert(!StateStack.empty());

	AppState * LastState = StateStack.back();
	LastState->Exit();
	StateStack.pop_back();
	delete LastState;

	StateStack.push_back(NewState);
	_Keyboard->setEventCallback(NewState);
	_Mouse->setEventCallback(NewState);
	NewState->Enter();
}

bool AppStateManager::frameRenderingQueued(const Ogre::FrameEvent &evt)
{
	StateStack.back()->Update(evt.timeSinceLastFrame);

	_Keyboard->capture();
	_Mouse->capture();

	return !StateStack.empty();
}

void AppStateManager::MainLoop(AppState* InitialState)
{
	StateStack.push_back(InitialState);
	_Keyboard->setEventCallback(InitialState);
	_Mouse->setEventCallback(InitialState);
	InitialState->Enter();

	_OgreRoot->startRendering();
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
		cleanupOIS();
	}
}

AppState::~AppState()
{
}

AppState::AppState(void)
{
}
