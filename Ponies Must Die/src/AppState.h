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

#ifndef APPSTATE_H
#define APPSTATE_H

#include <OgreFrameListener.h>
#include <OISKeyboard.h>
#include <OISMouse.h>


namespace pmd
{
class AppState
{
public:
    AppState();
    virtual ~AppState();
	virtual void Enter(void) = 0;
	virtual void Exit(void) = 0;
	virtual void Pause(void) = 0;
	virtual void Resume(void) = 0;
	
	virtual void Update(float TimeSinceLastFrame) = 0;
};
}

#endif // APPSTATE_H
