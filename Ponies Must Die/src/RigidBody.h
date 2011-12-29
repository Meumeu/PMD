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

#include <LinearMath/btMotionState.h>
#include <OgreQuaternion.h>
#include <OgreVector3.h>

template<class T> class RigidBody : public btMotionState
{
public:
	RigidBody(const Ogre::Quaternion &rot, const Ogre::Vector3 &pos, T * node);
	virtual ~RigidBody();
	virtual void getWorldTransform(btTransform &worldTrans) const;
	virtual void setWorldTransform(const btTransform &worldTrans);

private:
	btTransform      _Transform;
	Ogre::Quaternion _Rotation;
	Ogre::Vector3    _Position;
	T *              _Node;
};
