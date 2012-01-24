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

#include <vector>
#ifdef _WINDOWS
#include <boost/cstdint.hpp>
using boost::uint16_t;
using boost::uint32_t;
#else
#include <stdint.h>
#endif

namespace Ogre
{
	class VertexData;
	class IndexData;
	class Entity;
	class Matricx;
}

class btTriangleMesh;

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
	static btVector3 OgreConverter::Ogre2Bullet(Ogre::Vector3& v);

public:
	OgreConverter(Ogre::Entity& entity);
	void AddToTriMesh(Ogre::Matrix4& transform, btTriangleMesh& trimesh);
};
