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

#include "CharacterAnimation.h"

#include <OgreAnimationState.h>
#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>

#include <boost/foreach.hpp>

CharacterAnimation::CharacterAnimation(Ogre::Entity* ent)
{
	Ogre::AnimationStateSet * anims = ent->getAllAnimationStates();
	Ogre::AnimationStateIterator it = anims->getAnimationStateIterator();
	while(it.hasMoreElements())
	{
		Ogre::AnimationState * as = it.getNext();
		as->setWeight(0);
		Animations.insert(std::make_pair(as->getAnimationName(), AnimState(as, 0, 5, 5, 1)));
	}
	
	ent->getSkeleton()->setBlendMode(Ogre::ANIMBLEND_CUMULATIVE);
}

void CharacterAnimation::SetAnimation(std::string AnimName, float weight)
{
	BOOST_FOREACH(auto & i, Animations)
	{
		if (i.first == AnimName)
		{
			i.second._TargetWeight = weight;
		}
		else
		{
			i.second._TargetWeight = 0;
		}
	}
}

void CharacterAnimation::ClearAnimations(void )
{
	BOOST_FOREACH(auto & i, Animations)
	{
		i.second._TargetWeight = 0;
	}
}

void CharacterAnimation::SetFadeSpeed(std::string AnimName, float FadeInSpeed, float FadeOutSpeed)
{
	AnimationMap::iterator it = Animations.find(AnimName);
	if (it == Animations.end()) return; //throw std::out_of_range(AnimName + " out of range");

	it->second._FadeInSpeed = FadeInSpeed;
	it->second._FadeOutSpeed = FadeOutSpeed;
}

void CharacterAnimation::SetSpeed(std::string AnimName, float Speed)
{
	AnimationMap::iterator it = Animations.find(AnimName);
	if (it == Animations.end()) return; //throw std::out_of_range(AnimName + " out of range");

	it->second._Speed = Speed;
}

float CharacterAnimation::GetLength(std::string AnimName)
{
	AnimationMap::iterator it = Animations.find(AnimName);
	if (it == Animations.end()) return 0; //throw std::out_of_range(AnimName + " out of range");
	
	return it->second._as->getLength();
}

void CharacterAnimation::SetTime(std::string AnimName, float t)
{
	AnimationMap::iterator it = Animations.find(AnimName);
	if (it == Animations.end()) return; //throw std::out_of_range(AnimName + " out of range");
	
	it->second._as->setTimePosition(t);
}

void CharacterAnimation::PushAnimation(std::string AnimName, float weight)
{
	AnimationMap::iterator it = Animations.find(AnimName);
	if (it == Animations.end()) return; //throw std::out_of_range(AnimName + " out of range");

	it->second._TargetWeight = weight;
}

void CharacterAnimation::SetWeight(std::string AnimName, float weight)
{
	AnimationMap::iterator it = Animations.find(AnimName);
	if (it == Animations.end()) return; //throw std::out_of_range(AnimName + " out of range");

	it->second._as->setWeight(weight);
}

void CharacterAnimation::Update(float dt)
{
	BOOST_FOREACH(auto & i, Animations)
	{
		float CurWeight = i.second._as->getWeight();
		if (CurWeight > i.second._TargetWeight)
		{
			CurWeight -= i.second._FadeOutSpeed * dt;
			if (CurWeight < i.second._TargetWeight)
				CurWeight = i.second._TargetWeight;
		}
		else if (CurWeight < i.second._TargetWeight)
		{
			CurWeight += i.second._FadeInSpeed * dt;
			if (CurWeight > i.second._TargetWeight)
				CurWeight = i.second._TargetWeight;
		}
		
		i.second._as->setEnabled(CurWeight > 0);
		i.second._as->setWeight(CurWeight);
		i.second._as->addTime(dt * i.second._Speed);
	}
}

