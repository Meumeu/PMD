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
#include "RigidBody.h"
#include "environment.h"
#include "AppStateManager.h"
#include "CharacterController.h"

#include <stdio.h>
#include <OgreEntity.h>
#include <OgreMeshManager.h>

#include "bullet/btBulletDynamicsCommon.h"
#include "bullet/btBulletCollisionCommon.h"

const float CameraDistance = 2;
const float CameraMargin = 0.01;
const float CameraHeight = 1.7;

class CameraCollisionCallback : public btCollisionWorld::RayResultCallback
{
public:
	CameraCollisionCallback(const btRigidBody * player) :  _hitfraction(1), _player(player) {};
	virtual ~CameraCollisionCallback() {};
	virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool /* normalInWorldSpace */)
	{
		if (rayResult.m_collisionObject != _player)
		{
			if (rayResult.m_hitFraction < _hitfraction)
			{
				_hitfraction = rayResult.m_hitFraction;
			}
		}
		return rayResult.m_hitFraction;
	}

	float _hitfraction;

private:
	const btRigidBody * _player;
};

bool Game::keyPressed(const OIS::KeyEvent& e)
{
	if (e.key == OIS::KC_ESCAPE)
	{
		_EscPressed = true;
	}

	if (e.key == OIS::KC_SPACE)
	{
		_Player->Jump();
	}

	return true;
}

void Game::Update(float TimeSinceLastFrame)
{
	if (_Window->isClosed()) return;

	if (_EscPressed)
	{
		AppStateManager::Exit();
		return;
	}

	float velX = 0, velZ = 0;

	if (_Keyboard->isKeyDown(OIS::KC_Z) || _Keyboard->isKeyDown(OIS::KC_W))
	{
		velZ = -1;
	}
	else if (_Keyboard->isKeyDown(OIS::KC_S))
	{
		velZ = 1;
	}

	if (_Keyboard->isKeyDown(OIS::KC_Q) || _Keyboard->isKeyDown(OIS::KC_A))
	{
		velX = -1;
	}
	else if (_Keyboard->isKeyDown(OIS::KC_D))
	{
		velX = 1;
	}

	Ogre::Vector3 TargetVelocity = Ogre::Vector3(
		velX * cos(_Heading.valueRadians()) + velZ * sin(_Heading.valueRadians()),
		0,
		-velX * sin(_Heading.valueRadians()) + velZ * cos(_Heading.valueRadians()));

	if (TargetVelocity != Ogre::Vector3(0,0,0))
		TargetVelocity.normalise();

	if (_Keyboard->isKeyDown(OIS::KC_LSHIFT))
	{
		_Player->SetVelocity(10 * TargetVelocity);
	}
	else
	{
		_Player->SetVelocity(3 * TargetVelocity);
	}

	OIS::MouseState ms = _Mouse->getMouseState();

	_Pitch -= Ogre::Radian(ms.Y.rel * 0.003);
	if (_Pitch.valueRadians() > M_PI / 2)
		_Pitch = Ogre::Radian(M_PI / 2);
	else if (_Pitch.valueRadians() < -M_PI / 2)
		_Pitch = Ogre::Radian(-M_PI / 2);

	_Heading -= Ogre::Radian(ms.X.rel * 0.003);
	if (_Heading.valueRadians() > M_PI)
		_Heading -= Ogre::Radian(2 * M_PI);
	else if (_Heading.valueRadians() < -M_PI)
		_Heading += Ogre::Radian(2 * M_PI);

	_Camera->setOrientation(
		Ogre::Quaternion(_Heading, Ogre::Vector3::UNIT_Y) *
		Ogre::Quaternion(_Pitch, Ogre::Vector3::UNIT_X));

	_World->stepSimulation(TimeSinceLastFrame, 3);

#ifdef PHYSICS_DEBUG
	_debugDrawer->step();
#endif

	_Player->UpdateGraphics(TimeSinceLastFrame);
	for(auto & cc : _Enemies)
	{
		cc->UpdateGraphics(TimeSinceLastFrame);
	}

	btVector3 CamDirection(
		 cos(_Pitch.valueRadians()) * sin(_Heading.valueRadians()),
		-sin(_Pitch.valueRadians()),
		 cos(_Pitch.valueRadians()) * cos(_Heading.valueRadians()));

	btVector3 Cam1(
		_Player->GetPosition().x,
		_Player->GetPosition().y + CameraHeight,
		_Player->GetPosition().z);

	btVector3 Cam2 = Cam1 + (CameraDistance + CameraMargin) * CamDirection;

	CameraCollisionCallback CamCallback(_Player->GetBody());

	_World->rayTest(Cam1, Cam2, CamCallback);
	Ogre::Vector3 CameraPosition(
		Cam1.x() + (CamCallback._hitfraction * CameraDistance - CameraMargin) * CamDirection.x() / 1.2,
		Cam1.y() + (CamCallback._hitfraction * CameraDistance - CameraMargin) * CamDirection.y() / 1.2,
		Cam1.z() + (CamCallback._hitfraction * CameraDistance - CameraMargin) * CamDirection.z() / 1.2);
	_Camera->setPosition(CameraPosition);

	return;
}

