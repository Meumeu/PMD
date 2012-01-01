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
	_MotionState(
		Ogre::Quaternion::IDENTITY,
		Ogre::Vector3::ZERO,
		Ogre::Vector3(0, Height / 2, 0),
		0),
	_Shape(Radius, Height-2*Radius),
	_Body(0)
{
	_Node = SceneMgr->getRootSceneNode()->createChildSceneNode();
	_Node->attachObject(Entity);

	_Mass = Mass;

	/*_Shape.calculateLocalInertia(Mass, _Inertia);
	_Inertia.setX(0);
	_Inertia.setZ(0);*/
	_Inertia = btVector3(0, 0, 0);

	_MotionState.setNode(_Node);
	_Body = new btRigidBody(Mass, &_MotionState, &_Shape, _Inertia);

	_World = World;

	World->addRigidBody(_Body);
}

CharacterController::~CharacterController(void)
{
	// TODO
//	_Node->getParentSceneNode()->removeChild(_Node->getName());
//	delete _Node;

	_World->removeRigidBody(_Body);
	delete _Body;

	_Node->getParentSceneNode()->removeChild(_Node->getName());
	delete _Node;
}

void CharacterController::TickCallback(void)
{
	btMatrix3x3 M(_Body->getOrientation());
	btVector3 CurrentVelocity = _Body->getLinearVelocity();
	btVector3 TargetVelocity = M * _TargetVelocity;
	btVector3 F = 10 * _Mass * (TargetVelocity - CurrentVelocity);
	F.setY(0);
	

	//btQuaternion CurrentQ = _Body->getOrientation();
	btQuaternion TargetQ(btVector3(0,1,0), _TargetHeading);

	/*const float freq = 5;
	const float w = freq * 2 * M_PI;
	const float ksi = 0.7;

	const float Kp = _Inertia.y() * w * w;
	const float Kd = _Inertia.y() * 2 * ksi * w;
	btQuaternion q = TargetQ * CurrentQ.inverse();
	if (q.getW() < 0) q = -q;

	float AngleError = q.getAngle();

	btVector3 T = Kp * q.getAxis() * AngleError - Kd * _Body->getAngularVelocity();

	T.setX(0);
	T.setZ(0);*/

	if (_Jump) F.setY(_Mass * 15);

	_Body->activate(true);
	_Body->applyCentralForce(F);
	/*_Body->applyTorque(T);*/
	btTransform &comtr = (btTransform &)(_Body->getCenterOfMassTransform());
	comtr.setRotation(TargetQ);
}
}
