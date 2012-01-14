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
#include "AppStateManager.h"

Game::Game(void) :
	_Root(NULL),
	_Camera(NULL),
	_SceneMgr(NULL),
	_Window(NULL),
	_Viewport(NULL),
	_Mouse(NULL),
	_Keyboard(NULL),
	_Heading(0),
	_Pitch(0),

	_CollisionConfiguration(NULL),
	_Dispatcher(NULL),
	_OverlappingPairCache(NULL),
	_Solver(NULL),
	_World(NULL),
	
	_Player(NULL),
	_EscPressed(false)
{
}

Game::~Game()
{
}

void Game::Enter(void)
{
	_Root = AppStateManager::GetSingleton().GetOgreRoot();
	_Window = AppStateManager::GetSingleton().GetWindow();
	_Mouse = AppStateManager::GetSingleton().GetMouse();
	_Keyboard = AppStateManager::GetSingleton().GetKeyboard();
	
	_SceneMgr = _Root->createSceneManager("OctreeSceneManager");
	_Camera = _SceneMgr->createCamera("PlayerCam");
	_Viewport = _Window->addViewport(_Camera, 0);
	_Camera->setAspectRatio(Ogre::Real(_Viewport->getActualWidth()) / Ogre::Real(_Viewport->getActualHeight()));

	setupBullet();
	
	go();
}

void Game::Exit(void)
{
	cleanupBullet();

	AppStateManager::GetSingleton().GetWindow()->removeViewport(0);
	_SceneMgr->destroyCamera(_Camera);
	AppStateManager::GetSingleton().GetOgreRoot()->destroySceneManager(_SceneMgr);
}

void Game::Pause(void)
{
}

void Game::Resume(void)
{
}

void Game::StaticBulletCallback(btDynamicsWorld *world, btScalar timeStep)
{
	Game * g = static_cast<Game*>(world->getWorldUserInfo());
	g->BulletCallback(timeStep);
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

	_World->setGravity(btVector3(0, -20, 0));

	_World->setInternalTickCallback(
		&Game::StaticBulletCallback,
		static_cast<void*>(this),
		true);
}

void Game::cleanupBullet(void)
{
	delete _World;
	delete _Solver;
	delete _OverlappingPairCache;
	delete _Dispatcher;
	delete _CollisionConfiguration;
}
