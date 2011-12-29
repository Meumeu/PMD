/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Guillaume Meunier <guillaume.meunier@centraliens.net>

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

#include "RigidBody.h"
#include <OgreSceneNode.h>
#include <OgreBone.h>


template<class T> RigidBody<T>::RigidBody(
	const Ogre::Quaternion &rot,
	const Ogre::Vector3 &pos,
	T * node)
{
	_Transform = btTransform(
		btQuaternion(rot.x, rot.y, rot.z, rot.w),
		btVector3(pos.x, pos.y, pos.z));

	_Rotation = rot;
	_Position = pos;

	_Node = node;
}

template<class T> void RigidBody<T>::setWorldTransform(const btTransform &worldTrans)
{
	_Transform = worldTrans;

	btQuaternion q = worldTrans.getRotation();
	btVector3 x = worldTrans.getOrigin();

	_Rotation = Ogre::Quaternion(q.w(), q.x(), q.y(), q.z());
	_Position = Ogre::Vector3(x.x(), x.y(), x.z());

	_Node->setPosition(_Position);
	_Node->setOrientation(_Rotation);
}

template<class T> RigidBody<T>::~RigidBody(void)
{
}

template<class T> void RigidBody<T>::getWorldTransform(btTransform &worldTrans) const
{
	worldTrans = _Transform;
}

template class RigidBody<Ogre::SceneNode>;
