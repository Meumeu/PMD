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

#include "pmd.h"
#include "Game.h"
#include "RigidBody.h"
#include "environment.h"

#include <stdio.h>
#include <OgreEntity.h>
#include <OgreMeshManager.h>

#include <btBulletDynamicsCommon.h>
#include "AppStateManager.h"

const float CameraDistance = 0.8;
const float CameraHeight = 1.8;

namespace pmd
{
void Game::Update(float TimeSinceLastFrame)
{
	if (_Window->isClosed()) return;

	if (_Keyboard->isKeyDown(OIS::KC_ESCAPE))
	{
		AppStateManager::GetSingleton().Exit();
		return;
	}

	float velX = 0, velZ = 0;
	
	if (_Keyboard->isKeyDown(OIS::KC_Z) || _Keyboard->isKeyDown(OIS::KC_W))
	{
		velZ = -3;
	}
	else if (_Keyboard->isKeyDown(OIS::KC_S))
	{
		velZ = 3;
	}

	if (_Keyboard->isKeyDown(OIS::KC_Q) || _Keyboard->isKeyDown(OIS::KC_A))
	{
		velX = -3;
	}
	else if (_Keyboard->isKeyDown(OIS::KC_D))
	{
		velX = 3;
	}
	
	if (_Keyboard->isKeyDown(OIS::KC_LSHIFT))
	{
		velX *= 3.5;
		velZ *= 3.5;
	}

	_Player->_TargetVelocity = btVector3(velX, 0, velZ);
	_Player->_TargetHeading = _Heading;
	_Player->_Jump = _Keyboard->isKeyDown(OIS::KC_SPACE);

	_Pitch -= Ogre::Radian(_Mouse->getMouseState().Y.rel * 0.003);
	if (_Pitch.valueRadians() > M_PI / 2)
		_Pitch = Ogre::Radian(M_PI / 2);
	else if (_Pitch.valueRadians() < -M_PI / 2)
		_Pitch = Ogre::Radian(-M_PI / 2);

	_Heading -= _Mouse->getMouseState().X.rel * 0.003;

	_Camera->setOrientation(Ogre::Quaternion(_Pitch, Ogre::Vector3::UNIT_X));
	_Camera->setPosition(0, CameraHeight - CameraDistance * sin(_Pitch.valueRadians()), CameraDistance * cos(_Pitch.valueRadians()));

	
	_World->stepSimulation(TimeSinceLastFrame, 3);

	return;
}

void Game::BulletCallback(btScalar timeStep)
{
	_Player->TickCallback();
}

/*void DumpSkeleton(std::stringstream &ss, Ogre::SkeletonInstance * s, int level)
{
	Ogre::Skeleton::BoneIterator bi = s->getBoneIterator();
	while (bi.hasMoreElements())
	{
		for(int i = 0; i < level + 2; i++)
		{
			ss << " ";
		}
		Ogre::Bone * b = bi.getNext();

		ss << b->getName() << " " << b->getPosition() << std::endl;
	}
}

void DumpNodes(std::stringstream &ss, Ogre::Node *n, int level)
{
	for(int i = 0; i < level; i++)
	{
		ss << " ";
	}
	ss << "SceneNode: " << n->getName() << " at " << n->getPosition() << std::endl;
	
	Ogre::SceneNode::ObjectIterator object_it = ((Ogre::SceneNode *)n)->getAttachedObjectIterator();
	Ogre::Node::ChildNodeIterator node_it = n->getChildIterator();

	Ogre::MovableObject *m;
	while(object_it.hasMoreElements())
	{
		for(int i = 0; i < level + 2; i++)
		{
			ss << " ";
		}
		m = object_it.getNext();
		ss << m->getMovableType() << ": " << m->getName() << std::endl;
		if (!strcmp(m->getMovableType().c_str(), "Entity"))
		{
			Ogre::Entity * e = (Ogre::Entity *)m;
			Ogre::SkeletonInstance * s = e->getSkeleton();
			if (s)
			{
				DumpSkeleton(ss, s, level);
			}
			Ogre::AnimationStateSet * anim = e->getAllAnimationStates();
			if (anim)
			{
				Ogre::AnimationStateIterator anim_it = anim->getAnimationStateIterator();
				while (anim_it.hasMoreElements())
				{
					Ogre::AnimationState * st = anim_it.getNext();
					for(int i = 0; i < level + 2; i++)
					{
						ss << " ";
					}
					ss << "Animation: " << st->getAnimationName() << std::endl;
				}
			}
		}
	}
	
	while(node_it.hasMoreElements())
	{
		DumpNodes(ss, node_it.getNext(), level + 2);
	}
}

std::string DumpNodes(Ogre::Node *n)
{
	std::stringstream ss;
	ss << std::endl << "Node Hierarchy:" << std::endl;
	DumpNodes(ss, n, 0);
	return ss.str();
}*/

void Game::go(void)
{
	//_SceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_MODULATIVE); // ombre que sur les cubes ??

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	std::fstream f("../../../default_level.txt", std::fstream::in);
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	std::fstream f("../default_level.txt", std::fstream::in);
#else
#error Unsupported platform
#endif
	if (!f.is_open()) throw new std::invalid_argument("file not found");

	Environment env(_SceneMgr, f);

	Ogre::Entity * PlayerEntity = _SceneMgr->createEntity("player", "player.mesh");

	PlayerEntity->setCastShadows(true);

	_Player = new CharacterController(_SceneMgr, _World, PlayerEntity, 1.9, 0.4, 80);
	_Player->_Node->attachObject(_Camera);

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
		240, 240, 200, 200, true, 1, 5, 5, Ogre::Vector3::UNIT_Z);


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

