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

#ifndef CHARACTERCONTROLLER_H
#define CHARACTERCONTROLLER_H

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include "RigidBody.h"

namespace pmd
{
class CharacterController
{
public:
	CharacterController(
		Ogre::SceneManager * SceneMgr,
		btDynamicsWorld * World,
		Ogre::Entity * Entity,
		float Height,
		float Radius,
		float Mass);
	~CharacterController(void);

	void TickCallback(void);

	btScalar _TargetHeading;
	btVector3 _TargetVelocity;
	bool _Jump;

	btRigidBody * _Body;
	Ogre::SceneNode * _Node;
	RigidBody<Ogre::SceneNode> _MotionState;
	btCapsuleShape _Shape;
	btVector3 _Inertia;
	btScalar _Mass;
	btDynamicsWorld * _World;
};
}

#endif // CHARACTERCONTROLLER_H