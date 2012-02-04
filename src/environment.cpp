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

#include <boost/foreach.hpp>
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

#include "Recast/Recast.h"
#include "Recast/RecastHeightfield.h"
#include "Recast/RecastCompactHeightfield.h"
#include "Recast/RecastContourSet.h"

#include "DebugDrawer.h"

#define DEBUG_RECAST 4

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
	new DebugDrawer(_sceneManager, 0.5);
	
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

	Ogre::StaticGeometry *sg = _sceneManager->createStaticGeometry("environment");

	Recast::Heightfield heightfield(0.2f, 0.01f, M_PI/4, 1); //FIXME: adjust values

	BOOST_FOREACH(Block const& block, _blocks)
	{
		//sg->addEntity(block._entity, block._position, getQuaternion(block._orientation));

		OgreConverter converter(*block._entity);
		Ogre::Matrix4 transform = getMatrix4(block._orientation, block._position);
		converter.AddToTriMesh(transform, _TriMesh);
		converter.AddToHeightField(transform, heightfield);
	}
	
	Recast::CompactHeightfield chf(2.0, 0.9, heightfield, 0.6, true, true); //FIXME: adjust values
	chf.buildRegions(8, 20); //FIXME: adjust values
	Recast::ContourSet cs(chf, 1.3, 12, Recast::RC_CONTOUR_TESS_WALL_EDGES | Recast::RC_CONTOUR_TESS_AREA_EDGES);
	
	Ogre::Plane plane(Ogre::Vector3::UNIT_Y, 0);
	Ogre::MeshManager::getSingleton().createPlane(
		"chf",
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		plane,
		chf._cs, chf._cs, 1, 1, true, 1, 1, 1, Ogre::Vector3::UNIT_Z);
	
#if DEBUG_RECAST == 1 || DEBUG_RECAST == 2
	for(int x = 0; x < chf._xsize; ++x)
	{
		for(int z = 0; z < chf._zsize; ++z)
		{
			float x0 = (x + chf._xmin) * chf._cs;
			float z0 = (z + chf._zmin) * chf._cs;
#if DEBUG_RECAST == 1
			BOOST_FOREACH(Recast::CompactSpan& span, chf._cells[x + z * chf._xsize])
			{
				if (!span._walkable) continue;
				float y0 = span._bottom * chf._ch;
				Ogre::Vector3 v[8];
				v[0] = Ogre::Vector3(x0, y0+0.1, z0);
				v[1] = Ogre::Vector3(x0, y0+0.1, z0+chf._cs);
				v[2] = Ogre::Vector3(x0+chf._cs, y0+0.1, z0+chf._cs);
				v[3] = Ogre::Vector3(x0+chf._cs, y0+0.1, z0);
				v[4] = Ogre::Vector3(x0+chf._cs, y0+0.11, z0+chf._cs);
				v[5] = Ogre::Vector3(x0, y0+0.11, z0+chf._cs);
				v[6] = Ogre::Vector3(x0, y0+0.11, z0);
				v[7] = Ogre::Vector3(x0+chf._cs, y0+0.11, z0);
				unsigned int r = span._regionID;
				Ogre::ColourValue c(((r / 16) % 4) * 0.333, ((r / 4) % 4) * 0.333, (r % 4) * 0.333);
				
				DebugDrawer::getSingleton().drawCuboid(v, c, true);
			}
#else
#if DEBUG_RECAST == 2
			BOOST_FOREACH(Recast::Span const& span, heightfield.getSpans(x + chf._xmin, z + chf._zmin))
			{
				float y1 = span._smin * chf._ch;
				float y2 = span._smax * chf._ch;
				Ogre::Vector3 v[8];
				v[0] = Ogre::Vector3(x0, y1, z0);
				v[1] = Ogre::Vector3(x0, y1, z0+chf._cs);
				v[2] = Ogre::Vector3(x0+chf._cs, y1, z0+chf._cs);
				v[3] = Ogre::Vector3(x0+chf._cs, y1, z0);
				v[4] = Ogre::Vector3(x0+chf._cs, y2, z0+chf._cs);
				v[5] = Ogre::Vector3(x0, y2, z0+chf._cs);
				v[6] = Ogre::Vector3(x0, y2, z0);
				v[7] = Ogre::Vector3(x0+chf._cs, y2, z0);
				
				DebugDrawer::getSingleton().drawCuboid(v, Ogre::ColourValue::Blue, true);
			}
#endif
#endif
		}
	}
#endif

#if DEBUG_RECAST == 3
	BOOST_FOREACH(Recast::Contour const & cont, cs._conts)
	{
		size_t n = cont.rverts.size();
		for(size_t i = 0; i < n; ++i)
		{
			size_t j = (i + 1) % n;
			Ogre::Vector3 a((cont.rverts[i].x + chf._xmin) * chf._cs,
				         cont.rverts[i].y * chf._ch,
				        (cont.rverts[i].z + chf._zmin) * chf._cs);
			
			Ogre::Vector3 b((cont.rverts[j].x + chf._xmin) * chf._cs,
				         cont.rverts[j].y * chf._ch,
				        (cont.rverts[j].z + chf._zmin) * chf._cs);
			
			unsigned int r = cont.reg;
			Ogre::ColourValue c(((r / 16) % 4) * 0.333, ((r / 4) % 4) * 0.333, (r % 4) * 0.333);
			DebugDrawer::getSingleton().drawLine(a, b, c);
		}
	}
#endif

#if DEBUG_RECAST == 4
	BOOST_FOREACH(Recast::Contour const & cont, cs._conts)
	{
		size_t n = cont.verts.size();
		for(size_t i = 0; i < n; ++i)
		{
			size_t j = (i + 1) % n;
			Ogre::Vector3 a((cont.verts[i].x + chf._xmin) * chf._cs,
				         cont.verts[i].y * chf._ch,
				        (cont.verts[i].z + chf._zmin) * chf._cs);
			
			Ogre::Vector3 b((cont.verts[j].x + chf._xmin) * chf._cs,
				         cont.verts[j].y * chf._ch,
				        (cont.verts[j].z + chf._zmin) * chf._cs);
			
			unsigned int r = cont.reg;
			Ogre::ColourValue c(((r / 16) % 4) * 0.333, ((r / 4) % 4) * 0.333, (r % 4) * 0.333);
			DebugDrawer::getSingleton().drawLine(a, b, c);
		}
	}
#endif
	DebugDrawer::getSingleton().build();
	
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
	delete DebugDrawer::getSingletonPtr();
	_world.removeRigidBody(_EnvBody.get());
}
