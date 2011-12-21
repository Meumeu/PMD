/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Guillaume Meunier <guillaume.meunier@centraliens.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pmd.h"
#include "Game.h"

#include "environment.h"

#include <stdio.h>
#include <OgreEntity.h>

const float CameraDistance = 0.8;
const float CameraHeight = 1.8;

namespace pmd
{
	bool Game::mouseMoved(const OIS::MouseEvent &arg)
	{
		_Heading -= Ogre::Radian(arg.state.X.rel * 0.003);
		_Pitch -= Ogre::Radian(arg.state.Y.rel * 0.003);
		
		if (_Heading.valueRadians() > M_PI)
			_Heading -= Ogre::Radian(2 * M_PI);
		else if (_Heading.valueRadians() < -M_PI)
			_Heading += Ogre::Radian(2 * M_PI);
		
		if (_Pitch.valueRadians() > M_PI / 2)
			_Pitch = Ogre::Radian(M_PI / 2);
		else if (_Pitch.valueRadians() < -M_PI / 2)
			_Pitch = Ogre::Radian(-M_PI / 2);
		
		_Player->setOrientation(Ogre::Quaternion(_Heading, Ogre::Vector3::UNIT_Y));
		_Camera->setOrientation(Ogre::Quaternion(_Pitch, Ogre::Vector3::UNIT_X));
		_Camera->setPosition(0, CameraHeight - CameraDistance * sin(_Pitch.valueRadians()), CameraDistance * cos(_Pitch.valueRadians()));
		
		return true;
	}

	bool Game::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
	{
		return true;
	}

	bool Game::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
	{
		return true;
	}

	bool Game::keyPressed(const OIS::KeyEvent &arg)
	{
		if (arg.key == OIS::KC_ESCAPE)
			_Shutdown = true;
		return true;
	}

	bool Game::keyReleased(const OIS::KeyEvent &arg)
	{
		return true;
	}

	bool Game::frameRenderingQueued(const Ogre::FrameEvent& evt)
	{
		if (_Window->isClosed())
			return false;

		if (_Shutdown)
			return false;

		double velX = 0, velZ = 0;
		double dt = evt.timeSinceLastFrame;
		
		if (_Keyboard->isKeyDown(OIS::KC_Z))
		{
			velZ = -2;
		}
		else if (_Keyboard->isKeyDown(OIS::KC_S))
		{
			velZ = 2;
		}

		if (_Keyboard->isKeyDown(OIS::KC_Q))
		{
			velX = -2;
		}
		else if (_Keyboard->isKeyDown(OIS::KC_D))
		{
			velX = 2;
		}
		
		if (_Keyboard->isKeyDown(OIS::KC_LSHIFT))
		{
			velX *= 2.5;
			velZ *= 2.5;
		}
		
		
		
		Ogre::Matrix3 orientation;
		_Player->getOrientation().ToRotationMatrix(orientation);
		Ogre::Vector3 vel = orientation * Ogre::Vector3(velX, 0, velZ);
		
		_Player->setPosition(_Player->getPosition() + vel * dt);
		
		//Need to capture/update each device
		_Keyboard->capture();
		_Mouse->capture();

		return true;
	}

	void Game::go(void)
	{
		if (!setup())
			return;

		Environment env(_SceneMgr);

		Ogre::Entity * PlayerEntity = _SceneMgr->createEntity("player", "player.mesh");

		_Player = _SceneMgr->getRootSceneNode()->createChildSceneNode("player");
		_Player->attachObject(PlayerEntity);
		_Player->attachObject(_Camera);

		_Player->setOrientation(Ogre::Quaternion(_Heading, Ogre::Vector3::UNIT_Y));
		_Player->setPosition(0, 0, 0);

		_Camera->setOrientation(Ogre::Quaternion(_Pitch, Ogre::Vector3::UNIT_X));
		_Camera->setPosition(0, CameraHeight - CameraDistance * sin(_Pitch.valueRadians()), CameraDistance * cos(_Pitch.valueRadians()));

		_Camera->setNearClipDistance(0.01);

		_Root->startRendering();

		cleanup();
	}
}
