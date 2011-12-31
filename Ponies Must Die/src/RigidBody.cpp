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
	const Ogre::Vector3 &CoG,
	T * node)
{
	_CoG = CoG;
	_Rotation = rot;

	Ogre::Matrix3 M;
	_Rotation.ToRotationMatrix(M);
	_Position = pos;
	Ogre::Vector3 pos2 = pos + M * _CoG;

	_Transform = btTransform(
		btQuaternion(rot.x, rot.y, rot.z, rot.w),
		btVector3(pos2.x, pos2.y, pos2.z));

	_Node = node;
}

template<class T> void RigidBody<T>::setWorldTransform(const btTransform &worldTrans)
{
	_Transform = worldTrans;

	btQuaternion q = worldTrans.getRotation();
	btVector3 x = worldTrans.getOrigin();

	_Rotation = Ogre::Quaternion(q.w(), q.x(), q.y(), q.z());
	Ogre::Matrix3 M;
	_Rotation.ToRotationMatrix(M);

	_Position = Ogre::Vector3(x.x(), x.y(), x.z()) - M * _CoG;

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
template class RigidBody<Ogre::Bone>;
