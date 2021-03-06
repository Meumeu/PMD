/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011-2012  Guillaume Meunier <guillaume.meunier@centraliens.net>

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
#include "environment.h"

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
	_EscPressed(false),
	_DebugAI(false)
{
}

Game::~Game()
{
}

void Game::Enter(void)
{
	_Root = AppStateManager::GetOgreRoot();
	_Window = AppStateManager::GetWindow();
	_Mouse = AppStateManager::GetMouse();
	_Keyboard = AppStateManager::GetKeyboard();

	_SceneMgr = _Root->createSceneManager("OctreeSceneManager");
	_Camera = _SceneMgr->createCamera("PlayerCam");
	_Viewport = _Window->addViewport(_Camera, 0);
	_Camera->setAspectRatio(Ogre::Real(_Viewport->getActualWidth()) / Ogre::Real(_Viewport->getActualHeight()));

	setupBullet();

	_dd = std::unique_ptr<DebugDrawer>(new DebugDrawer(_SceneMgr, 0.5));
	_dd->setEnabled(true);
	
	go();
}

void Game::Exit(void)
{
	_dd.reset();
	
	_Player = std::shared_ptr<CharacterController>();
	_Enemies.clear();
	_Env = std::shared_ptr<Environment>();
	cleanupBullet();

	AppStateManager::GetWindow()->removeViewport(0);
	_SceneMgr->destroyCamera(_Camera);
	AppStateManager::GetOgreRoot()->destroySceneManager(_SceneMgr);
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
	_CollisionConfiguration = std::shared_ptr<btCollisionConfiguration>(new btDefaultCollisionConfiguration());
	_Dispatcher = std::shared_ptr<btCollisionDispatcher>(new btCollisionDispatcher(_CollisionConfiguration.get()));
	_OverlappingPairCache = std::shared_ptr<btBroadphaseInterface>(new btDbvtBroadphase());
	_Solver = std::shared_ptr<btConstraintSolver>(new btSequentialImpulseConstraintSolver());

	_World = std::shared_ptr<btDynamicsWorld>(new btDiscreteDynamicsWorld(
		_Dispatcher.get(),
		_OverlappingPairCache.get(),
		_Solver.get(),
		_CollisionConfiguration.get()));

	_World->setGravity(btVector3(0, -20, 0));

	_World->setInternalTickCallback(
		&Game::StaticBulletCallback,
		static_cast<void*>(this),
		true);

	_bulletDebug = std::unique_ptr<BulletDebug>(new BulletDebug(*_SceneMgr, *_World));
}

void Game::cleanupBullet(void)
{
	_bulletDebug.reset();
	
	_World = std::shared_ptr<btDynamicsWorld>();
	_Dispatcher = std::shared_ptr<btCollisionDispatcher>();
}
