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

#include <OGRE/OgreSceneManager.h>
#include <OGRE/OgreEntity.h>

CharacterController::CharacterController(
	Ogre::SceneManager *               SceneMgr,
	std::shared_ptr<btDynamicsWorld>   World,
	std::string                        MeshName,
	float                              Height,
	float                              Mass,
	btVector3&                         Position,
	float                              Heading) :
	_MaxYawSpeed(2 * 2 * M_PI),
	_CurrentHeading(0),
	_TargetVelocity(0, 0, 0),
	_Jump(false),
	_GroundContact(false),
	_MotionState(
		Ogre::Quaternion::IDENTITY,
		Ogre::Vector3::ZERO,
		Ogre::Vector3(0, Height / 2, 0),
		0),
	_Entity(SceneMgr->createEntity(MeshName)),
	_MeshSize(_Entity->getBoundingBox().getMaximum() - _Entity->getBoundingBox().getMinimum()),
	_MeshCenter((_Entity->getBoundingBox().getMaximum() + _Entity->getBoundingBox().getMinimum()) / 2),
	_Scale(Height / _MeshSize.y),
	_Shape(btVector3(
		_Scale * _MeshSize.x / 2,
		Height / 2,
		_Scale * _MeshSize.z / 2)),
	//_Shape(Height - _Scale * _MeshSize.z, _Scale * _MeshSize.z / 2),
	_Mass(Mass),
	_Body(_Mass, &_MotionState, &_Shape, btVector3(0, 0, 0)),
	_World(World),
	_Animations(_Entity),
	_IdleTime(0),
	_CoG(0, Height / 2, 0),
	_CurrentPathIndex(0),
	_CurrentPathAge(FLT_MAX)
{
	_Node = SceneMgr->getRootSceneNode()->createChildSceneNode(
		Ogre::Vector3(Position.x(), Position.y(), Position.z()),
		Ogre::Quaternion(Ogre::Radian(Heading), Ogre::Vector3::UNIT_Y));

	Ogre::SceneNode * entnode = _Node->createChildSceneNode(
		Ogre::Vector3(
			-_MeshCenter.x * _Scale,
			-_Entity->getBoundingBox().getMinimum().y * _Scale,
			-_MeshCenter.z * _Scale));

	entnode->scale(_Scale, _Scale, _Scale);
	entnode->attachObject(_Entity);

	_Body.setCenterOfMassTransform(btTransform(btQuaternion(btVector3(0, 1, 0), Heading), Position + btVector3(0, _CoG.y, 0)));
	_CurrentHeading = Heading;

	_MotionState.setNode(_Node);

	World->addRigidBody(&_Body);

	_Animations.SetWeight("IdleTop", 1);
	_Animations.SetWeight("IdleBase", 1);
}

CharacterController::~CharacterController(void)
{
	_World->removeRigidBody(&_Body);

	_Node->getParentSceneNode()->removeChild(_Node->getName());
}

void CharacterController::UpdatePhysics(btScalar dt)
{
	bool IsIdle = true;
	btVector3 CurrentVelocity = _Body.getLinearVelocity();

	if (_TargetVelocity.length2() > 1)
	{
		btScalar TargetHeading = atan2(_TargetVelocity.x(), _TargetVelocity.z());

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

		btTransform comtr = _Body.getCenterOfMassTransform();
		comtr.setRotation(TargetQ);
		_Body.setCenterOfMassTransform(comtr);

		IsIdle = false;
	}
	btVector3 F = 10 * _Mass * (_TargetVelocity - CurrentVelocity);
	F.setY(0);

	// Update collision status
	int numManifolds = _World->getDispatcher()->getNumManifolds();
	_GroundContact = false;
	for(int i=0;i<numManifolds;i++)
	{
		btPersistentManifold* contactManifold =  _World->getDispatcher()->getManifoldByIndexInternal(i);

		if (contactManifold->getBody0() == &_Body || contactManifold->getBody1() == &_Body)
		{
			int numContacts = contactManifold->getNumContacts();
			for(int contact=0; contact < numContacts; contact++)
			{
				btManifoldPoint& pt = contactManifold->getContactPoint(contact);
				if (pt.getDistance() < 0.1f)
				{
					const btVector3& normalOnB = pt.m_normalWorldOnB;
					if (normalOnB.getY() != 0)
					{
						_GroundContact = true;
					}
				}
			}
		}
	}

	if (_Jump && _GroundContact)
	{
		_Jump = false;

		btVector3 Velocity = _Body.getLinearVelocity();
		Velocity.setY(9);
		_Body.setLinearVelocity(Velocity);
	}

	if (!_GroundContact)
		IsIdle = false;

	_Body.activate(true);
	_Body.applyCentralForce(F);

	_IdleTime = IsIdle ? _IdleTime + dt : 0;
}

