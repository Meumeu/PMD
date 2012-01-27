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
#include "Recast.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"

namespace Recast
{
/*rcCompactHeightfield* rcAllocCompactHeightfield()
{
	rcCompactHeightfield* chf = (rcCompactHeightfield*)rcAlloc(sizeof(rcCompactHeightfield), RC_ALLOC_PERM);
	memset(chf, 0, sizeof(rcCompactHeightfield));
	return chf;
}

void rcFreeCompactHeightfield(rcCompactHeightfield* chf)
{
	if (!chf) return;
	rcFree(chf->cells);
	rcFree(chf->spans);
	rcFree(chf->dist);
	rcFree(chf->areas);
	rcFree(chf);
}*/

/// @par
///
/// This is just the beginning of the process of fully building a compact heightfield.
/// Various filters may be applied applied, then the distance field and regions built.
/// E.g: #rcBuildDistanceField and #rcBuildRegions
///
/// See the #rcConfig documentation for more information on the configuration parameters.
///
/// @see rcAllocCompactHeightfield, rcHeightfield, rcCompactHeightfield, rcConfig
//bool rcBuildCompactHeightfield(rcContext* ctx, const int walkableHeight, const int walkableClimb,
//							   Heightfield& hf, CompactHeightfield& chf)
CompactHeightfield::CompactHeightfield(rcContext* ctx, const int walkableHeight, const int walkableClimb, Heightfield& hf) :
	_walkableHeight(walkableHeight),
	_walkableClimb(walkableClimb),
	_borderSize(0),
	_maxDistance(0),
	_maxRegions(0),
	_cs(hf._cs),
	_ch(hf._ch)
{
	rcAssert(ctx);
	
	ctx->startTimer(RC_TIMER_BUILD_COMPACTHEIGHTFIELD);
	
	const int w = hf.GetWidth();
	const int h = hf.GetHeight();
	const int spanCount = rcGetHeightFieldSpanCount(ctx, hf);

	// Fill in header.
	_width = w;
	_height = h;
	_spanCount = spanCount;
	rcVcopy(_bmin, hf._bmin);
	rcVcopy(_bmax, hf._bmax);
	_bmax[1] += walkableHeight*hf._ch;
	
	_cells = boost::scoped_array<CompactCell>(new CompactCell[w*h]);
	memset(_cells, 0, sizeof(rcCompactCell)*w*h);

	_spans = boost::scoped_array<CompactSpan>(new CompactSpan[spanCount]);
	memset(_spans, 0, sizeof(rcCompactSpan)*spanCount);

	_areas = boost::scoped_array<unsigned char>(new unsigned char[spanCount]);
	memset(_areas, RC_NULL_AREA, sizeof(unsigned char)*spanCount);
	
	const int MAX_HEIGHT = 0xffff;
	
	// Fill in cells and spans.
	int idx = 0;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const rcSpan* s = hf.spans[x + y*w];
			// If there are no spans at this cell, just leave the data to index=0, count=0.
			if (!s) continue;
			rcCompactCell& c = chf.cells[x+y*w];
			c.index = idx;
			c.count = 0;
			while (s)
			{
				if (s->area != RC_NULL_AREA)
				{
					const int bot = (int)s->smax;
					const int top = s->next ? (int)s->next->smin : MAX_HEIGHT;
					chf.spans[idx].y = (unsigned short)rcClamp(bot, 0, 0xffff);
					chf.spans[idx].h = (unsigned char)rcClamp(top - bot, 0, 0xff);
					chf.areas[idx] = s->area;
					idx++;
					c.count++;
				}
				s = s->next;
			}
		}
	}

	// Find neighbour connections.
	const int MAX_LAYERS = RC_NOT_CONNECTED-1;
	int tooHighNeighbour = 0;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const rcCompactCell& c = chf.cells[x+y*w];
			for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i)
			{
				rcCompactSpan& s = chf.spans[i];
				
				for (int dir = 0; dir < 4; ++dir)
				{
					rcSetCon(s, dir, RC_NOT_CONNECTED);
					const int nx = x + rcGetDirOffsetX(dir);
					const int ny = y + rcGetDirOffsetY(dir);
					// First check that the neighbour cell is in bounds.
					if (nx < 0 || ny < 0 || nx >= w || ny >= h)
						continue;
						
					// Iterate over all neighbour spans and check if any of the is
					// accessible from current cell.
					const rcCompactCell& nc = chf.cells[nx+ny*w];
					for (int k = (int)nc.index, nk = (int)(nc.index+nc.count); k < nk; ++k)
					{
						const rcCompactSpan& ns = chf.spans[k];
						const int bot = rcMax(s.y, ns.y);
						const int top = rcMin(s.y+s.h, ns.y+ns.h);

						// Check that the gap between the spans is walkable,
						// and that the climb height between the gaps is not too high.
						if ((top - bot) >= walkableHeight && rcAbs((int)ns.y - (int)s.y) <= walkableClimb)
						{
							// Mark direction as walkable.
							const int idx = k - (int)nc.index;
							if (idx < 0 || idx > MAX_LAYERS)
							{
								tooHighNeighbour = rcMax(tooHighNeighbour, idx);
								continue;
							}
							rcSetCon(s, dir, idx);
							break;
						}
					}
					
				}
			}
		}
	}
	
	if (tooHighNeighbour > MAX_LAYERS)
	{
		ctx->log(RC_LOG_ERROR, "rcBuildCompactHeightfield: Heightfield has too many layers %d (max: %d)",
				 tooHighNeighbour, MAX_LAYERS);
	}
		
	ctx->stopTimer(RC_TIMER_BUILD_COMPACTHEIGHTFIELD);
	
	return true;
}

}
