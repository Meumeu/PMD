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

#ifndef GAME_H
#define GAME_H


#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>
#include <OgreRenderWindow.h>
#include <OgreRoot.h>

#include <OISInputManager.h>
#include <OISKeyboard.h>
#include <OISMouse.h>

#include <btBulletDynamicsCommon.h>

#include <boost/shared_ptr.hpp>

#include "CharacterController.h"
#include "AppState.h"
#include "btOgre/BtOgreExtras.h"

class Environment;
class Game : public AppState
{
public:
	Game(void);
	~Game(void);
	
	virtual void Enter(void);
	virtual void Exit(void);
	virtual void Pause(void);
	virtual void Resume(void);
	
	virtual void Update(float TimeSinceLastFrame);
	
	virtual bool keyPressed(const OIS::KeyEvent&);

private:
	void go(void);
	void setupBullet(void);
	void cleanupBullet(void);

	static void StaticBulletCallback(btDynamicsWorld *world, btScalar timeStep);
	void BulletCallback(btScalar timeStep);

	boost::shared_ptr<CharacterController> CreateCharacter(
		std::string MeshName,
		float HeightY,
		float mass,
		btVector3& position,
		float heading = 0);
	
	Ogre::Root *                                         _Root;
	Ogre::Camera *                                       _Camera;
	Ogre::SceneManager *                                 _SceneMgr;
 	Ogre::RenderWindow *                                 _Window;
	Ogre::Viewport *                                     _Viewport;

	//OIS Input devices
	OIS::Mouse *                                         _Mouse;
	OIS::Keyboard *                                      _Keyboard;

	Ogre::Radian                                         _Heading;
	Ogre::Radian                                         _Pitch;

	boost::shared_ptr<btCollisionConfiguration>          _CollisionConfiguration;
	boost::shared_ptr<btCollisionDispatcher>             _Dispatcher;
	boost::shared_ptr<btBroadphaseInterface>             _OverlappingPairCache;
	boost::shared_ptr<btConstraintSolver>                _Solver;
	boost::shared_ptr<btDynamicsWorld>                   _World;

	boost::shared_ptr<CharacterController>               _Player;
	std::vector<boost::shared_ptr<CharacterController> > _Enemies;

	boost::shared_ptr<Environment>                       _Env;

#ifdef PHYSICS_DEBUG
	BtOgre::DebugDrawer *                                _debugDrawer;
#endif

	bool                                                 _EscPressed;
};

#endif // GAME_H
