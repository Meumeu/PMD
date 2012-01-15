/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011-2012  Guillaume Meunier <guillaume.meunier@centraliens.net>
    and 2012 Patrick Nicolas <patricknicolas@laposte.net>

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

CharacterController::CharacterController(
	Ogre::SceneManager * SceneMgr,
	btDynamicsWorld * World,
	Ogre::Entity * Entity,
	Ogre::SceneNode * Node,
	float Height,
	float Radius,
	float Mass) :
	_MaxYawSpeed(2 * 2 * M_PI),
	_CurrentHeading(0),
	_TargetVelocity(0, 0, 0),
	_Jump(false),
	_GroundContact(false),
	_Body(NULL),
	_MotionState(
		Ogre::Quaternion::IDENTITY,
		Ogre::Vector3::ZERO,
		Ogre::Vector3(0, Height / 2, 0),
		0),
	_Shape(Radius, Height-2*Radius),
	_Inertia(0, 0, 0),
	_Mass(Mass),
	_World(World),
	_Animations(Entity),
	_IdleTime(0),
	_JumpDelay(-1)
{
	//_Node = SceneMgr->getRootSceneNode()->createChildSceneNode();
	//_Node->attachObject(Entity);
	_Node = Node;

	_MotionState.setNode(_Node);
	_Body = new btRigidBody(Mass, &_MotionState, &_Shape, _Inertia);

	World->addRigidBody(_Body);
	
	_Animations.SetWeight("IdleTop", 1);
	_Animations.SetWeight("IdleBase", 1);

	_JumpStartDelay = _Animations.GetLength("JumpStart");
}

CharacterController::~CharacterController(void)
{
	_World->removeRigidBody(_Body);
	delete _Body;

	_Node->getParentSceneNode()->removeChild(_Node->getName());
	delete _Node;
}

void CharacterController::UpdatePhysics(btScalar dt)
{
	btVector3 TargetVelocity(0, 0, 0);
	btVector3 CurrentVelocity = _Body->getLinearVelocity();
	
	if (_TargetVelocity.length2() > 1)
	{
		btScalar TargetHeading = atan2(_TargetVelocity.x(), _TargetVelocity.z());
		TargetVelocity = _TargetDirection.normalized() * _TargetVelocity;
		
		btScalar TargetHeading = atan2(TargetVelocity.x(), TargetVelocity.z());
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
		
		_IdleTime = 0;
	}
	btVector3 F = 10 * _Mass * (_TargetVelocity - CurrentVelocity);
	F.setY(0);

	// Update collision status	
	int numManifolds = _World->getDispatcher()->getNumManifolds();
	_GroundContact = false;
	for (int i=0;i<numManifolds;i++)
	{
		btPersistentManifold* contactManifold =  _World->getDispatcher()->getManifoldByIndexInternal(i);
		
		if (contactManifold->getBody0() == _Body || contactManifold->getBody1() == _Body)
		{
			int numContacts = contactManifold->getNumContacts();
			for (int contact=0;contact<numContacts;contact++)
			{
				btManifoldPoint& pt = contactManifold->getContactPoint(contact);
				if (pt.getDistance()<0.f)
				{
					const btVector3& normalOnB = pt.m_normalWorldOnB;
					if (normalOnB.getY() !=0 )
					{
						_GroundContact = true;
					}
				}
			}
		}
	}

	if (_Jump && _GroundContact)
	{
		btVector3 Velocity = _Body->getLinearVelocity();
		Velocity.setY(5);
		_Body->setLinearVelocity(Velocity);
		_IdleTime = 0;
	}

	_JumpDelay -= dt;

	_Body->activate(true);
	_Body->applyCentralForce(F);
	
	_IdleTime += dt;
}

void CharacterController::UpdateGraphics(float dt)
{
	_Animations.ClearAnimations();
	
	if (_JumpDelay > 0)
	{
		_Animations.SetTime("JumpStart", _JumpStartDelay - _JumpDelay);
		_Animations.PushAnimation("JumpStart");
	}
	else if (!touche_le_sol)
	{
		_Animations.PushAnimation("JumpLoop");
	}
	else if (_TargetVelocity.length2() > 0)
	{
		_Animations.SetSpeed("RunBase", _TargetVelocity.length() / 10);
		_Animations.SetSpeed("RunTop", _TargetVelocity.length() / 10);
		_Animations.PushAnimation("RunBase");
		_Animations.PushAnimation("RunTop");
	}
	else
	{
		if (_IdleTime < 5)
		{
			_Animations.PushAnimation("IdleBase");
			_Animations.PushAnimation("IdleTop");
		}
		else if (_IdleTime < 15)
		{
			_Animations.SetAnimation("Dance");
		}
		else
		{
			_Animations.PushAnimation("IdleBase");
			_Animations.PushAnimation("IdleTop");
			_IdleTime = 0;
		}
	}
	
	_Animations.Update(dt);
}

