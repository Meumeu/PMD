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
#include <OgreConfigFile.h>

namespace pmd
{
	Game::Game(void) :
		_Shutdown(false),
		_Root(NULL),
		_Camera(NULL),
		_SceneMgr(NULL),
		_Window(NULL),
		_Viewport(NULL),
		_InputManager(NULL),
		_Mouse(NULL),
		_Keyboard(NULL),
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		_ResourcesCfg("../etc/resources.cfg"),
		_PluginsCfg("/etc/OGRE/plugins.cfg"),
		_OgreCfg("../etc/ogre.cfg"),
		_OgreLog("../ogre.log"),
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		_ResourcesCfg("../etc/resources.cfg"),
		_PluginsCfg("../etc/plugins.cfg"),
		_OgreCfg("../etc/ogre.cfg"),
		_OgreLog("../ogre.log"),
#else
#error Unsupported platform
#endif
		_Player(NULL),
		_Pitch(0),
		_Heading(0),

		_CollisionConfiguration(0),
		_Dispatcher(0),
		_OverlappingPairCache(0),
		_Solver(0),
		_World(0),
		_PlayerBody(0)
	{
	}

	Game::~Game()
	{
	}

	void Game::setupResources(void)
	{
		// Load resource paths from config file
		Ogre::ConfigFile cf;
		cf.load(_ResourcesCfg);

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
	void Game::setupFrameListener(void)
	{
		OIS::ParamList pl;
		size_t windowHnd = 0;
		std::ostringstream windowHndStr;

		_Window->getCustomAttribute("WINDOW", &windowHnd);
		windowHndStr << windowHnd;
		pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

		_InputManager = OIS::InputManager::createInputSystem( pl );

		_Keyboard = static_cast<OIS::Keyboard*>(_InputManager->createInputObject( OIS::OISKeyboard, true ));
		_Mouse = static_cast<OIS::Mouse*>(_InputManager->createInputObject( OIS::OISMouse, true ));

		_Mouse->setEventCallback(this);
		_Keyboard->setEventCallback(this);

		//Set initial mouse clipping size
		windowResized(_Window);

		//Register as a Window listener
		Ogre::WindowEventUtilities::addWindowEventListener(_Window, this);

		_Root->addFrameListener(this);
	}

	bool Game::setup(void)
	{
		_Root = new Ogre::Root(_PluginsCfg, _OgreCfg, _OgreLog);

		if (!_Root->restoreConfig())
			if (!_Root->showConfigDialog())
				return false;
		
		_Window = _Root->initialise(true, "Ponies Must Die");
		_SceneMgr = _Root->createSceneManager("OctreeSceneManager");
		_Camera = _SceneMgr->createCamera("PlayerCam");

		// Create one viewport, entire window
		_Viewport = _Window->addViewport(_Camera);
		_Viewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

		Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

		// Alter the camera aspect ratio to match the viewport
		_Camera->setAspectRatio(
			Ogre::Real(_Viewport->getActualWidth()) / Ogre::Real(_Viewport->getActualHeight()));

		setupResources();

		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

		setupFrameListener();

		setupBullet();
		return true;
	}

	void Game::setupBullet(void)
	{
		_CollisionConfiguration = new btDefaultCollisionConfiguration();
		_Dispatcher = new btCollisionDispatcher(_CollisionConfiguration);
		_OverlappingPairCache = new btDbvtBroadphase();
		_Solver = new btSequentialImpulseConstraintSolver();

		_World = new btDiscreteDynamicsWorld(
			_Dispatcher,
			_OverlappingPairCache,
			_Solver,
			_CollisionConfiguration);

		_World->setGravity(btVector3(0, -10, 0));
	}

	void Game::cleanupBullet(void)
	{
		delete _World;
		delete _Solver;
		delete _OverlappingPairCache;
		delete _Dispatcher;
		delete _CollisionConfiguration;
	}

	void Game::cleanup(void)
	{
	}
	
	//Adjust mouse clipping area
	void Game::windowResized(Ogre::RenderWindow* rw)
	{
		unsigned int width, height, depth;
		int left, top;
		rw->getMetrics(width, height, depth, left, top);

		const OIS::MouseState &ms = _Mouse->getMouseState();
		ms.width = width;
		ms.height = height;
	}

	//Unattach OIS before window shutdown (very important under Linux)
	void Game::windowClosed(Ogre::RenderWindow* rw)
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
}