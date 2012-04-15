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

#include "BulletDebug.h"

BulletDebug::BulletDebug(Ogre::SceneManager & scm, btDynamicsWorld & world) : _dd(&scm, 0.5), _world(&world), enabled(false)
{
	world.setDebugDrawer(this);
	_dd.setEnabled(true);

	setDebugMode(btIDebugDraw::DBG_DrawWireframe);
}

BulletDebug::~BulletDebug()
{
	_world->setDebugDrawer(0);
}

void BulletDebug::draw()
{
	if (enabled)
	{
		_dd.clear();
		_world->debugDrawWorld();
		_dd.build();
	}
}

void BulletDebug::setEnabled(bool _enable)
{
	enabled = _enable;
}

void BulletDebug::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	_dd.drawLine(
		Ogre::Vector3(from.getX(), from.getY(), from.getZ()),
		Ogre::Vector3(to.getX(), to.getY(), to.getZ()),
		Ogre::ColourValue(color.getX(), color.getY(), color.getZ()));
}