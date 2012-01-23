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


#include "MainMenu.h"
#include "AppStateManager.h"

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

#include "Game.h"

void MainMenu::Enter(void)
{
	_Root = AppStateManager::GetOgreRoot();
	_Window = AppStateManager::GetWindow();
	
	_SceneMgr = _Root->createSceneManager("OctreeSceneManager");
	Ogre::Camera * Camera = _SceneMgr->createCamera("PlayerCam");
	
	_Window->addViewport(Camera, -1);
	
	new CEGUI::DefaultLogger;
	CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
	CEGUI::Logger::getSingleton().setLogFilename(AppStateManager::GetLogDir() + "/cegui.log");
	
	_Renderer = &CEGUI::OgreRenderer::bootstrapSystem(*_Window);
	CEGUI::System * GuiSystem = CEGUI::System::getSingletonPtr();
	
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
	GuiSystem->injectMousePosition((float)_Window->getWidth() / 2.0f, (float)_Window->getHeight() / 2.0f);
	
	CEGUI::Window * root = CEGUI::WindowManager::getSingleton().createWindow("DefaultWindow", "Root");
	
	float Height = _Window->getHeight();
	float Width = _Window->getWidth();
	if (Height * 4.0 / 3.0 < Width) Width = Height * 4.0 / 3.0;
	else if (Height * 4.0 / 3.0 > Width) Height = Width * 3.0 / 4.0;
	
	root->setSize(CEGUI::UVector2(CEGUI::UDim(0.0, Width), CEGUI::UDim(0.0, Height)));
	root->setPosition(CEGUI::UVector2(CEGUI::UDim(0.5, -Width / 2.0), CEGUI::UDim(0.5, -Height / 2.0)));
	
	CEGUI::Window * menu = CEGUI::WindowManager::getSingleton().loadWindowLayout("MainMenu.layout");
	root->addChildWindow(menu);
	
	CEGUI::System::getSingleton().setGUISheet(root);
	
	CEGUI::PushButton * btn;
	
	btn = (CEGUI::PushButton *)CEGUI::WindowManager::getSingleton().getWindow("MainMenu/Start");
	btn->setText((CEGUI::utf8 *)"Démarrer");
	btn->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&MainMenu::StartGame, this));

	btn = (CEGUI::PushButton *)CEGUI::WindowManager::getSingleton().getWindow("MainMenu/Options");
	btn->setText((CEGUI::utf8 *)"Options");
	btn->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&MainMenu::Options, this));

	btn = (CEGUI::PushButton *)CEGUI::WindowManager::getSingleton().getWindow("MainMenu/Quit");
	btn->setText((CEGUI::utf8 *)"Quitter");
	btn->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&MainMenu::Quit, this));
}

bool MainMenu::Options(const CEGUI::EventArgs& e)
{
	throw std::runtime_error("Unimplemented");
}

bool MainMenu::StartGame(const CEGUI::EventArgs& e)
{
	boost::shared_ptr<AppState> g(new Game);
	AppStateManager::Enter(g);
	return true;
}

void MainMenu::Exit(void)
{
	_Renderer->destroySystem();
}

void MainMenu::Pause(void)
{
	_Renderer->setRenderingEnabled(false);
}

void MainMenu::Resume(void)
{
	_Renderer->setRenderingEnabled(true);
}

bool MainMenu::keyPressed(const OIS::KeyEvent& e)
{
	CEGUI::System::getSingleton().injectKeyDown(e.key);
	CEGUI::System::getSingleton().injectChar(e.text);

	return true;
}

bool MainMenu::keyReleased(const OIS::KeyEvent& e)
{
	CEGUI::System::getSingleton().injectKeyUp(e.key);

	return true;
}

CEGUI::MouseButton MainMenu::convertButton(OIS::MouseButtonID buttonID)
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


bool MainMenu::mouseMoved(const OIS::MouseEvent& e)
{
	CEGUI::System::getSingleton().injectMouseMove(e.state.X.rel, e.state.Y.rel);
	//CEGUI::System::getSingleton().injectMousePosition(e.state.X.abs, e.state.Y.abs);
	if (e.state.Z.rel)
		CEGUI::System::getSingleton().injectMouseWheelChange(e.state.Z.rel / 120.0);
	
	return true;
}

bool MainMenu::mousePressed(const OIS::MouseEvent& , OIS::MouseButtonID id)
{
	CEGUI::System::getSingleton().injectMouseButtonDown(convertButton(id));
	return true;
}

bool MainMenu::mouseReleased(const OIS::MouseEvent& , OIS::MouseButtonID id)
{
	CEGUI::System::getSingleton().injectMouseButtonUp(convertButton(id));
	return true;
}

void MainMenu::Update(float dt)
{
	if (_Shutdown)
	{
		AppStateManager::Exit();
		return;
	}
	
	CEGUI::System::getSingleton().injectTimePulse(dt);
}

MainMenu::MainMenu(void): AppState(),
	_Shutdown(false)
{
	Ogre::ResourceGroupManager& manager = Ogre::ResourceGroupManager::getSingleton();
	std::string resources = AppStateManager::GetResourcesDir();
	
	manager.addResourceLocation(resources + "/gui", "FileSystem", "GUI");
}

MainMenu::~MainMenu()
{
	if (Ogre::Root::getSingletonPtr())
	{
		Ogre::ResourceGroupManager::getSingleton().clearResourceGroup("GUI");
	}
}
