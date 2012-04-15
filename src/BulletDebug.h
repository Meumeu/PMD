/*
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

#ifndef BULLETDEBUG_H
#define BULLETDEBUG_H

#include "bullet/LinearMath/btIDebugDraw.h"
#include "bullet/BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "DebugDrawer.h"
#include <OgreSceneManager.h>

class BulletDebug : public btIDebugDraw
{
	DebugDrawer _dd;
	btDynamicsWorld * _world;
	bool enabled;
	int dbgmode;

	BulletDebug();
	BulletDebug(BulletDebug const &);
	BulletDebug & operator=(BulletDebug const &);

public:
	BulletDebug(Ogre::SceneManager &, btDynamicsWorld &);
	~BulletDebug();
	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
	void draw();
	void setEnabled(bool);
	bool isEnabled() { return enabled; }
	void toggleEnabled() { setEnabled(!enabled); }

	virtual void draw3dText(const btVector3& location,const char* textString) {}
	virtual void setDebugMode(int debugMode) { dbgmode = debugMode; }
	virtual int  getDebugMode() const { return dbgmode; }
	virtual void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color) {};
	virtual void reportErrorWarning(const char* warningString) {};
};

#endif // BULLETDEBUG_H
