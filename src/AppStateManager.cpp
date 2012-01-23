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

#include "AppStateManager.h"
#include "pmd.h"

#include <OgreConfigFile.h>
#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <OIS/OISInputManager.h>

#include <boost/filesystem.hpp>

#include <RendererModules/Ogre/CEGUIOgreRenderer.h>
#include <CEGUIImageset.h>
#include <CEGUIScheme.h>
#include <CEGUIWindowManager.h>
#include <CEGUISystem.h>
#include <CEGUIFont.h>
#include <CEGUIWidgetModule.h>
#include <CEGUISchemeManager.h>
#include <CEGUIImagesetManager.h>
#include <CEGUIFontManager.h>

#include <CEGUI.h>

#ifndef _WINDOWS

#ifndef OGRE_PLUGINS_DIR
#error OGRE_PLUGINS_DIR not defined
#endif

#ifndef PATH_RESOURCES
#error PATH_RESOURCES not defined
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
	

#ifdef _WINDOWS
	char buf[MAX_PATH];
	if (!GetModuleFileName(NULL, buf, MAX_PATH))
		throw std::runtime_error("GetModuleFileName failed");
	
	char * last_slash = strrchr(buf, '\\');
	if (last_slash) *last_slash = 0;
	_ResourcesDir = buf;
	_LogDir = buf;
#else
	_ResourcesDir = PATH_RESOURCES;
	_LogDir = "/tmp";
#endif
	
	_OgreRoot = new Ogre::Root("", SettingsDir + "ogre.cfg", _LogDir + "/ogre.log");
	
#ifdef _WINDOWS
#	ifdef _DEBUG
	_OgreRoot->loadPlugin(_ResourcesDir + "/RenderSystem_GL_d.dll");
	_OgreRoot->loadPlugin(_ResourcesDir + "/Plugin_OctreeSceneManager_d.dll");
#	else
	_OgreRoot->loadPlugin(_ResourcesDir + "/RenderSystem_GL.dll");
	_OgreRoot->loadPlugin(_ResourcesDir + "/Plugin_OctreeSceneManager.dll");
#	endif
#else
	_OgreRoot->loadPlugin(OGRE_PLUGINS_DIR "/RenderSystem_GL.so");
	_OgreRoot->loadPlugin(OGRE_PLUGINS_DIR "/Plugin_OctreeSceneManager.so");
#endif
	
	if (!_OgreRoot->restoreConfig() && !_OgreRoot->showConfigDialog())
	{
		exit(0);
	}
}

AppStateManager::~AppStateManager()
{
	if (_OgreRoot)
	{
		delete _OgreRoot;
		_OgreRoot = 0;
	}
}

void AppStateManager::AddResourceDirectory(const std::string& path)
{
	Ogre::ResourceGroupManager& manager = Ogre::ResourceGroupManager::getSingleton();
	manager.addResourceLocation(path, "FileSystem", path);
	
	for (boost::filesystem::directory_iterator files(path), end; files != end ; ++files)
	{
		if (files->path().extension() == ".zip")
		{
			manager.addResourceLocation(files->path().string(), "Zip", path);
		}
	}
}

void AppStateManager::RemoveResourceDirectory(const std::string& path)
{
	Ogre::ResourceGroupManager& manager = Ogre::ResourceGroupManager::getSingleton();
	manager.destroyResourceGroup(path);
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
	if (_CeguiRootWindow)
	{
		_CeguiRootWindow->destroy();
		_CeguiRootWindow = 0;
	}
	
	if (_CeguiRenderer)
	{
		_CeguiRenderer->destroySystem();
		_CeguiRenderer = 0;
	}
	
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

		AddResourceDirectory(Singleton->_ResourcesDir + "/models");//default resources
		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
		
		Singleton->setupOIS();

		//Set initial mouse clipping size
		Singleton->windowResized(Singleton->_Window);
		
		new CEGUI::DefaultLogger;
		CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
		CEGUI::Logger::getSingleton().setLogFilename(Singleton->_LogDir + "/cegui.log");
		
		Singleton->_CeguiRenderer = &CEGUI::OgreRenderer::bootstrapSystem(*Singleton->_Window);
		CEGUI::System * GuiSystem = CEGUI::System::getSingletonPtr();
		
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation(Singleton->_ResourcesDir + "/gui", "FileSystem", "GUI");
		CEGUI::Imageset::setDefaultResourceGroup("GUI");
		CEGUI::Font::setDefaultResourceGroup("GUI");
		CEGUI::Scheme::setDefaultResourceGroup("GUI");
		CEGUI::WidgetLookManager::setDefaultResourceGroup("GUI");
		CEGUI::WindowManager::setDefaultResourceGroup("GUI");
		
		CEGUI::SchemeManager::getSingleton().create("TaharezLook.scheme");
		CEGUI::ImagesetManager::getSingleton().create("TaharezLook.imageset");

		CEGUI::FontManager::getSingleton().create("DejaVuSans-10.font");
		CEGUI::System::getSingleton().setDefaultFont("DejaVuSans-10");
		
		GuiSystem->setDefaultMouseCursor((CEGUI::utf8*)"TaharezLook", (CEGUI::utf8*)"MouseArrow");
		
		// set the mouse cursor initially in the middle of the screen
		GuiSystem->injectMousePosition((float)Singleton->_Window->getWidth() / 2.0f, (float)Singleton->_Window->getHeight() / 2.0f);
		
		Singleton->_CeguiRootWindow = CEGUI::WindowManager::getSingleton().createWindow("DefaultWindow", "Root");
		CEGUI::System::getSingleton().setGUISheet(Singleton->_CeguiRootWindow);
		
		float Height = Singleton->_Window->getHeight();
		float Width = Singleton->_Window->getWidth();
		if (Height * 4.0 / 3.0 < Width) Width = Height * 4.0 / 3.0;
		else if (Height * 4.0 / 3.0 > Width) Height = Width * 3.0 / 4.0;
		
		Singleton->_CeguiRootWindow->setSize(CEGUI::UVector2(CEGUI::UDim(0.0, Width), CEGUI::UDim(0.0, Height)));
		Singleton->_CeguiRootWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0.5, -Width / 2.0), CEGUI::UDim(0.5, -Height / 2.0)));

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

CEGUI::MouseButton AppStateManager::convertButton(OIS::MouseButtonID buttonID)
{
	switch (buttonID)
	{
	case OIS::MB_Left:
		return CEGUI::LeftButton;
	
	case OIS::MB_Right:
		return CEGUI::RightButton;
	
	case OIS::MB_Middle:
		return CEGUI::MiddleButton;
	
	default:
		return CEGUI::LeftButton;
	}
}

AppState::~AppState()
{
}

AppState::AppState(void)
{
}
