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

#ifndef CHARACTERANIMATION_H
#define CHARACTERANIMATION_H

#include <map>
#include <string>

namespace Ogre
{
	class AnimationState;
	class Entity;
}

class CharacterAnimation
{
private:
	struct AnimState
	{
		Ogre::AnimationState * _as;
		float _TargetWeight;
		float _FadeInSpeed;
		float _FadeOutSpeed;
		float _Speed;
		AnimState(Ogre::AnimationState* AnimState, float TargetWeight, float FadeInSpeed, float FadeOutSpeed, float Speed) :
			_as(AnimState),
			_TargetWeight(TargetWeight),
			_FadeInSpeed(FadeInSpeed),
			_FadeOutSpeed(FadeOutSpeed),
			_Speed(Speed) {}
	};
	
	typedef std::map<std::string, AnimState> AnimationMap;
	
	AnimationMap Animations;
	
public:
	CharacterAnimation(Ogre::Entity * ent);
	
	void SetAnimation(std::string AnimName, float weight = 1);
	void ClearAnimations(void);
	void PushAnimation(std::string AnimName, float weight = 1);
	void SetWeight(std::string AnimName, float weight);
	
	void SetFadeSpeed(std::string AnimName, float FadeInSpeed, float FadeOutSpeed);
	void SetSpeed(std::string AnimName, float Speed);
	void Update(float dt);
};

#endif // CHARACTERANIMATION_H
