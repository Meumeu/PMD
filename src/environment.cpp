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


#include "environment.h"
#include "pmd.h"

#include <boost/foreach.hpp>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreStaticGeometry.h>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "btOgre/BtOgreGP.h"

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
	while (!level.eof())
	{
		std::string MeshName;
		std::string Orientation;
		int x, y, z;

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
		}
	}

	Ogre::StaticGeometry *sg = _sceneManager->createStaticGeometry("environment");

	
	BOOST_FOREACH(Block const& block, _blocks)
	{
		block._entity->setCastShadows(false);
		sg->addEntity(block._entity, block._position, getQuaternion(block._orientation));
		BtOgre::StaticMeshToShapeConverter converter(block._entity, getMatrix4(block._orientation, block._position));

		BtItems btStuff(converter);
		_btItems.push_back(btStuff);
		_world.addRigidBody(btStuff.getBody());
	}
	sg->build();
	sg->setCastShadows(true);
	
	BOOST_FOREACH(Block const& block, _blocks)
	{
		_sceneManager->destroyEntity(block._entity);
	}
}

Environment::~Environment()
{
	BOOST_FOREACH(BtItems & btStuff, _btItems)
	{
		_world.removeRigidBody(btStuff.getBody());
	}

}

Environment::BtItems::BtItems(BtOgre::StaticMeshToShapeConverter& converter)
{
	_btShape = boost::shared_ptr<btCollisionShape>(converter.createTrimesh());
	_motionState = boost::shared_ptr<btDefaultMotionState>(new btDefaultMotionState());
	_body = boost::shared_ptr<btRigidBody>(new btRigidBody(0, _motionState.get(), _btShape.get()));
}

