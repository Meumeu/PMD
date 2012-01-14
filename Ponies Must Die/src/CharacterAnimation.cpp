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
#include <boost/foreach.hpp>

#include <OgreAnimationState.h>
#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>

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
	BOOST_FOREACH(AnimationMap::value_type &i, Animations)
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
	BOOST_FOREACH(AnimationMap::value_type &i, Animations)
	{
		i.second._TargetWeight = 0;
	}
}

void CharacterAnimation::SetFadeSpeed(std::string AnimName, float FadeInSpeed, float FadeOutSpeed)
{
	Animations.at(AnimName)._FadeInSpeed = FadeInSpeed;
	Animations.at(AnimName)._FadeOutSpeed = FadeOutSpeed;
}

void CharacterAnimation::SetSpeed(std::string AnimName, float Speed)
{
	Animations.at(AnimName)._Speed = Speed;
}

void CharacterAnimation::PushAnimation(std::string AnimName, float weight)
{
	Animations.at(AnimName)._TargetWeight = weight;
}

void CharacterAnimation::SetWeight(std::string AnimName, float weight)
{
	Animations.at(AnimName)._as->setWeight(weight);
}

void CharacterAnimation::Update(float dt)
{
	BOOST_FOREACH(AnimationMap::value_type &i, Animations)
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
		i.second._as->addTime(dt);
	}
}

