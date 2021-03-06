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

#include "pmd.h"
#include "bullet/BulletDynamics/Dynamics/btRigidBody.h"
#include "bullet/BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "bullet/BulletCollision/CollisionShapes/btCylinderShape.h"
#include "bullet/BulletCollision/CollisionShapes/btCapsuleShape.h"

#include <OGRE/OgreVector3.h>
#include <OGRE/OgreSceneNode.h>

#include "RigidBody.h"
#include "CharacterAnimation.h"
#include "Pathfinding/Pathfinding.h"

#include <memory>
#include "environment.h"
#include "DebugDrawer.h"

namespace Ogre
{
	class Entity;
}

class CharacterController
{
public:
	CharacterController(
		Ogre::SceneManager *               SceneMgr,
		std::shared_ptr<btDynamicsWorld>   World,
		std::string                        MeshName,
		float                              Height,
		float                              Mass,
		btVector3&                         Position,
		float                              Heading,
		float                              InitialHitPoints);
	~CharacterController();

	void UpdatePhysics(btScalar dt);
	void UpdateGraphics(float dt);
	void SetVelocity(Ogre::Vector3 Velocity)
	{
		_TargetVelocity = btVector3(Velocity.x, Velocity.y, Velocity.z);
	}
	Ogre::Vector3 GetVelocity()
	{
		return Ogre::Vector3(_TargetVelocity.x(), _TargetVelocity.y(), _TargetVelocity.z());
	}
	void Jump(void)
	{
		_Jump = _GroundContact;
	}
	Ogre::Vector3 GetPosition()
	{
		return _Node->getPosition();
	}
	float GetHeading(void)
	{
		return _CurrentHeading;
	}
	const btRigidBody * GetBody()
	{
		return &_Body;
	}

	void Damage(float DamagePoints);

	void UpdateAITarget(Ogre::Vector3 const & target, std::shared_ptr<Environment> env, float velocity);
	void UpdateAI(float dt);
	void DebugDrawAI(DebugDrawer & dd);

private:
	btScalar                           _MaxYawSpeed;
	btScalar                           _CurrentHeading;
	btVector3                          _TargetVelocity;
	bool                               _Jump;
	bool                               _GroundContact;

	Ogre::SceneNode *                  _Node;
	RigidBody<Ogre::SceneNode>         _MotionState;
	Ogre::Entity *                     _Entity;
	Ogre::Vector3                      _MeshSize;
	Ogre::Vector3                      _MeshCenter;
	float                              _Scale;
	float                              _Radius;
	//btBoxShape                         _Shape;
	//btCapsuleShape                     _Shape;
	btCylinderShape                    _Shape;
	btScalar                           _Mass;
	btRigidBody                        _Body;
	std::shared_ptr<btDynamicsWorld>   _World;

	CharacterAnimation                 _Animations;

	float                              _IdleTime;
	Ogre::Vector3                      _CoG;

	Ogre::Vector3                      _CurrentTarget;
	Pathfinding::NavMesh::Path         _CurrentPath;
	size_t                             _CurrentPathIndex;
	float                              _CurrentPathAge;
	float                              _CurrentVelocity;

	float                              _HitPoints;
};

#endif // CHARACTERCONTROLLER_H