void Game::BulletCallback(btScalar timeStep)
{
	_Player->UpdatePhysics(timeStep);

	for(auto & cc : _Enemies)
	{
		cc->UpdateAITarget(_Player->GetPosition(), _Env, 3);
		cc->UpdateAI(timeStep);
		//cc->UpdateAI(timeStep, _Player->GetPosition(), _Env, 3);

		cc->UpdatePhysics(timeStep);
	}
}

std::shared_ptr<CharacterController> Game::CreateCharacter(std::string MeshName, float Height, float mass, btVector3& position, float heading)
{
	return std::shared_ptr<CharacterController>(new CharacterController(_SceneMgr, _World, MeshName, Height, mass, position, heading));
}

void Game::go(void)
{
	//_SceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);

	const std::string level(AppStateManager::GetResourcesDir()+"/levels/level1");
	AppStateManager::AddResourceDirectory(level);
	std::fstream f((level+"/level.txt").c_str(), std::fstream::in);
	if (f.is_open())
	{
		_Env = std::shared_ptr<Environment>(new Environment(_SceneMgr, *_World, f));
	}

	btVector3 PlayerPosition(0, 10, 0);
	_Player = std::shared_ptr<CharacterController>(new CharacterController(_SceneMgr, _World, "Sinbad.mesh", 1.8, 100, PlayerPosition, 0));

	_Camera->setOrientation(Ogre::Quaternion(_Pitch, Ogre::Vector3::UNIT_X));
	_Camera->setPosition(0, CameraHeight - CameraDistance * sin(_Pitch.valueRadians()), CameraDistance * cos(_Pitch.valueRadians()));

	_Camera->setNearClipDistance(0.01);

	Ogre::Light* pointLight = _SceneMgr->createLight();
	pointLight->setType(Ogre::Light::LT_POINT);
	pointLight->setPosition(Ogre::Vector3(15, 10, 15));
	pointLight->setDiffuseColour(0.5,0.5,0.5);
	pointLight->setSpecularColour(0.5,0.5,0.5);

	Ogre::Light* dirLight = _SceneMgr->createLight();
	dirLight->setType(Ogre::Light::LT_DIRECTIONAL);
	dirLight->setDirection(Ogre::Vector3(-1, -1, -1));
	dirLight->setDiffuseColour(0.5,0.5,0.5);
	dirLight->setSpecularColour(0.5,0.5,0.5);

	_SceneMgr->setAmbientLight(Ogre::ColourValue(0.05, 0.05, 0.05));

	for(float x = 0; x < 8; x += 1)
	{
		btVector3 pos(x, 10, -3);
		_Enemies.push_back(std::shared_ptr<CharacterController>(new CharacterController(_SceneMgr, _World, "Pony.mesh", 1.2, 30, pos, 0)));
	}

	Ogre::LogManager::getSingleton().logMessage("Game started");
}
