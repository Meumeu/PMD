/*
    Copyright (C) 2011-2012  Patrick Nicolas <patricknicolas@laposte.net>

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
#include "environment.h"

#include <boost/date_time.hpp>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreStaticGeometry.h>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "OgreConverter.h"
#include "OgreMeshManager.h"
#include "bullet/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "bullet/BulletCollision/CollisionShapes/btTriangleMesh.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "bullet/btBulletCollisionCommon.h"
#include "bullet/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "bullet/BulletDynamics/Dynamics/btRigidBody.h"

#include "Pathfinding/Pathfinding.h"

#include "DebugDrawer.h"

static bool CustomMaterialCombinerCallback(
	btManifoldPoint& cp,
	const btCollisionObject* colObj0,
	int /* partId0 */,
	int /* index0 */,
	const btCollisionObject* colObj1,
	int partId1,
	int index1)
{
	btAdjustInternalEdgeContacts(cp,colObj1,colObj0, partId1,index1);
	return false;
}

static Ogre::Quaternion getQuaternion(Environment::orientation_t orientation)
{
	switch (orientation)
	{
		case Environment::North:
			return Ogre::Quaternion::IDENTITY;
		case Environment::East:
			return Ogre::Quaternion(Ogre::Radian(-M_PI/2),Ogre::Vector3::UNIT_Y);
		case Environment::South:
			return Ogre::Quaternion(Ogre::Radian(M_PI)   ,Ogre::Vector3::UNIT_Y);
		case Environment::West:
			return Ogre::Quaternion(Ogre::Radian(M_PI/2) ,Ogre::Vector3::UNIT_Y);
	}

	throw std::runtime_error("Invalid Environment::orientation_t");
}
static Ogre::Matrix4 getMatrix4(Environment::orientation_t orientation, Ogre::Vector3 translation)
{
	Ogre::Matrix4 matrix(getQuaternion(orientation));
	matrix.setTrans(translation);
	return matrix;
}

Environment::Environment ( Ogre::SceneManager* sceneManager, btDynamicsWorld& world, std::istream& level) : _sceneManager(sceneManager), _world(world)
{
	new DebugDrawer(_sceneManager, 0.5);

	boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();
	while (!level.eof())
	{
		std::string MeshName;
		std::string Orientation;
		float x, y, z;

		level >> MeshName >> x >> y >> z >> Orientation;

		if (MeshName.compare("") && (MeshName[0] != '#'))
		{
			std::cout << "mesh " << MeshName << " at (" << x << "," << y << "," << z << "), " << Orientation << std::endl;
			orientation_t o;
			if (!Orientation.compare("N"))
				o = North;
			else if (!Orientation.compare("S"))
				o = South;
			else if (!Orientation.compare("E"))
				o = East;
			else if (!Orientation.compare("W"))
				o = West;
			else
			{
				std::stringstream str;
				str << "Invalid orientation (" << Orientation << ")";
				throw std::invalid_argument(str.str());
			}

			Ogre::Entity * Entity = _sceneManager->createEntity(MeshName);
			Entity->setCastShadows(false);
			_blocks.push_back(Block(Entity, o, Ogre::Vector3(x,y,z)));
		}
	}

	boost::posix_time::ptime t2 = boost::posix_time::microsec_clock::universal_time();
	Ogre::StaticGeometry *sg = _sceneManager->createStaticGeometry("environment");

	boost::posix_time::ptime t3 = boost::posix_time::microsec_clock::universal_time();


	_NavMesh.AgentHeight = 1.8;
	_NavMesh.AgentRadius = 0.5;
	_NavMesh.AgentMaxSlope = M_PI / 4;
	_NavMesh.AgentMaxClimb = 0.5;
	_NavMesh.CellHeight = 0.2;
	_NavMesh.CellSize = 0.3;
	_NavMesh.QueryExtent = Ogre::Vector3(10, 10, 10);

	for(auto const & block : _blocks)
	{
		sg->addEntity(block._entity, block._position, getQuaternion(block._orientation));

		OgreConverter converter(*block._entity);
		Ogre::Matrix4 transform = getMatrix4(block._orientation, block._position);
		converter.AddToTriMesh(transform, _TriMesh);
		converter.AddToHeightField(transform, _NavMesh);
	}

	boost::posix_time::ptime t4= boost::posix_time::microsec_clock::universal_time();
	_NavMesh.Build();
	boost::posix_time::ptime t5 = boost::posix_time::microsec_clock::universal_time();

	//_NavMesh.DrawCompactHeightfield = true;
	//_NavMesh.DrawRawContours = true;
	//_NavMesh.DrawContours = true;
	_NavMesh.DrawPolyMeshDetail = true;

	_NavMesh.DebugDraw();
	DebugDrawer::getSingleton().build();
	boost::posix_time::ptime t6 = boost::posix_time::microsec_clock::universal_time();

	_TriMeshShape = std::shared_ptr<btBvhTriangleMeshShape>(new btBvhTriangleMeshShape(&_TriMesh, true));
	boost::posix_time::ptime t7 = boost::posix_time::microsec_clock::universal_time();

	btRigidBody::btRigidBodyConstructionInfo rbci(0, 0, _TriMeshShape.get());
	_EnvBody = std::shared_ptr<btRigidBody>(new btRigidBody(rbci));
	boost::posix_time::ptime t8 = boost::posix_time::microsec_clock::universal_time();

	_world.addRigidBody(_EnvBody.get());
	boost::posix_time::ptime t9 = boost::posix_time::microsec_clock::universal_time();

	btTriangleInfoMap * triinfomap = new btTriangleInfoMap();
	btGenerateInternalEdgeInfo(_TriMeshShape.get(), triinfomap);
	gContactAddedCallback = CustomMaterialCombinerCallback;
	_EnvBody->setCollisionFlags(_EnvBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK | btCollisionObject::CF_STATIC_OBJECT);
	_EnvBody->setContactProcessingThreshold(0);
	boost::posix_time::ptime t10 = boost::posix_time::microsec_clock::universal_time();

	sg->build();
	//sg->setCastShadows(true);
	boost::posix_time::ptime t11 = boost::posix_time::microsec_clock::universal_time();

	for(auto const & block : _blocks)
	{
		_sceneManager->destroyEntity(block._entity);
	}
	boost::posix_time::ptime t12 = boost::posix_time::microsec_clock::universal_time();

	std::stringstream str;

	str << "Read level file: .  .  .  .  .  .  .  .  .  .  " << t2 - t1;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create static geometry:.  .  .  .  .  .  .  .  " << t3 - t2;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Convert geometry to Bullet and Recast:.  .  .  " << t4 - t3;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Build NavMesh:.  .  .  .  .  .  .  .  .  .  .  " << t5 - t4;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create debug drawer:.  .  .  .  .  .  .  .  .  " << t6 - t5;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create triangle mesh shape:  .  .  .  .  .  .  " << t7 - t6;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create rigid body:  .  .  .  .  .  .  .  .  .  " << t8 - t7;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Add rigid body : .  .  .  .  .  .  .  .  .  .  " << t9 - t8;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Add material callback: .  .  .  .  .  .  .  .  " << t10 - t9;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Build static geometry: .  .  .  .  .  .  .  .  " << t11 - t10;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Clean up temporary variables:.  .  .  .  .  .  " << t12 - t11;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Total: . .  .  . .  .  .  .  .  .  .  .  .  .  " << t12 - t1;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");
}

Environment::~Environment()
{
	delete DebugDrawer::getSingletonPtr();
	_world.removeRigidBody(_EnvBody.get());
}
