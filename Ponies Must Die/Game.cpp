#include "pmd.h"

namespace pmd
{
	Game::Game(void) :
		mShutdown(false),
		mRoot(NULL),
		mCamera(NULL),
		mSceneMgr(NULL),
		mWindow(NULL),
		mViewport(NULL),
		mInputManager(NULL),
		mMouse(NULL),
		mKeyboard(NULL),
		mResourcesCfg("../etc/resources.cfg"),
		mPluginsCfg("../etc/plugins.cfg"),
		mOgreCfg("../etc/ogre.cfg"),
		mOgreLog("../ogre.log")
	{
	}

	Game::~Game()
	{
	}

	bool Game::mouseMoved(const OIS::MouseEvent &arg)
	{
		return true;
	}

	bool Game::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
	{
		return true;
	}

	bool Game::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
	{
		return true;
	}

	bool Game::keyPressed(const OIS::KeyEvent &arg)
	{
		if (arg.key == OIS::KC_ESCAPE)
			mShutdown = true;
		return true;
	}

	bool Game::keyReleased(const OIS::KeyEvent &arg)
	{
		return true;
	}

	bool Game::frameRenderingQueued(const Ogre::FrameEvent& evt)
	{
		if(mWindow->isClosed())
			return false;

		if(mShutdown)
			return false;

		//Need to capture/update each device
		mKeyboard->capture();
		mMouse->capture();

		return true;
	}

	void Game::setupResources(void)
	{
		// Load resource paths from config file
		Ogre::ConfigFile cf;
		cf.load(mResourcesCfg);

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
		Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
		OIS::ParamList pl;
		size_t windowHnd = 0;
		std::ostringstream windowHndStr;
Ogre::LogManager::getSingletonPtr()->logMessage("*** toto ***");
		mWindow->getCustomAttribute("WINDOW", &windowHnd);
		windowHndStr << windowHnd;
		pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
Ogre::LogManager::getSingletonPtr()->logMessage("*** titi ***");
		mInputManager = OIS::InputManager::createInputSystem( pl );
Ogre::LogManager::getSingletonPtr()->logMessage("*** tata ***");

		mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject( OIS::OISKeyboard, true ));
		mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject( OIS::OISMouse, true ));
Ogre::LogManager::getSingletonPtr()->logMessage("*** tutu ***");

		mMouse->setEventCallback(this);
		mKeyboard->setEventCallback(this);
Ogre::LogManager::getSingletonPtr()->logMessage("*** tyty ***");

		//Set initial mouse clipping size
		windowResized(mWindow);
Ogre::LogManager::getSingletonPtr()->logMessage("*** tete ***");

		//Register as a Window listener
		Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

		mRoot->addFrameListener(this);
	}

	bool Game::setup(void)
	{
		mRoot = new Ogre::Root(mPluginsCfg, mOgreCfg, mOgreLog);
		if (!mRoot->showConfigDialog()) return false;
		
		mWindow = mRoot->initialise(true, "Ponies Must Die");
		mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);
		mCamera = mSceneMgr->createCamera("PlayerCam");

		// Create one viewport, entire window
		mViewport = mWindow->addViewport(mCamera);
		mViewport->setBackgroundColour(Ogre::ColourValue(0,0,0));

		Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

		// Alter the camera aspect ratio to match the viewport
		mCamera->setAspectRatio(
			Ogre::Real(mViewport->getActualWidth()) / Ogre::Real(mViewport->getActualHeight()));

		setupResources();

		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

		setupFrameListener();
		return true;
	}

	void Game::cleanup(void)
	{
	}

	void Game::go(void)
	{
		if (!setup())
			return;
		
		mRoot->startRendering();

		cleanup();
	}

	//Adjust mouse clipping area
	void Game::windowResized(Ogre::RenderWindow* rw)
	{
		unsigned int width, height, depth;
		int left, top;
		rw->getMetrics(width, height, depth, left, top);

		const OIS::MouseState &ms = mMouse->getMouseState();
		ms.width = width;
		ms.height = height;
	}

	//Unattach OIS before window shutdown (very important under Linux)
	void Game::windowClosed(Ogre::RenderWindow* rw)
	{
		//Only close for window that created OIS (the main window in these demos)
		if( rw == mWindow )
		{
			if( mInputManager )
			{
				mInputManager->destroyInputObject( mMouse );
				mInputManager->destroyInputObject( mKeyboard );

				OIS::InputManager::destroyInputSystem(mInputManager);
				mInputManager = 0;
			}
		}
	}

}