void CharacterController::UpdateGraphics(float dt)
{
	_Animations.ClearAnimations();

	if (!_GroundContact)
	{
		_Animations.PushAnimation("JumpLoop");
	}
	else if (_TargetVelocity.length2() > 0)
	{
		_Animations.SetSpeed("RunBase", _TargetVelocity.length() / 10);
		_Animations.SetSpeed("RunTop", _TargetVelocity.length() / 10);
		_Animations.PushAnimation("RunBase");
		_Animations.PushAnimation("RunTop");
		_Animations.PushAnimation("my_animation");
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

void CharacterController::UpdateAITarget(const Ogre::Vector3& target, std::shared_ptr< Environment > env, float velocity)
{
	if (target.squaredDistance(_CurrentTarget) > 0.001 || _CurrentPathAge > 0.1)
	{
		boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();
		_CurrentPath = env->QueryPath(GetPosition(), target);
		boost::posix_time::ptime t2 = boost::posix_time::microsec_clock::universal_time();
		_CurrentPathIndex = 0;
		_CurrentTarget = target;
		_CurrentVelocity = velocity;
		std::cerr << "Pathfinding in " << (t2-t1).total_microseconds() << " µs\n";
	}
}


void CharacterController::UpdateAI(float dt)
{
	_CurrentPathAge += dt;
	//std::cerr << _CurrentPathIndex << " / " << _CurrentPath.size() << "\n";
	if (_CurrentPathIndex + 2 <= _CurrentPath.size())
	{
		Ogre::Vector3 const & a = _CurrentPath[_CurrentPathIndex];
		Ogre::Vector3 const & b = _CurrentPath[_CurrentPathIndex+1];
		Ogre::Vector3 const & m = GetPosition();
		Ogre::Vector3 const ab = b - a;
		Ogre::Vector3 const am = m - a;
		Ogre::Vector3 const mb = b - m;
		Ogre::Vector3 const mb_unit = mb.normalisedCopy();
		
		float remaining = mb.length();
		for(size_t i = _CurrentPathIndex + 1; i + 2 <= _CurrentPath.size(); ++i)
		{
			remaining += _CurrentPath[i].distance(_CurrentPath[i+1]);
		}
		std::cerr << "Remaining distance: " << remaining << " m\n";
		
		float lambda = am.dotProduct(ab) / ab.squaredLength();
		
		//const float tau = 0.1;
		
		SetVelocity(_CurrentVelocity * mb_unit);
		
		if (lambda > 1) _CurrentPathIndex++;
		/*if (_CurrentPathIndex + 1 == _CurrentPath.size())
			SetVelocity(Ogre::Vector3(0.));*/
	}
	
	return;
	
	
#if 0
	_CurrentPathAge += dt;
	if (_CurrentPathAge > 0.2 /*&& target.squaredDistance(_CurrentTarget) > 0.5 * 0.5*/)
	{
		_CurrentPathAge = 0;
// 		_CurrentTarget = target;
		
		boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();
		_CurrentPath = env->QueryPath(GetPosition(), target);
		boost::posix_time::ptime t2 = boost::posix_time::microsec_clock::universal_time();
		std::cerr << "Pathfinding in " << (t2-t1).total_microseconds() << " µs\n";
		//if (_CurrentPath.empty()) Jump();
		
		//_CurrentPathIndex = 0;
		
		if (_CurrentPath.size() < 2) return;
		Ogre::Vector3 target = _CurrentPath[1] - GetPosition();
		target.normalise();

		SetVelocity(target * velocity);
	}
	
	/*if (_CurrentPath.empty()) return;
	Ogre::Vector3 const &  Pos = GetPosition();
	size_t PathSize = _CurrentPath.size();
	
	while(_CurrentPathIndex < PathSize - 1)
	{
		Ogre::Vector3 am = Pos - _CurrentPath[_CurrentPathIndex];
		Ogre::Vector3 ab = _CurrentPath[_CurrentPathIndex + 1] - _CurrentPath[_CurrentPathIndex];
		float ab2 = ab.squaredLength();
		
		if (am.dotProduct(ab) < ab2) break;
		_CurrentPathIndex++;
	}
	
	Ogre::Vector3 inst_target = _CurrentPath[_CurrentPathIndex+1] - Pos;
	inst_target.normalise();
	SetVelocity(inst_target * velocity);*/
#endif
}
