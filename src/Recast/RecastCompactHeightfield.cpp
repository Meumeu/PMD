//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
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
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <climits>

#include <boost/foreach.hpp>

#include "Recast.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"
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

/// @par
///
/// This is just the beginning of the process of fully building a compact heightfield.
/// Various filters may be applied applied, then the distance field and regions built.
/// E.g: #rcBuildDistanceField and #rcBuildRegions
///
/// See the #rcConfig documentation for more information on the configuration parameters.
///
/// @see rcAllocCompactHeightfield, rcHeightfield, rcCompactHeightfield, rcConfig
CompactHeightfield::CompactHeightfield(const int walkableHeight, const int walkableClimb, const Heightfield& hf) :
	_walkableHeight(walkableHeight),
	_walkableClimb(walkableClimb),
	_borderSize(0),
	_maxDistance(0),
	_maxRegions(0),
	_cs(hf.getCellSize()),
	_ch(hf.getCellHeight())
{
	hf.getOffsetAndSize(_xmin, _xsize, _zmin, _zsize);
	_cells.reset(new std::vector<CompactSpan>[_xsize * _zsize]);
	
	// Fill in cells and spans.
	for (int z = 0; z < _zsize; ++z)
	{
		for (int x = 0; x < _xsize; ++x)
		{
			std::list<Span> const & spans = hf.getSpans(x, z);
			
			if (!spans.empty())
			{
				std::vector<CompactSpan> & vspan = _cells[x + z * _xsize];
				for(std::list<Span>::const_iterator next = spans.begin(), s = next++; s != spans.end(); s = (next != spans.end()) ? next++ : next)
				{
					if (!s->_walkable) continue;
					
					const int bottom = s->_smax;
					const int height = ((next == spans.end()) ? INT_MAX : next->_smin) - bottom;

					if (height < _walkableHeight ) continue;
					
					vspan.push_back(CompactSpan(bottom, height));
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
						if (isWalkable(s, ns, walkableHeight, walkableClimb))
							s.neighbours[dir] = &ns;
					}
				}
			}
		}
	}
}

}
