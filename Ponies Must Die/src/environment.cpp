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

namespace pmd
{

Environment::Environment (Ogre::SceneManager* sceneManager, std::istream &level) : _sceneManager(sceneManager)
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
				str << "Invalid orientation (" << Orientation << ")";// << " line " << LineNumber;
				throw std::invalid_argument(str.str());
			}

			Ogre::Entity * Entity = _sceneManager->createEntity(MeshName);
			_blocks.push_back(Block(Entity, o, Ogre::Vector3(x,y,z)));
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