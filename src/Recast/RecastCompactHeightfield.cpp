//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
// Copyright 2012 Patrick Nicolas <patricknicolas@laposte.net>
// Copyright 2012 Guillaume Meunier <guillaume.meunier@centraliens.net>
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

#include <climits>
#include <boost/foreach.hpp>

#include "Recast.h"
#include "RecastHeightfield.h"
#include "RecastCompactHeightfield.h"

namespace Recast
{

static bool isWalkable(CompactSpan const & s1, CompactSpan const & s2, const int walkableHeight, const int walkableClimb)
{
	const int bottom = std::max(s1._bottom, s2._bottom);
	const int top = std::min(s1._bottom + s1._height, s2._bottom + s2._height);
	const int climb = std::abs(s1._bottom - s2._bottom);
	
	return (top - bottom) >= walkableHeight && climb <= walkableClimb;
}

static void filterLowHangingWalkableObstacles(std::list<Span> & spans, int walkableClimb)
{
	bool previousWalkable = false;
	Span * prev = 0;
	BOOST_FOREACH(Span& i, spans)
	{
		const bool walkable = i._walkable;
		if (!walkable && previousWalkable)
		{
			if (std::abs(i._smax - prev->_smax) <= walkableClimb)
				i._walkable = previousWalkable;
		}
		
		prev = &i;
		
		// Copy walkable flag so that it cannot propagate
		// past multiple non-walkable objects.
		previousWalkable = walkable;
	}
}

const int MAX_HEIGHT = 1000000;
const int MIN_HEIGHT = -1000000;

static void filterLedgeSpans(std::list<Span> & spans, int x, int z, const Heightfield& hf, int walkableClimb, int walkableHeight)
{
	for(std::list<Span>::iterator it = spans.begin(), end = spans.end(); it != end; ++it)
	{
		Span * curr = &*it;
		std::list<Span>::iterator it2 = it; ++it2;
		Span * next = (it2 != end) ? &*it2 : 0;
		
		if (!curr->_walkable) continue;
		
		const int bot = curr->_smax;
		const int top = next ? next->_smin : MAX_HEIGHT;
		
		int minh = MAX_HEIGHT - bot, asmin = curr->_smax, asmax = curr->_smax;
		
		for(int dir = Direction::Begin; dir < Direction::End; ++dir)
		{
			const int nx = x + Direction::getXOffset(dir);
			const int nz = z + Direction::getZOffset(dir);
			
			const std::list<Span> & nspans = hf.getSpans(nx, nz);
			if (nspans.begin() == nspans.end()) continue;
			
			// Minus infinity to first span
			const int nbot = MIN_HEIGHT; //-walkableClimb;
			const int ntop = nspans.begin()->_smin;
			
			if (std::min(top, ntop) - std::max(bot, nbot) > walkableHeight)
				minh = std::min(minh, nbot - bot);
			
			for(std::list<Span>::const_iterator it2 = nspans.begin(), end2 = nspans.end(); it2 != end2; ++it2)
			{
				std::list<Span>::const_iterator it3 = it2; ++it3;
				assert(it3 == end2 || it3->_smin > it2->_smax);
				const int nbot = it2->_smax;
				const int ntop = (it3 != end2) ? it3->_smin : MAX_HEIGHT;
				
				if (std::min(top, ntop) - std::max(bot, nbot) < walkableHeight) continue;
				
				minh = std::min(minh, nbot - bot);
				if (std::abs(nbot - bot) <= walkableClimb)
				{
					asmin = std::min(asmin, nbot);
					asmax = std::max(asmax, nbot);
				}
			}
		}
		
		// The current span is close to a ledge if the drop to any
		// neighbour span is less than the walkableClimb.
		if (minh < -walkableClimb) curr->_walkable = false;
		
		// If the difference between all neighbours is too large,
		// we are at steep slope, mark the span as ledge.
		if ((asmax - asmin) > walkableClimb) curr->_walkable = false;
	}
}

std::pair<int, const CompactSpan *> CompactHeightfield::findSpan(IntVertex const & v) const
{
	if (v.x < 0 || v.z < 0 || v.x >= _xsize || v.z >= _zsize) return std::make_pair(INT_MAX, (const CompactSpan*)0);
	
	const CompactSpan * best = (const CompactSpan *)0;
	int bestdist = INT_MAX;
	
	BOOST_FOREACH(CompactSpan const & span, _cells[v.x + v.z * _xsize])
	{
		int dist = std::abs(v.y - span._bottom);
		if (dist < bestdist)
		{
			bestdist = dist;
			best = &span;
		}
	}
	
	return std::make_pair(bestdist, best);
}

/// @par
///
/// This is just the beginning of the process of fully building a compact heightfield.
/// Various filters may be applied applied, then the distance field and regions built.
/// E.g: #rcBuildDistanceField and #rcBuildRegions
///
/// See the #rcConfig documentation for more information on the configuration parameters.
///
/// @see rcAllocCompactHeightfield, rcHeightfield, rcCompactHeightfield, rcConfig
CompactHeightfield::CompactHeightfield(const float walkableHeight, const float walkableClimb, const Heightfield& hf, const float radius,
	const bool filterLowHangingWalkableObstacles, const bool filterLedgeSpans, const int borderSize) :
	_spanNumber(0),
	_walkableHeight(ceil(walkableHeight / hf.getCellHeight())),
	_walkableClimb(floor(walkableClimb / hf.getCellHeight())),
	_borderSize(borderSize),
	_maxDistance(0),
	_maxRegions(0),
	_cs(hf.getCellSize()),
	_ch(hf.getCellHeight())
{
	hf.getOffsetAndSize(_xmin, _xsize, _zmin, _zsize);
	std::cerr << "size (" << _xsize << "," << _zsize << "), cs=" << _cs << "\n";
	_cells.reset(new std::vector<CompactSpan>[_xsize * _zsize]);
	
	// Fill in cells and spans.
	
	for (int z = 0; z < _zsize; ++z)
	{
		for (int x = 0; x < _xsize; ++x)
		{
			std::list<Span> spans = hf.getSpans(x + _xmin, z + _zmin);
			
			if (filterLowHangingWalkableObstacles) Recast::filterLowHangingWalkableObstacles(spans, _walkableClimb);
			if (filterLedgeSpans) Recast::filterLedgeSpans(spans, x + _xmin, z + _zmin, hf, _walkableClimb, _walkableHeight);
			
			if (!spans.empty())
			{
				std::vector<CompactSpan> & vspan = _cells[x + z * _xsize];
				for(std::list<Span>::const_iterator next = spans.begin(), s = next++; s != spans.end(); s = (next != spans.end()) ? next++ : next)
				{
					assert((next == spans.end()) || (next->_smin >= s->_smax));
					if (!s->_walkable) continue;
					
					const int bottom = s->_smax;
					const int height = ((next == spans.end()) ? MAX_HEIGHT : next->_smin) - bottom;

					if (height < _walkableHeight ) continue;
					
					vspan.push_back(CompactSpan(bottom, height));
					_spanNumber++;
				}
			}
		}
	}
	
	

	// Find neighbour connections.
	for(int z = 0; z < _zsize; z++)
	{
		for(int x = 0; x < _xsize; x++)
		{
			std::vector<CompactSpan>& c = _cells[x + z * _xsize];
			
			for(int i = 0, end = c.size(); i < end; i++)
			{
				CompactSpan &s = c[i];
				
				for(int dir = Direction::Begin; dir < Direction::End; ++dir)
				{
					s.neighbours[dir] = 0;
					const int nx = x + Direction::getXOffset(dir);
					const int nz = z + Direction::getZOffset(dir);
					
					// First check that the neighbour cell is in bounds.
					if (nx < 0 || nz < 0 || nx >= _xsize || nz >= _zsize) continue;
					
					std::vector<CompactSpan>& nc = _cells[nx + nz * _xsize];
					
					for(int k = 0, endk = nc.size(); k < endk; k++)
					{
						CompactSpan& ns = nc[k];
						if (isWalkable(s, ns, _walkableHeight, _walkableClimb))
							s.neighbours[dir] = &ns;
					}
				}
			}
		}
	}
	
	erodeWalkableArea(ceil(radius / _cs));
}

}
