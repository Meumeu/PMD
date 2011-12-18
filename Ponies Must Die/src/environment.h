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


#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <vector>
#include <OgreVector3.h>

namespace Ogre {
	class SceneManager;
	class Entity;
}
namespace pmd
{
class Environment
{
	enum orientation_t { North, South, East, West};
	struct Block
	{
		Block(Ogre::Entity * entity, orientation_t orientation, Ogre::Vector3 position):
			_entity(entity), _orientation(orientation), _position(position) {}

		Ogre::Entity * _entity;
		orientation_t _orientation;
		Ogre::Vector3 _position;
	};
public:
	Environment(Ogre::SceneManager *sceneManager);
	~Environment();

	void draw();
private:
	Ogre::SceneManager * _sceneManager;
	std::vector<Block> _blocks;
};
}
#endif // ENVIRONMENT_H
