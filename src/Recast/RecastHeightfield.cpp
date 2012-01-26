//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
// Copyright 2012 Patrick Nicolas <patricknicolas@laposte.net>
// This version is derived from original Recast source
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <float.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include "Recast.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"

namespace Recast
{

Heightfield::Heightfield(Ogre::AxisAlignedBox const& boundingBox, float cellSize, float cellHeight):
_boundingBox(boundingBox), _cs(cellSize), _ch(cellHeight)
{
	_width = (int)(_boundingBox.getSize().x/_cs + 0.5f);
	_height = (int)(_boundingBox.getSize().z/_ch + 0.5f);
}

void Span::merge(Span const& other, const int flagMergeThr)
{
	_smin = std::min(other._smin, _smin);
	_smax = std::max(other._smax, _smax);
	if (std::abs((int)_smax - (int)_smax) <= flagMergeThr)
	{
		_area = std::max(_area, other._area);
	}
}

void Heightfield::addSpan(const int x, const int y,
	const unsigned short smin, const unsigned short smax,
	const unsigned char area, const int flagMergeThr)
{
	Span s(smin, smax, area);

	std::map<std::pair<int,int>, std::list<Span> >::iterator cell = _spans.find(std::make_pair(x,y));
	//If the cell is empty, add a new list
	if ( cell == _spans.end())
	{
		std::list<Span> newList;
		newList.push_back(s);
		_spans.insert(std::make_pair(std::make_pair(x,y), newList));
		return;
	}
	std::list<Span> & intervals = cell->second;
	std::list<Span>::iterator target= intervals.begin();
	for (std::list<Span>::iterator end = intervals.end() ;
	     target != end && target->_smin <= s._smax;
	     ++target)
	{
		//Merge spans, remove the current one
		if (target->_smax >= s._smin)
		{
			s.merge(*target, flagMergeThr);
			target = intervals.erase(target);
		}
	}
	intervals.insert(target, s);
}

#ifdef RCHF
int rcGetHeightFieldSpanCount(rcContext* /*ctx*/, rcHeightfield& hf)
{
	// TODO: VC complains about unref formal variable, figure out a way to handle this better.
//	rcAssert(ctx);
	
	const int w = hf.width;
	const int h = hf.height;
	int spanCount = 0;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			for (rcSpan* s = hf.spans[x + y*w]; s; s = s->next)
			{
				if (s->area != RC_NULL_AREA)
					spanCount++;
			}
		}
	}
	return spanCount;
}
#endif

}
