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

#include "Recast/Recast.h"
#include "Recast/RecastHeightfield.h"
#include "Recast/RecastCompactHeightfield.h"
#include "Recast/RecastContourSet.h"
#include "Recast/RecastPolyMesh.h"

#include "DebugDrawer.h"

#define DEBUG_RECAST 0

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
	Recast::Heightfield heightfield(0.2f, 0.01f, M_PI/4, 1); //FIXME: adjust values

	boost::posix_time::ptime t4 = boost::posix_time::microsec_clock::universal_time();
	BOOST_FOREACH(Block const& block, _blocks)
	{
		//sg->addEntity(block._entity, block._position, getQuaternion(block._orientation));

		OgreConverter converter(*block._entity);
		Ogre::Matrix4 transform = getMatrix4(block._orientation, block._position);
		converter.AddToTriMesh(transform, _TriMesh);
		converter.AddToHeightField(transform, heightfield);
	}
	
	boost::posix_time::ptime t5 = boost::posix_time::microsec_clock::universal_time();
	Recast::CompactHeightfield chf(2.0, 0.9, heightfield, 0.6); //FIXME: adjust values
	boost::posix_time::ptime t6 = boost::posix_time::microsec_clock::universal_time();
	chf.buildRegions(8, 20); //FIXME: adjust values
	boost::posix_time::ptime t7 = boost::posix_time::microsec_clock::universal_time();
	Recast::ContourSet cs(chf, 1.3, 12, Recast::RC_CONTOUR_TESS_WALL_EDGES | Recast::RC_CONTOUR_TESS_AREA_EDGES);
	boost::posix_time::ptime t8 = boost::posix_time::microsec_clock::universal_time();
	Recast::PolyMesh pm(cs);
	boost::posix_time::ptime t9 = boost::posix_time::microsec_clock::universal_time();

#if DEBUG_RECAST == 1 || DEBUG_RECAST == 2
	for(int x = 0; x < chf._xsize; ++x)
	{
		for(int z = 0; z < chf._zsize; ++z)
		{
			float x0 = (x + chf._xmin) * chf._cs;
			float z0 = (z + chf._zmin) * chf._cs;
#if DEBUG_RECAST == 1
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
				
				DebugDrawer::getSingleton().drawCuboid(v, span._walkable ? Ogre::ColourValue::Blue : Ogre::ColourValue::Green, true);
			}
#endif
#if DEBUG_RECAST == 2
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

#if DEBUG_RECAST == 5
	BOOST_FOREACH(Recast::PolyMesh::Polygon const & p, pm.polys)
	{
		unsigned int r = p.regionID;
		Ogre::Vector3 v[3];
		for(int i = 0; i < 3; i++)
		{
			v[i].x = (p.vertices[i].x + chf._xmin) * chf._cs;
			v[i].y = p.vertices[i].y * chf._ch;
			v[i].z = (p.vertices[i].z + chf._zmin) * chf._cs;
		}
		
		Ogre::ColourValue c(((r / 16) % 4) * 0.333, ((r / 4) % 4) * 0.333, (r % 4) * 0.333);
		DebugDrawer::getSingleton().drawTri(v, c, true);
		
		Ogre::Vector3 centre = (v[0] + v[1] + v[2]) / 3;
		
		for(int i = 0; i < 3; i++)
		{
			if (p.neighbours[i] == -1) continue;
			Recast::PolyMesh::Polygon const & neighbour = pm.polys[p.neighbours[i]];
			
			Ogre::Vector3 ncentre(0, 0, 0);
			for(int i = 0; i < 3; i++)
			{
				ncentre.x += (neighbour.vertices[i].x + chf._xmin) * chf._cs;
				ncentre.y += neighbour.vertices[i].y * chf._ch;
				ncentre.z += (neighbour.vertices[i].z + chf._zmin) * chf._cs;
			}
			ncentre /= 3;
			
			DebugDrawer::getSingleton().drawLine(centre + Ogre::Vector3(0, 0.5, 0), ncentre + Ogre::Vector3(0, 0.5, 0), Ogre::ColourValue::White);
		}
	}
#endif
	DebugDrawer::getSingleton().build();
	boost::posix_time::ptime t10 = boost::posix_time::microsec_clock::universal_time();
	
	_TriMeshShape = boost::shared_ptr<btBvhTriangleMeshShape>(new btBvhTriangleMeshShape(&_TriMesh, true));
	boost::posix_time::ptime t11 = boost::posix_time::microsec_clock::universal_time();

	btRigidBody::btRigidBodyConstructionInfo rbci(0, 0, _TriMeshShape.get());
	_EnvBody = boost::shared_ptr<btRigidBody>(new btRigidBody(rbci));
	boost::posix_time::ptime t12 = boost::posix_time::microsec_clock::universal_time();

	_world.addRigidBody(_EnvBody.get());
	boost::posix_time::ptime t13 = boost::posix_time::microsec_clock::universal_time();

	btTriangleInfoMap * triinfomap = new btTriangleInfoMap();
	btGenerateInternalEdgeInfo(_TriMeshShape.get(), triinfomap);
	gContactAddedCallback = CustomMaterialCombinerCallback;
	_EnvBody->setCollisionFlags(_EnvBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK | btCollisionObject::CF_STATIC_OBJECT);
	_EnvBody->setContactProcessingThreshold(0);
	boost::posix_time::ptime t14 = boost::posix_time::microsec_clock::universal_time();


	sg->build();
	//sg->setCastShadows(true);
	boost::posix_time::ptime t15 = boost::posix_time::microsec_clock::universal_time();
	
	BOOST_FOREACH(Block const& block, _blocks)
	{
		_sceneManager->destroyEntity(block._entity);
	}
	boost::posix_time::ptime t16 = boost::posix_time::microsec_clock::universal_time();

	std::stringstream str;
	
	str << "Read level file: .  .  .  .  .  .  .  .  .  .  " << t2 - t1;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create static geometry:.  .  .  .  .  .  .  .  " << t3 - t2;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create heightfield: .  .  .  .  .  .  .  .  .  " << t4 - t3;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Convert to Bullet and Recast::Heightfield:  .  " << t5 - t4;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create CompactHeightfield:.  .  .  .  .  .  .  " << t6 - t5;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Build regions:.  .  .  .  .  .  .  .  .  .  .  " << t7 - t6;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create contours: .  .  .  .  .  .  .  .  .  .  " << t8 - t7;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create polymesh: .  .  .  .  .  .  .  .  .  .  " << t9 - t8;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create debug drawer:.  .  .  .  .  .  .  .  .  " << t10 - t9;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create triangle mesh shape:  .  .  .  .  .  .  " << t11- t10;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Create rigid body:  .  .  .  .  .  .  .  .  .  " << t12 - t11;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Add rigid body : .  .  .  .  .  .  .  .  .  .  " << t13 - t12;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Add material callback: .  .  .  .  .  .  .  .  " << t14 - t13;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Build static geometry: .  .  .  .  .  .  .  .  " << t15 - t14;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Clean up temporary variables:.  .  .  .  .  .  " << t16 - t15;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");

	str << "Total: . .  .  . .  .  .  .  .  .  .  .  .  .  " << t16 - t1;
	Ogre::LogManager::getSingleton().logMessage(str.str());
	str.str("");
}

Environment::~Environment()
{
	delete DebugDrawer::getSingletonPtr();
	_world.removeRigidBody(_EnvBody.get());
}
