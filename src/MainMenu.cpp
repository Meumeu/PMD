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

#include <CEGUIWindowManager.h>
#include <CEGUISystem.h>
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>
#include <elements/CEGUIPushButton.h>

#include "Game.h"

void MainMenu::Enter(void)
{
	_SceneMgr = AppStateManager::GetOgreRoot()->createSceneManager("OctreeSceneManager");
	Ogre::Camera * Camera = _SceneMgr->createCamera("PlayerCam");
	
	AppStateManager::GetWindow()->addViewport(Camera, -1);
	
	CEGUI::Window * menu = CEGUI::WindowManager::getSingleton().loadWindowLayout("MainMenu.layout");
	AppStateManager::GetCeguiRootWindow()->addChildWindow(menu);
	
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

bool MainMenu::Quit(const CEGUI::EventArgs &e)
{
	_Shutdown = true;
	return true;
}

void MainMenu::Exit(void)
{
}

void MainMenu::Pause(void)
{
	AppStateManager::GetOgreRenderer()->setRenderingEnabled(false);
}

void MainMenu::Resume(void)
{
	AppStateManager::GetOgreRenderer()->setRenderingEnabled(true);
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


bool MainMenu::mouseMoved(const OIS::MouseEvent& e)
{
	CEGUI::System::getSingleton().injectMouseMove(e.state.X.rel, e.state.Y.rel);
	if (e.state.Z.rel)
		CEGUI::System::getSingleton().injectMouseWheelChange(e.state.Z.rel / 120.0);
	
	return true;
}

bool MainMenu::mousePressed(const OIS::MouseEvent& , OIS::MouseButtonID id)
{
	CEGUI::System::getSingleton().injectMouseButtonDown(AppStateManager::convertButton(id));
	return true;
}

bool MainMenu::mouseReleased(const OIS::MouseEvent& , OIS::MouseButtonID id)
{
	CEGUI::System::getSingleton().injectMouseButtonUp(AppStateManager::convertButton(id));
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
}

MainMenu::~MainMenu()
{
}
