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

#include <stdio.h>
#include <OgreEntity.h>
#include <OgreMeshManager.h>
#include <boost/foreach.hpp>

#include <btBulletDynamicsCommon.h>
#include "AppStateManager.h"

const float CameraDistance = 2;
const float CameraHeight = 1.7;


bool Game::keyPressed(const OIS::KeyEvent& e)
{
	if (e.key == OIS::KC_ESCAPE)
	{
		_EscPressed = true;
	}

	if (e.key == OIS::KC_SPACE)
	{
		_Player->_Jump = _Player->_GroundContact;
	}
	
	return true;
}

void Game::Update(float TimeSinceLastFrame)
{
	if (_Window->isClosed()) return;

	if (_EscPressed)
	{
		AppStateManager::GetSingleton().Exit();
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

	btVector3 TargetVelocity = btVector3(
		velX * cos(_Heading.valueRadians()) + velZ * sin(_Heading.valueRadians()),
		0,
		-velX * sin(_Heading.valueRadians()) + velZ * cos(_Heading.valueRadians()));

	if (TargetVelocity != btVector3(0,0,0))
		TargetVelocity.normalize();

	if (_Keyboard->isKeyDown(OIS::KC_LSHIFT))
	{
		_Player->_TargetVelocity = 10 * TargetVelocity;
	}
	else
	{
		_Player->_TargetVelocity = 3 * TargetVelocity;
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

	_Camera->setPosition(
		_Player->_Node->getPosition() +
		Ogre::Vector3(
			CameraDistance * cos(_Pitch.valueRadians()) * sin(_Heading.valueRadians()),
			CameraHeight - CameraDistance * sin(_Pitch.valueRadians()),
			CameraDistance * cos(_Pitch.valueRadians()) * cos(_Heading.valueRadians())));

	_Player->UpdateGraphics(TimeSinceLastFrame);
	BOOST_FOREACH(CharacterController * cc, _Ennemies)
	{
		cc->UpdateGraphics(TimeSinceLastFrame);
	}
	
	return;
}

void Game::BulletCallback(btScalar timeStep)
{
	_Player->UpdatePhysics(timeStep);
	BOOST_FOREACH(CharacterController * cc, _Ennemies)
	{
		btVector3 target = _Player->_Body->getCenterOfMassPosition() - cc->_Body->getCenterOfMassPosition();

		if (target.length2() > 50)
		{
			float theta = atan2(cc->_TargetVelocity.z(), cc->_TargetVelocity.x());
			theta += (rand() % 100 - 50) * 0.001;

			target.setX(cos(theta));
			target.setY(0);
			target.setZ(sin(theta));
			cc->_TargetVelocity = 2 * target;
		}
		else
		{
			target.setY(0);
			cc->_TargetVelocity = 5 * target.normalized();
		}
		
		cc->UpdatePhysics(timeStep);
	}
}

CharacterController* Game::CreateCharacter(std::string MeshName, float height)
{
	Ogre::Entity * ent = _SceneMgr->createEntity(MeshName);
	Ogre::AxisAlignedBox box = ent->getBoundingBox();
	float scale = height / (box.getMaximum().y - box.getMinimum().y);
	
	Ogre::SceneNode * node = _SceneMgr->getRootSceneNode()->createChildSceneNode();
	Ogre::SceneNode * entnode = node->createChildSceneNode(Ogre::Vector3(0, -box.getMinimum().y * scale, 0));
	entnode->scale(scale, scale, scale);
	entnode->attachObject(ent);
	
	return new CharacterController(_SceneMgr, _World, ent, node, height, 0.4, 80);
}

void Game::go(void)
{
	_SceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	std::fstream f("../../../default_level.txt", std::fstream::in);
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	std::fstream f("../default_level.txt", std::fstream::in);
#else
#error Unsupported platform
#endif
	if (!f.is_open()) throw Ogre::Exception(Ogre::Exception::ERR_FILE_NOT_FOUND, "File not found", __FUNCTION__, "File not found", __FILE__, __LINE__);

	Environment env(_SceneMgr, f);

	_Player = CreateCharacter("Sinbad.mesh", 1.8);
	

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

	Ogre::Plane plane(Ogre::Vector3::UNIT_Y, 0);
	Ogre::MeshManager::getSingleton().createPlane(
		"ground",
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		plane,
		240, 240, 100, 100, true, 1, 5, 5, Ogre::Vector3::UNIT_Z);

	// ground
	Ogre::Entity* entGround = _SceneMgr->createEntity("GroundEntity", "ground");
	entGround->setCastShadows(false);
	entGround->setMaterialName("Examples/Rockwall");
	_SceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(entGround);

	btRigidBody * btGround = new btRigidBody(
		0,
		new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, -10, 0))),
		new btBoxShape(btVector3(120, 10, 120)));
	_World->addRigidBody(btGround);

	for(float x = -10; x < 10; x += 1)
	{
		//CharacterController * cc = CreateCharacter("Sinbad.mesh", 1.8);
		CharacterController * cc = CreateCharacter("Pony.mesh", 1.8);
		cc->_Body->translate(btVector3(x, 0, -10));
		
		_Ennemies.push_back(cc);
	}

	Ogre::LogManager::getSingleton().logMessage("Game started");
}
