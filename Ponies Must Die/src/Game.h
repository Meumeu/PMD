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

#include "CharacterController.h"
#include "AppState.h"

namespace pmd
{
class Game :
	/*public Ogre::FrameListener,
	public Ogre::WindowEventListener,
	public OIS::KeyListener,
	public OIS::MouseListener*/
	public AppState
{
public:
	Game(void);
	~Game(void);
	
	virtual void Enter(void);
	virtual void Exit(void);
	virtual void Pause(void);
	virtual void Resume(void);
	
	virtual void Update(float TimeSinceLastFrame);

private:
	void go(void);
	void setupBullet(void);
	void cleanupBullet(void);

	static void StaticBulletCallback(btDynamicsWorld *world, btScalar timeStep);
	void BulletCallback(btScalar timeStep);

	Ogre::Root *           _Root;
	Ogre::Camera *         _Camera;
	Ogre::SceneManager *   _SceneMgr;
 	Ogre::RenderWindow *   _Window;
	Ogre::Viewport *       _Viewport;

	//OIS Input devices
 	OIS::Mouse *           _Mouse;
 	OIS::Keyboard *        _Keyboard;

	float                  _Heading;
	Ogre::Radian           _Pitch;

	btCollisionConfiguration * _CollisionConfiguration;
	btCollisionDispatcher *    _Dispatcher;
	btBroadphaseInterface *    _OverlappingPairCache;
	btConstraintSolver *       _Solver;
	btDynamicsWorld *          _World;

	CharacterController * _Player;
};
}

#endif // GAME_H
