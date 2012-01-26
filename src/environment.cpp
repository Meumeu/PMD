/*
    Copyright (C) 2011  Patrick Nicolas <patricknicolas@laposte.net>

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

#include <boost/foreach.hpp>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreStaticGeometry.h>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "OgreConverter.h"
#include "bullet/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "bullet/BulletCollision/CollisionShapes/btTriangleMesh.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "bullet/btBulletCollisionCommon.h"
#include "bullet/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "bullet/BulletDynamics/Dynamics/btRigidBody.h"
#include "Recast.h"

static bool CustomMaterialCombinerCallback(
	btManifoldPoint& cp,
	const btCollisionObject* colObj0,
	int partId0,
	int index0,
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
	Ogre::AxisAlignedBox boundingBox;
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
			Entity->setCastShadows(true);
			_blocks.push_back(Block(Entity, o, Ogre::Vector3(x,y,z)));
			Ogre::Matrix4 transform = getMatrix4(o, Ogre::Vector3(x,y,z));

			const Ogre::Vector3 * corners = Entity->getBoundingBox().getAllCorners();
			for(int i = 0; i < 8; i++)
				boundingBox.merge(transform * corners[i]);
		}
	}

	Ogre::StaticGeometry *sg = _sceneManager->createStaticGeometry("environment");

	Recast::rcHeightfield heightfield;
	Recast::rcContext context(false);
	float bboxMin[3], bboxMax[3];
	OgreConverter::Vector3ToFloatArray(boundingBox.getMinimum(), bboxMin);
	OgreConverter::Vector3ToFloatArray(boundingBox.getMaximum(), bboxMax);
	float cellSize = 0.3f, cellHeight = 0.2f; //FIXME: value tweaking ?
	int width, height;
	Recast::rcCalcGridSize(bboxMin, bboxMax, cellSize, &width, &height);
	Recast::rcCreateHeightfield(&context, heightfield, width, height, bboxMin, bboxMax, cellSize, cellHeight);

	BOOST_FOREACH(Block const& block, _blocks)
	{
		block._entity->setCastShadows(false);
		sg->addEntity(block._entity, block._position, getQuaternion(block._orientation));

		OgreConverter converter(*block._entity);
		Ogre::Matrix4 transform = getMatrix4(block._orientation, block._position);
		converter.AddToTriMesh(transform, _TriMesh);
		converter.AddToHeightField(transform, heightfield, 0, 1);//FIXME: adjust values
	}

	_TriMeshShape = boost::shared_ptr<btBvhTriangleMeshShape>(new btBvhTriangleMeshShape(&_TriMesh, true));

	btRigidBody::btRigidBodyConstructionInfo rbci(0, 0, _TriMeshShape.get());
	_EnvBody = boost::shared_ptr<btRigidBody>(new btRigidBody(rbci));

	_world.addRigidBody(_EnvBody.get());

	btTriangleInfoMap * triinfomap = new btTriangleInfoMap();
	btGenerateInternalEdgeInfo(_TriMeshShape.get(), triinfomap);
	gContactAddedCallback = CustomMaterialCombinerCallback;
	_EnvBody->setCollisionFlags(_EnvBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK | btCollisionObject::CF_STATIC_OBJECT);
	_EnvBody->setContactProcessingThreshold(0);


	sg->build();
	sg->setCastShadows(true);
	
	BOOST_FOREACH(Block const& block, _blocks)
	{
		_sceneManager->destroyEntity(block._entity);
	}
}

Environment::~Environment()
{
	_world.removeRigidBody(_EnvBody.get());
}
