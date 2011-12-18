/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "environment.h"

#include <boost/foreach.hpp>
#include <set>
#include <OgreEntity.h>
#include <OgreSceneManager.h>

namespace pmd
{

Environment::Environment ( Ogre::SceneManager* sceneManager ) : _sceneManager(sceneManager)
{
	Ogre::Entity * block1Entity = _sceneManager->createEntity("block1.mesh");
	_blocks.push_back(Block(block1Entity, North, Ogre::Vector3(0,0,0)));
	_blocks.push_back(Block(block1Entity, North, Ogre::Vector3(1,0,0)));
	_blocks.push_back(Block(block1Entity, North, Ogre::Vector3(0,1,0)));
	_blocks.push_back(Block(block1Entity, North, Ogre::Vector3(1,1,0)));
	
	_blocks.push_back(Block(block1Entity, East,  Ogre::Vector3(2,0,0)));
	_blocks.push_back(Block(block1Entity, West,  Ogre::Vector3(0,2,0)));
	_blocks.push_back(Block(block1Entity, South, Ogre::Vector3(2,1,0)));

}

Environment::~Environment()
{
	std::set<Ogre::Entity*> entities;
	BOOST_FOREACH(Block const& block, _blocks)
	{
		entities.insert(block._entity);
	}
	BOOST_FOREACH(Ogre::Entity * entity, entities)
	{
		_sceneManager->destroyEntity(entity);
	}

}

void Environment::draw()
{
	Ogre::SceneNode* root = _sceneManager->getRootSceneNode();
	Ogre::Quaternion
		eastRotation  = Ogre::Quaternion(Ogre::Radian(-M_PI/2),Ogre::Vector3::UNIT_Z),
		southRotation = Ogre::Quaternion(Ogre::Radian(M_PI)   ,Ogre::Vector3::UNIT_Z),
		westRotation  = Ogre::Quaternion(Ogre::Radian(M_PI/2) ,Ogre::Vector3::UNIT_Z);
	BOOST_FOREACH(Block const& block, _blocks)
	{
		Ogre::SceneNode* blockNode = root->createChildSceneNode();
		blockNode->attachObject(block._entity);
		blockNode->setPosition(block._position);
		switch (block._orientation)
		{
			case North:
				break;
			case East:
				blockNode->setOrientation(eastRotation);
				break;
			case South:
				blockNode->setOrientation(southRotation);
				break;
			case West:
				blockNode->setOrientation(westRotation);
		}
	}
}

}