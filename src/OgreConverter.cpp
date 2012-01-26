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

#include <boost/foreach.hpp>

#include <OgreVector3.h>
#include <OgreVertexIndexData.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreSubMesh.h>

#include "bullet/BulletCollision/CollisionShapes/btTriangleMesh.h"

#include "OgreConverter.h"

#include "Recast/Recast.h"

void OgreConverter::AddVertices(Ogre::VertexData * data)
{
	const Ogre::VertexElement* Positions = data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
	Ogre::HardwareVertexBufferSharedPtr VertexBuffer = data->vertexBufferBinding->getBuffer(Positions->getSource());
	const size_t VertexSize = VertexBuffer->getVertexSize();

	uintptr_t VertexPtr = (uintptr_t)VertexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);

	for(size_t j = 0; j < data->vertexCount; j++, VertexPtr += VertexSize)
	{
		float * ptr;
		Positions->baseVertexPointerToElement((void*)VertexPtr, &ptr);
		Vertices.push_back(Ogre::Vector3(ptr[0], ptr[1], ptr[2]));
	}

	VertexBuffer->unlock();
}

void OgreConverter::AddIndexData(Ogre::IndexData * data, int offset)
{
	size_t FaceCount = data->indexCount / 3;
	Ogre::HardwareIndexBufferSharedPtr IndexBuffer = data->indexBuffer;

	if (IndexBuffer->getType() == Ogre::HardwareIndexBuffer::IT_32BIT)
	{
		const uint32_t * indices = static_cast<uint32_t *>(IndexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
		for(size_t i = 0; i < FaceCount; i++, indices += 3)
		{
			Face f;
			f.VertexIndices[0] = indices[0] + offset;
			f.VertexIndices[1] = indices[1] + offset;
			f.VertexIndices[2] = indices[2] + offset;

			Faces.push_back(f);
		}
		IndexBuffer->unlock();
	}
	else
	{
		const uint16_t * indices = static_cast<uint16_t *>(IndexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
		for(size_t i = 0; i < FaceCount; i++, indices += 3)
		{
			Face f;
			f.VertexIndices[0] = indices[0] + offset;
			f.VertexIndices[1] = indices[1] + offset;
			f.VertexIndices[2] = indices[2] + offset;

			Faces.push_back(f);
		}
		IndexBuffer->unlock();
	}
}

OgreConverter::OgreConverter(Ogre::Entity& entity)
{
	if (entity.getMesh()->sharedVertexData)
	{
		AddVertices(entity.getMesh()->sharedVertexData);
	}

	for(unsigned int i = 0; i < entity.getNumSubEntities(); i++)
	{
		Ogre::SubMesh * submesh = entity.getSubEntity(i)->getSubMesh();
		if (submesh->useSharedVertices)
		{
			AddIndexData(submesh->indexData, 0);
		}
		else
		{
			AddIndexData(submesh->indexData, Vertices.size());
			AddVertices(submesh->vertexData);
		}
	}
}

static btVector3 Ogre2Bullet(Ogre::Vector3 const& v)
{
	return btVector3(v.x, v.y, v.z);
}

void OgreConverter::AddToTriMesh(Ogre::Matrix4 const& transform, btTriangleMesh& trimesh) const
{
	BOOST_FOREACH(Face const& i, Faces)
	{
		Ogre::Vector3 v1 = transform * Vertices[i.VertexIndices[0]];
		Ogre::Vector3 v2 = transform * Vertices[i.VertexIndices[1]];
		Ogre::Vector3 v3 = transform * Vertices[i.VertexIndices[2]];

		trimesh.addTriangle(
			Ogre2Bullet(v1),
			Ogre2Bullet(v2),
			Ogre2Bullet(v3));
	}
}

void OgreConverter::Vector3ToFloatArray(Ogre::Vector3 const& v, float *point)
{
	point[0] = v.x;
	point[1] = v.y;
	point[2] = v.z;
}

void OgreConverter::AddToHeightField(Ogre::Matrix4 const& transform, Recast::rcHeightfield& heightField, unsigned char areaID, int flagMergeThr) const
{
	Recast::rcContext dummyCtx(false);
	float p1[3], p2[3], p3[3];
	
	BOOST_FOREACH(Face const& i, Faces)
	{
		Vector3ToFloatArray( transform * Vertices[i.VertexIndices[0]], p1);
		Vector3ToFloatArray( transform * Vertices[i.VertexIndices[1]], p2);
		Vector3ToFloatArray( transform * Vertices[i.VertexIndices[2]], p3);

		Recast::rcRasterizeTriangle(&dummyCtx, p1, p2, p3, areaID, heightField, flagMergeThr);
	}
}

