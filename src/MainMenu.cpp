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

#include <CEGUI.h>
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>
#include <CEGUIImageset.h>
#include <CEGUIScheme.h>
#include "Game.h"

void MainMenu::Enter(void)
{
	_Root = AppStateManager::GetSingleton().GetOgreRoot();
	_Window = AppStateManager::GetSingleton().GetWindow();
	
	
	_Renderer = &CEGUI::OgreRenderer::bootstrapSystem(*_Window);
	CEGUI::System * GuiSystem = CEGUI::System::getSingletonPtr();
	
	CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
	
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
	//GuiSystem->injectMousePosition((float)_Window->getWidth() / 2.0f, (float)_Window->getHeight() / 2.0f);
	
	CEGUI::Window* root = CEGUI::WindowManager::getSingleton().createWindow("DefaultWindow", "Root");
	
	CEGUI::Window* menu = CEGUI::WindowManager::getSingleton().loadWindowLayout("MainMenu.layout");
	
	root->addChildWindow(menu);
	
	CEGUI::System::getSingleton().setGUISheet(root);
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
	if (e.key == OIS::KC_ESCAPE)
		mShutDown = true;
	
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
	//CEGUI::System::getSingleton().injectMouseMove(e.state.X.rel, e.state.Y.rel);
	CEGUI::System::getSingleton().injectMousePosition(e.state.X.abs, e.state.Y.abs);
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
	OIS::Keyboard * Keyboard = AppStateManager::GetSingleton().GetKeyboard();
	if (mShutDown)
	{
		AppStateManager::GetSingleton().Exit();
		return;
	}
	
	if (Keyboard->isKeyDown(OIS::KC_SPACE))
	{
		boost::shared_ptr<AppState> g(new Game);
		AppStateManager::GetSingleton().Enter(g);
		return;
	}
	
	CEGUI::System::getSingleton().injectTimePulse(dt);
}

MainMenu::MainMenu(void): AppState(),
	mShutDown(false)
{
	Ogre::ResourceGroupManager& manager = Ogre::ResourceGroupManager::getSingleton();
	std::string resources = AppStateManager::GetSingleton().GetResourcesDir();
	
	manager.addResourceLocation(resources + "/gui", "FileSystem", "GUI");
}

MainMenu::~MainMenu()
{
	Ogre::ResourceGroupManager& manager = Ogre::ResourceGroupManager::getSingleton();
	manager.clearResourceGroup("GUI");
}