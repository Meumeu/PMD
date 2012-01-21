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

#ifndef CHARACTERCONTROLLER_H
#define CHARACTERCONTROLLER_H

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include "RigidBody.h"
#include "CharacterAnimation.h"

#include <boost/shared_ptr.hpp>

class CharacterController
{
public:
	CharacterController(
		Ogre::SceneManager *               SceneMgr,
		boost::shared_ptr<btDynamicsWorld> World,
		Ogre::Entity *                     Entity,
		Ogre::SceneNode *                  Node,
		float                              SizeX,
		float                              Height,
		float                              SizeZ,
		float                              Mass);
	~CharacterController(void);

	void UpdatePhysics(btScalar dt);
	void UpdateGraphics(float dt);

	btScalar                           _MaxYawSpeed;
	btScalar                           _CurrentHeading;
	btVector3                          _TargetVelocity;
	bool                               _Jump;
	bool                               _GroundContact;
	
	Ogre::SceneNode *                  _Node;
	RigidBody<Ogre::SceneNode>         _MotionState;
	btBoxShape                         _Shape;
	btRigidBody                        _Body;
	btScalar                           _Mass;
	boost::shared_ptr<btDynamicsWorld> _World;
	
	CharacterAnimation                 _Animations;
	
	float                              _IdleTime;
};

#endif // CHARACTERCONTROLLER_H
