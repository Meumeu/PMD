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

namespace pmd
{

Environment::Environment ( Ogre::SceneManager* sceneManager ) : _sceneManager(sceneManager)
{
	Ogre::Entity * block1Entity = _sceneManager->createEntity("block1","robot.mesh");
	_blocks.push_back(Block(block1Entity, North, Ogre::Vector3(0,0,0)));
	_blocks.push_back(Block(block1Entity->clone("block2"), North, Ogre::Vector3(1,0,0)));
	_blocks.push_back(Block(block1Entity->clone("block3"), North, Ogre::Vector3(0,0,1)));
	_blocks.push_back(Block(block1Entity->clone("block4"), North, Ogre::Vector3(1,0,1)));

	_blocks.push_back(Block(block1Entity->clone("block5"), East,  Ogre::Vector3(2,0,0)));
	_blocks.push_back(Block(block1Entity->clone("block6"), West,  Ogre::Vector3(0,0,2)));
	_blocks.push_back(Block(block1Entity->clone("block7"), South, Ogre::Vector3(2,0,1)));

	Ogre::StaticGeometry *sg = _sceneManager->createStaticGeometry("environment");

	//Rotation constants
	Ogre::Quaternion
		eastRotation  = Ogre::Quaternion(Ogre::Radian(-M_PI/2),Ogre::Vector3::UNIT_Y),
		southRotation = Ogre::Quaternion(Ogre::Radian(M_PI)   ,Ogre::Vector3::UNIT_Y),
		westRotation  = Ogre::Quaternion(Ogre::Radian(M_PI/2) ,Ogre::Vector3::UNIT_Y);
	
	BOOST_FOREACH(Block const& block, _blocks)
	{
		std::stringstream log;
		log << "Adding " << block._entity->getName() << " at (" << block._position.x << ", " << block._position.y << ") " << 
		(block._orientation == North ? "N" : block._orientation == East ? "E" : block._orientation == South ? "S" : "W");
		
		Ogre::LogManager::getSingletonPtr()->logMessage(log.str());

		switch (block._orientation)
		{
			case North:
				sg->addEntity(block._entity, block._position, Ogre::Quaternion::IDENTITY, Ogre::Vector3(5, 5, 5));
				break;
			case East:
				sg->addEntity(block._entity, block._position, eastRotation, Ogre::Vector3(5, 5, 5));
				break;
			case South:
				sg->addEntity(block._entity, block._position, southRotation, Ogre::Vector3(5, 5, 5));
				break;
			case West:
				sg->addEntity(block._entity, block._position, westRotation, Ogre::Vector3(5, 5, 5));
		}
	}
	sg->build();
}

Environment::~Environment()
{
	BOOST_FOREACH(Block const& block, _blocks)
	{
		_sceneManager->destroyEntity(block._entity);
	}

}

}