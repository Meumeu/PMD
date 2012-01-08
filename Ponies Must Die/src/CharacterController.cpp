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

#include "CharacterController.h"
#include "pmd.h"

namespace pmd
{
CharacterController::CharacterController(
	Ogre::SceneManager * SceneMgr,
	btDynamicsWorld * World,
	Ogre::Entity * Entity,
	float Height,
	float Radius,
	float Mass) :
	_MaxYawSpeed(2 * 2 * M_PI),
	_Body(NULL),
	_MotionState(
		Ogre::Quaternion::IDENTITY,
		Ogre::Vector3::ZERO,
		Ogre::Vector3(0, Height / 2, 0),
		0),
	_Shape(Radius, Height-2*Radius)
{
	_Node = SceneMgr->getRootSceneNode()->createChildSceneNode();
	_Node->attachObject(Entity);

	_Mass = Mass;

	_Inertia = btVector3(0, 0, 0);

	_MotionState.setNode(_Node);
	_Body = new btRigidBody(Mass, &_MotionState, &_Shape, _Inertia);

	_World = World;

	World->addRigidBody(_Body);
}

CharacterController::~CharacterController(void)
{
	_World->removeRigidBody(_Body);
	delete _Body;

	_Node->getParentSceneNode()->removeChild(_Node->getName());
	delete _Node;
}

void CharacterController::TickCallback(btScalar dt)
{
	btVector3 TargetVelocity(0, 0, 0);
	btVector3 CurrentVelocity = _Body->getLinearVelocity();
	
	if (_TargetDirection.length2() > 0.5)
	{
		TargetVelocity = _TargetDirection.normalized() * _TargetVelocity;
		
		btScalar TargetHeading = atan2(-TargetVelocity.x(), -TargetVelocity.z());
		btScalar DeltaHeading = TargetHeading - _CurrentHeading;
		if (DeltaHeading > M_PI)
			DeltaHeading -= 2 * M_PI;
		else if (DeltaHeading < -M_PI)
			DeltaHeading += 2 * M_PI;
		
		if (DeltaHeading > _MaxYawSpeed * dt)
			DeltaHeading = _MaxYawSpeed * dt;
		else if(DeltaHeading < -_MaxYawSpeed * dt)
			DeltaHeading = -_MaxYawSpeed * dt;
		
		_CurrentHeading += DeltaHeading;
		if (_CurrentHeading > M_PI)
			_CurrentHeading -= 2 * M_PI;
		else if (_CurrentHeading < -M_PI)
			_CurrentHeading += 2 * M_PI;
	
		btQuaternion TargetQ(btVector3(0,1,0), _CurrentHeading);
		
		btTransform &comtr = (btTransform &)(_Body->getCenterOfMassTransform());
		comtr.setRotation(TargetQ);
	}
	btVector3 F = 10 * _Mass * (TargetVelocity - CurrentVelocity);
	F.setY(0);

	if (_Jump)
	{
		btVector3 Velocity = _Body->getLinearVelocity();
		Velocity.setY(5);
		_Body->setLinearVelocity(Velocity);
	}

	_Body->activate(true);
	_Body->applyCentralForce(F);
}
}
