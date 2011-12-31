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

const float CameraDistance = 0.8;
const float CameraHeight = 1.8;

namespace pmd
{
	bool Game::mouseMoved(const OIS::MouseEvent &arg)
	{
		_Pitch -= Ogre::Radian(arg.state.Y.rel * 0.003);
		if (_Pitch.valueRadians() > M_PI / 2)
			_Pitch = Ogre::Radian(M_PI / 2);
		else if (_Pitch.valueRadians() < -M_PI / 2)
			_Pitch = Ogre::Radian(-M_PI / 2);

		_Heading -= arg.state.X.rel * 0.003;

		_Camera->setOrientation(Ogre::Quaternion(_Pitch, Ogre::Vector3::UNIT_X));
		_Camera->setPosition(0, CameraHeight - CameraDistance * sin(_Pitch.valueRadians()), CameraDistance * cos(_Pitch.valueRadians()));
		
		return true;
	}

	bool Game::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
	{
		return true;
	}

	bool Game::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
	{
		return true;
	}

	bool Game::keyPressed(const OIS::KeyEvent &arg)
	{
		if (arg.key == OIS::KC_ESCAPE)
			_Shutdown = true;
		return true;
	}

	bool Game::keyReleased(const OIS::KeyEvent &arg)
	{
		return true;
	}

	bool Game::frameRenderingQueued(const Ogre::FrameEvent& evt)
	{
		if (_Window->isClosed())
			return false;

		if (_Shutdown)
			return false;

		float velX = 0, velZ = 0;
		float dt = evt.timeSinceLastFrame;
		
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
		
		btMatrix3x3 orientation(_PlayerBody->getOrientation());
		btVector3 vel = orientation * btVector3(velX, 0, velZ);
		btVector3 cur_v = _PlayerBody->getVelocityInLocalPoint(btVector3(0,0,0));
		btVector3 F = 800 * (vel - cur_v);
		F.setY(0);
		
		btQuaternion q = _PlayerBody->getOrientation();
		btQuaternion q_target(btVector3(0,1,0), _Heading);

		float Iyy = 10.3;
		float freq = 5;
		float w = freq * 2 * M_PI;
		float ksi = 0.7;

		float Kp = Iyy * w * w;
		float Kd = Iyy * 2 * ksi * w;
		q = q_target * q.inverse();
		if (q.getW() < 0) q = -q;

		btVector3 T = Kp * q.getAxis() * q.getAngle() - Kd * _PlayerBody->getAngularVelocity();

		_PlayerBody->activate(true);
		_PlayerBody->applyTorque(T);
		_PlayerBody->applyCentralForce(F);

		_World->stepSimulation(evt.timeSinceLastFrame, 10);

		//Need to capture/update each device
		_Keyboard->capture();
		_Mouse->capture();

		return true;
	}

	void DumpSkeleton(std::stringstream &ss, Ogre::SkeletonInstance * s, int level)
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
	}

	void Game::go(void)
	{
		if (!setup())
			return;

		//_SceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_MODULATIVE); // ombre que sur les cubes ??

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		std::fstream f("../../../default_level.txt", std::fstream::in);
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		std::fstream f("../../default_level.txt", std::fstream::in);
#else
#error Unsupported platform
#endif
		if (!f.is_open()) throw new std::invalid_argument("file not found");

		Environment env(_SceneMgr, f);

		Ogre::Entity * PlayerEntity = _SceneMgr->createEntity("player", "player.mesh");

		PlayerEntity->setCastShadows(true);
		_Player = _SceneMgr->getRootSceneNode()->createChildSceneNode("player");
		_Player->attachObject(PlayerEntity);
		_Player->attachObject(_Camera);

		btCollisionShape * PlayerShape = new btCapsuleShape(0.4, 1.9-2*0.4);
		btVector3 PlayerInertia;
		PlayerShape->calculateLocalInertia(80, PlayerInertia);
		PlayerInertia.setX(0);
		PlayerInertia.setZ(0);

		_PlayerBody = new btRigidBody(
			80,
			new RigidBody<Ogre::SceneNode>(
			Ogre::Quaternion::IDENTITY,
				Ogre::Vector3(0, 0, 0),
				Ogre::Vector3(0, 0.95, 0),
				_Player),
			PlayerShape,
			PlayerInertia);
		_World->addRigidBody(_PlayerBody);

		_Camera->setOrientation(Ogre::Quaternion(_Pitch, Ogre::Vector3::UNIT_X));
		_Camera->setPosition(0, CameraHeight - CameraDistance * sin(_Pitch.valueRadians()), CameraDistance * cos(_Pitch.valueRadians()));

		_Camera->setNearClipDistance(0.01);

		Ogre::Light* pointLight = _SceneMgr->createLight();
		pointLight->setType(Ogre::Light::LT_POINT);
		pointLight->setPosition(Ogre::Vector3(15, 10, 15));
		pointLight->setDiffuseColour(1,1,1);
		pointLight->setSpecularColour(1,1,1);
		
		_SceneMgr->setAmbientLight(Ogre::ColourValue(0.05, 0.05, 0.05));

		Ogre::Plane plane(Ogre::Vector3::UNIT_Y, 0);
		Ogre::MeshManager::getSingleton().createPlane(
			"ground",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			plane,
			240, 240, 200, 200, true, 1, 5, 5, Ogre::Vector3::UNIT_Z);
 
		Ogre::Entity* entGround = _SceneMgr->createEntity("GroundEntity", "ground");
		entGround->setCastShadows(false);
		entGround->setMaterialName("Examples/Rockwall");
		_SceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(entGround);

		
		btRigidBody * btGround = new btRigidBody(
			0,
			new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, -10, 0))),
			new btBoxShape(btVector3(120, 10, 120)));
		_World->addRigidBody(btGround);

		for(float y=0; y < 1000; y += 1)
		{
			for (float x = -1; x < 1; x += 1000.6)
			{
				for (float z = -11; z < -9; z += 1000.6)
				{
					Ogre::Entity * entCube = _SceneMgr->createEntity("Prefab_Cube");
					Ogre::SceneNode * node = _SceneMgr->getRootSceneNode()->createChildSceneNode();
					node->attachObject(entCube);
					node->setScale(0.005, 0.005, 0.005);
					node->setPosition(1, 3, 1);
					btCollisionShape * btCubeShape = new btBoxShape(btVector3(0.25, 0.25, 0.25));
					//btCollisionShape * btCubeShape = new btSphereShape(0.25*sqrt(3.0f));
					btVector3 inertia;
					btCubeShape->calculateLocalInertia(100, inertia);
					btRigidBody * btCube = new btRigidBody(
						100,
						new RigidBody<Ogre::SceneNode>(
							Ogre::Quaternion::IDENTITY,
							Ogre::Vector3(x, y, z),
							Ogre::Vector3(0, 0, 0),
							node),
						btCubeShape,
						inertia);
					_World->addRigidBody(btCube);
				}
			}
		}

		Ogre::LogManager::getSingleton().logMessage(
			Ogre::LML_NORMAL,
			DumpNodes(_SceneMgr->getRootSceneNode()).c_str());

		_Root->startRendering();

		cleanup();
	}
}
