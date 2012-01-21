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
#include <boost/filesystem.hpp>

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
	
AppStateManager::AppStateManager(std::string SettingsDir) :
	_OgreRoot(0),
	_Window(0),
	_Timer(0),
	_InputManager(0),
	_Mouse(0),
	_Keyboard(0),
	_Shutdown(false),
	_SettingsDir(SettingsDir)
{
	if (Singleton) abort();
	Singleton = this;
	
	_OgreRoot = new Ogre::Root("", SettingsDir + "ogre.cfg", SettingsDir + "ogre.log");
	
	_OgreRoot->loadPlugin(PATH_RenderSystem_GL);
	_OgreRoot->loadPlugin(PATH_Plugin_OctreeSceneManager);
	
	if (!_OgreRoot->restoreConfig() && !_OgreRoot->showConfigDialog())
	{
		exit(0);
	}
	
#ifdef _WINDOWS
	char buf[MAX_PATH];
	if (!GetModuleFileName(NULL, buf, MAX_PATH))
		throw std::runtime_error("GetModuleFileName failed");
	
	char * last_slash = strrchr(buf, '\\');
	if (last_slash) *last_slash = 0;

	_ResourcesDir = buf;
#else
	_ResourcesDir = PATH_RESOURCES;
#endif
}

AppStateManager::~AppStateManager()
{
	if (_OgreRoot)
	{
		delete _OgreRoot;
		_OgreRoot = 0;
	}
}

void AppStateManager::setupResources(void)
{
	Ogre::ResourceGroupManager& manager = Ogre::ResourceGroupManager::getSingleton();
	manager.addResourceLocation(_ResourcesDir + "/models", "FileSystem", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	for (boost::filesystem::directory_iterator files(_ResourcesDir + "/models"), end; files != end ; ++files)
	{
		if (files->path().extension() == ".zip")
		{
			manager.addResourceLocation(files->path().string(), "Zip", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
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

void AppStateManager::Enter(boost::shared_ptr<AppState> NewState)
{
	assert(!Singleton->StateStack.empty());

	Singleton->StateStack.back()->Pause();
	Singleton->StateStack.push_back(NewState);
	Singleton->_Keyboard->setEventCallback(NewState.get());
	Singleton->_Mouse->setEventCallback(NewState.get());
	NewState->Enter();
}

void AppStateManager::Exit(void)
{
	assert(!Singleton->StateStack.empty());

	boost::shared_ptr<AppState> LastState = Singleton->StateStack.back();
	LastState->Exit();
	Singleton->StateStack.pop_back();
	
	if (!Singleton->StateStack.empty())
	{
		boost::shared_ptr<AppState> NewState = Singleton->StateStack.back();
		
		Singleton->_Keyboard->setEventCallback(NewState.get());
		Singleton->_Mouse->setEventCallback(NewState.get());
		NewState->Resume();
	}
	else
	{
		Singleton->_Keyboard->setEventCallback(NULL);
		Singleton->_Mouse->setEventCallback(NULL);
	}
}

void AppStateManager::SwitchTo(boost::shared_ptr<AppState> NewState)
{
	assert(!Singleton->StateStack.empty());

	boost::shared_ptr<AppState> LastState = Singleton->StateStack.back();
	LastState->Exit();
	Singleton->StateStack.pop_back();

	Singleton->StateStack.push_back(NewState);
	Singleton->_Keyboard->setEventCallback(NewState.get());
	Singleton->_Mouse->setEventCallback(NewState.get());
	NewState->Enter();
}

bool AppStateManager::frameRenderingQueued(const Ogre::FrameEvent &evt)
{
	StateStack.back()->Update(evt.timeSinceLastFrame);

	_Keyboard->capture();
	_Mouse->capture();

	return !StateStack.empty();
}

void AppStateManager::cleanup()
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
}

void AppStateManager::MainLoop(boost::shared_ptr<AppState> InitialState)
{
	Singleton->_Window = Singleton->_OgreRoot->initialise(true, "Ponies Must Die");

	try
	{
		Singleton->_Timer = Singleton->_OgreRoot->getTimer();
		
		Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

		Singleton->setupResources();
		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
		
		Singleton->setupOIS();

		//Set initial mouse clipping size
		Singleton->windowResized(Singleton->_Window);

		Ogre::WindowEventUtilities::addWindowEventListener(Singleton->_Window, Singleton);
		Singleton->_OgreRoot->addFrameListener(Singleton);

		Singleton->StateStack.push_back(InitialState);
		Singleton->_Keyboard->setEventCallback(InitialState.get());
		Singleton->_Mouse->setEventCallback(InitialState.get());
		InitialState->Enter();

		Singleton->_OgreRoot->startRendering();
	}
	catch(...)
	{
		Singleton->cleanup();
		throw;
	}

	Singleton->cleanup();
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