	// slope test
	Ogre::Entity * entSlope = _SceneMgr->createEntity("Prefab_Cube");
	entSlope->setMaterialName("Examples/Rockwall");
	Ogre::SceneNode * nodeSlope = _SceneMgr->getRootSceneNode()->createChildSceneNode();
	nodeSlope->attachObject(entSlope);
	nodeSlope->setScale(1, 1, 1);
	nodeSlope->setPosition(170, 0, 0);
	nodeSlope->setOrientation(Ogre::Quaternion(Ogre::Degree(30), Ogre::Vector3::UNIT_X));
	btCollisionShape * SlopeShape = new btBoxShape(btVector3(50, 50, 50));
	btRigidBody * SlopeBody = new btRigidBody(
		0,
		new btDefaultMotionState(btTransform(btQuaternion(btVector3(1,0,0), M_PI/6), btVector3(170,0,0))),
		SlopeShape);
	SlopeBody->setFriction(1);
	_World->addRigidBody(SlopeBody);
	_Player->_Body->setFriction(1);

	

	for(float y=0; y < 5; y += 0.51)
	{
		for (float x = -(4.5-y)*0.6; x < (4.5-y)*0.6+0.1; x += 0.6)
		{
			for (float z = -(4.5-y)*0.6-20; z < (4.5-y)*0.6+0.1-20; z += 0.6)
			{
				Ogre::Entity * entCube = _SceneMgr->createEntity("Prefab_Cube");
				Ogre::SceneNode * node = _SceneMgr->getRootSceneNode()->createChildSceneNode();
				node->attachObject(entCube);
				node->setScale(0.005, 0.005, 0.005);
				btCollisionShape * btCubeShape = new btBoxShape(btVector3(0.25, 0.25, 0.25));
				//btCollisionShape * btCubeShape = new btSphereShape(0.25*sqrt(3.0f));
				btVector3 inertia;
				btCubeShape->calculateLocalInertia(1, inertia);
				btRigidBody * btCube = new btRigidBody(
					1,
					new RigidBody<Ogre::SceneNode>(
						Ogre::Quaternion::IDENTITY,
						Ogre::Vector3(x, y+0.25, z),
						Ogre::Vector3(0, 0, 0),
						node),
					btCubeShape,
					inertia);
				_World->addRigidBody(btCube);
			}
		}
	}

	for(float x = 0; x < 3; x += 1)
	{
		for(float z = 10; z < 13; z += 1)
		{
			Ogre::Entity * e = _SceneMgr->createEntity("robot.mesh");
			Ogre::SceneNode * sn = _SceneMgr->getRootSceneNode()->createChildSceneNode();
			sn->attachObject(e);
			sn->setScale(0.01, 0.01, 0.01);
			btCollisionShape * shape = new btBoxShape(btVector3(0.15, 0.5, 0.15));
			btVector3 I;
			shape->calculateLocalInertia(5, I);
			btRigidBody * body = new btRigidBody(
				5,
				new RigidBody<Ogre::SceneNode>(
				Ogre::Quaternion::IDENTITY,
				Ogre::Vector3(x, 0, z),
				Ogre::Vector3(0, 0.25, 0),
				sn),
				shape,
				I);
			_World->addRigidBody(body);
		}
	}

	Ogre::LogManager::getSingleton().logMessage("Game started");
}
}
