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
//#include <iosfwd>

namespace pmd
{

Environment::Environment (Ogre::SceneManager* sceneManager, std::string level) : _sceneManager(sceneManager)
{
	std::cout << "level=" << level << std::endl;
	if (!level.compare(""))
	{
		Ogre::Entity * block1Entity = _sceneManager->createEntity("block1","robot.mesh");
		_blocks.push_back(Block(block1Entity, North, Ogre::Vector3(0,0,0)));
		_blocks.push_back(Block(block1Entity->clone("block2"), North, Ogre::Vector3(1,0,0)));
		_blocks.push_back(Block(block1Entity->clone("block3"), North, Ogre::Vector3(0,0,1)));
		_blocks.push_back(Block(block1Entity->clone("block4"), North, Ogre::Vector3(1,0,1)));

		_blocks.push_back(Block(block1Entity->clone("block5"), East,  Ogre::Vector3(2,0,0)));
		_blocks.push_back(Block(block1Entity->clone("block6"), West,  Ogre::Vector3(0,0,2)));
		_blocks.push_back(Block(block1Entity->clone("block7"), South, Ogre::Vector3(2,0,1)));
	}
	else
	{
		std::fstream f(level.c_str(), std::fstream::in);
		while (!f.eof())
		{
			std::string MeshName;
			std::string Orientation;
			int x, y, z;
			f >> MeshName >> x >> y >> z >> Orientation;
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
					std::cerr << "invalid orientation '" << Orientation << "'" << std::endl;
					//exit(1);
				}

				Ogre::Entity * Entity = _sceneManager->createEntity(MeshName);
				_blocks.push_back(Block(Entity, o, Ogre::Vector3(x,y,z)));
			}
		}
	}

	Ogre::StaticGeometry *sg = _sceneManager->createStaticGeometry("environment");

	//Rotation constants
	Ogre::Quaternion
		eastRotation  = Ogre::Quaternion(Ogre::Radian(-M_PI/2),Ogre::Vector3::UNIT_Y),
		southRotation = Ogre::Quaternion(Ogre::Radian(M_PI)   ,Ogre::Vector3::UNIT_Y),
		westRotation  = Ogre::Quaternion(Ogre::Radian(M_PI/2) ,Ogre::Vector3::UNIT_Y);
	
	BOOST_FOREACH(Block const& block, _blocks)
	{
		std::stringstream log;
		log << "Adding " << block._entity->getName() << " at (" << block._position.x << ", " << block._position.z << ") " << 
		(block._orientation == North ? "N" : block._orientation == East ? "E" : block._orientation == South ? "S" : "W");
		
		Ogre::LogManager::getSingletonPtr()->logMessage(log.str());

		switch (block._orientation)
		{
			case North:
				sg->addEntity(block._entity, block._position, Ogre::Quaternion::IDENTITY, Ogre::Vector3(0.05, 0.05, 0.05));
				break;
			case East:
				sg->addEntity(block._entity, block._position, eastRotation, Ogre::Vector3(0.05, 0.05, 0.05));
				break;
			case South:
				sg->addEntity(block._entity, block._position, southRotation, Ogre::Vector3(0.05, 0.05, 0.05));
				break;
			case West:
				sg->addEntity(block._entity, block._position, westRotation, Ogre::Vector3(0.05, 0.05, 0.05));
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