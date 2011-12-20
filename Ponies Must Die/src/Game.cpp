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


float distance = 3;

namespace pmd
{
	bool Game::mouseMoved(const OIS::MouseEvent &arg)
	{
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

		/*if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
			return false;*/

		if (_Shutdown)
			return false;

		if (_Keyboard->isKeyDown(OIS::KC_ADD))
		{
			distance *= exp(evt.timeSinceLastFrame);
			_Camera->setPosition(0, 2, distance);
		}
		else if (_Keyboard->isKeyDown(OIS::KC_SUBTRACT))
		{
			distance *= exp(-evt.timeSinceLastFrame);
			_Camera->setPosition(0, 2, distance);
		}

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

		_Camera->setPosition(0, 2, distance);
		
		_Camera->setNearClipDistance(0.01);
		
		_Root->startRendering();

		cleanup();
	}
}
