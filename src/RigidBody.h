/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011-2012  Guillaume Meunier <guillaume.meunier@centraliens.net>

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

#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include "bullet/LinearMath/btMotionState.h"
#include <OgreQuaternion.h>
#include <OgreVector3.h>

template<class T> class RigidBody : public btMotionState
{
public:
	RigidBody(
		const Ogre::Quaternion &rot,
		const Ogre::Vector3 &pos,
		const Ogre::Vector3 &CoG,
		T * node);
	virtual ~RigidBody();
	virtual void getWorldTransform(btTransform &worldTrans) const;
	virtual void setWorldTransform(const btTransform &worldTrans);
	void setNode(T * node);

private:
	btTransform      _Transform;
	Ogre::Vector3    _CoG;
	Ogre::Quaternion _Rotation;
	Ogre::Vector3    _Position;
	T *              _Node;
};

#endif // RIGIDBODY_H
