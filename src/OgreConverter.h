/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  Guillaume Meunier <guillaume.meunier@centraliens.net>

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

#ifndef OGRECONVERTER_H
#define OGRECONVERTER_H

#include <vector>
#ifdef _WINDOWS
#include <boost/cstdint.hpp>
using boost::uint16_t;
using boost::uint32_t;
#else
#include <stdint.h>
#endif

#include <OgreVector3.h>
namespace Ogre
{
	class VertexData;
	class IndexData;
	class Entity;
	class Matricx;
}

class btTriangleMesh;

struct rcHeightfield;

class OgreConverter
{
	struct Face
	{
		uint32_t VertexIndices[3];
	};

	std::vector<Ogre::Vector3> Vertices;
	std::vector<Face> Faces;

	void AddVertices(Ogre::VertexData * data);
	void AddIndexData(Ogre::IndexData * data, int offset);

public:
	OgreConverter(Ogre::Entity& entity);
	void AddToTriMesh(Ogre::Matrix4 const& transform, btTriangleMesh& trimesh) const;
	void AddToHeightField(Ogre::Matrix4 const& transform, rcHeightfield& heightField, unsigned char areaID, int flagMergeThr) const;
};

#endif
