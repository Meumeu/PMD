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
#include <climits>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <boost/foreach.hpp>

#include "Recast.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"
#include "RecastCompactHeightfield.h"

namespace Recast
{
/*static void insertSort(unsigned char* a, const int n)
{
	int i, j;
	for (i = 1; i < n; i++)
	{
		const unsigned char value = a[i];
		for (j = i - 1; j >= 0 && a[j] > value; j--)
			a[j+1] = a[j];
		a[j+1] = value;
	}
}*/

/// @par 
/// 
/// Basically, any spans that are closer to a boundary or obstruction than the specified radius 
/// are marked as unwalkable.
///
/// This method is usually called immediately after the heightfield has been built.
///
/// @see rcCompactHeightfield, rcBuildCompactHeightfield, rcConfig::walkableRadius

void CompactHeightfield::erodeWalkableArea(int radius)
{
	// Mark boundary cells
	for(int i = 0; i < _xsize * _zsize; i++)
	{
		std::vector<CompactSpan> & cell = _cells[i];
		
		BOOST_FOREACH(CompactSpan & span, cell)
		{
			if (span._walkable)
			{
				// Init distance
				span._borderDistance = USHRT_MAX;
				
				for(int dir = Direction::Begin; dir < Direction::End; dir++)
				{
					if (!span.neighbours[dir] || !span.neighbours[dir]->_walkable)
						span._walkable = 0; // At least one missing or non walkable neighbour (boundary)
				}
			}
			else
			{
				span._walkable = 0;
			}
		}
	}
	
	// Pass 1
	for(int z = 0; z < _zsize; z++)
	{
		for(int x = 0; x < _xsize; x++)
		{
			std::vector<CompactSpan>& c = _cells[x + z * _xsize];
			BOOST_FOREACH(CompactSpan& span, c)
			{
				const CompactSpan * n;
				const CompactSpan * nn;
				if ((n = span.neighbours[Direction::Left]))
				{
					span._borderDistance = std::min(span._borderDistance, std::min<short unsigned int>(n->_borderDistance + 2, USHRT_MAX));
					
					if ((nn = n->neighbours[Direction::Backward]))
					{
						span._borderDistance = std::min(span._borderDistance, std::min<short unsigned int>(nn->_borderDistance + 3, USHRT_MAX));
					}
				}
				
				if ((n = span.neighbours[Direction::Backward]))
				{
					span._borderDistance = std::min(span._borderDistance, std::min<short unsigned int>(n->_borderDistance + 2, USHRT_MAX));
					
					if ((nn = n->neighbours[Direction::Right]))
					{
						span._borderDistance = std::min(span._borderDistance, std::min<short unsigned int>(nn->_borderDistance + 3, USHRT_MAX));
					}
				}
			}
		}
	}
	
	// Pass 2
	for(int z = _zsize - 1; z >= 0; z--)
	{
		for(int x = _xsize - 1; x >= 0; x--)
		{
			std::vector<CompactSpan>& c = _cells[x + z * _xsize];
			BOOST_FOREACH(CompactSpan& span, c)
			{
				const CompactSpan * n;
				const CompactSpan * nn;
				if ((n = span.neighbours[Direction::Right]))
				{
					span._borderDistance = std::min(span._borderDistance, std::min<short unsigned int>(n->_borderDistance + 2, USHRT_MAX));
					
					if ((nn = n->neighbours[Direction::Forward]))
					{
						span._borderDistance = std::min(span._borderDistance, std::min<short unsigned int>(nn->_borderDistance + 3, USHRT_MAX));
					}
				}
				
				if ((n = span.neighbours[Direction::Forward]))
				{
					span._borderDistance = std::min(span._borderDistance, std::min<short unsigned int>(n->_borderDistance + 2, USHRT_MAX));
					
					if ((nn = n->neighbours[Direction::Left]))
					{
						span._borderDistance = std::min(span._borderDistance, std::min<short unsigned int>(nn->_borderDistance + 3, USHRT_MAX));
					}
				}
			}
		}
	}

	const int thr = radius * 2;
	for(int i = 0; i < _xsize * _zsize; i++)
	{
		std::vector<CompactSpan> & cell = _cells[i];
		
		BOOST_FOREACH(CompactSpan & span, cell)
		{
			if (span._borderDistance < thr)
				span._walkable = false;
		}
	}
}

// TODO: le reste de ce fichier
#if 0

/// @par
///
/// This filter is usually applied after applying area id's using functions
/// such as #rcMarkBoxArea, #rcMarkConvexPolyArea, and #rcMarkCylinderArea.
/// 
/// @see rcCompactHeightfield
bool rcMedianFilterWalkableArea(rcContext* ctx, rcCompactHeightfield& chf)
{
	rcAssert(ctx);
	
	const int w = chf.width;
	const int h = chf.height;
	
	ctx->startTimer(RC_TIMER_MEDIAN_AREA);
	
	unsigned char* areas = (unsigned char*)rcAlloc(sizeof(unsigned char)*chf.spanCount, RC_ALLOC_TEMP);
	if (!areas)
	{
		ctx->log(RC_LOG_ERROR, "medianFilterWalkableArea: Out of memory 'areas' (%d).", chf.spanCount);
		return false;
	}
	
	// Init distance.
	memset(areas, 0xff, sizeof(unsigned char)*chf.spanCount);
	
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const rcCompactCell& c = chf.cells[x+y*w];
			for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i)
			{
				const rcCompactSpan& s = chf.spans[i];
				if (chf.areas[i] == RC_NULL_AREA)
				{
					areas[i] = chf.areas[i];
					continue;
				}
				
				unsigned char nei[9];
				for (int j = 0; j < 9; ++j)
					nei[j] = chf.areas[i];
				
				for (int dir = 0; dir < 4; ++dir)
				{
					if (rcGetCon(s, dir) != RC_NOT_CONNECTED)
					{
						const int ax = x + rcGetDirOffsetX(dir);
						const int ay = y + rcGetDirOffsetY(dir);
						const int ai = (int)chf.cells[ax+ay*w].index + rcGetCon(s, dir);
						if (chf.areas[ai] != RC_NULL_AREA)
							nei[dir*2+0] = chf.areas[ai];
						
						const rcCompactSpan& as = chf.spans[ai];
						const int dir2 = (dir+1) & 0x3;
						if (rcGetCon(as, dir2) != RC_NOT_CONNECTED)
						{
							const int ax2 = ax + rcGetDirOffsetX(dir2);
							const int ay2 = ay + rcGetDirOffsetY(dir2);
							const int ai2 = (int)chf.cells[ax2+ay2*w].index + rcGetCon(as, dir2);
							if (chf.areas[ai2] != RC_NULL_AREA)
								nei[dir*2+1] = chf.areas[ai2];
						}
					}
				}
				insertSort(nei, 9);
				areas[i] = nei[4];
			}
		}
	}
	
	memcpy(chf.areas, areas, sizeof(unsigned char)*chf.spanCount);
	
	rcFree(areas);

	ctx->stopTimer(RC_TIMER_MEDIAN_AREA);
	
	return true;
}

/// @par
///
/// The value of spacial parameters are in world units.
/// 
/// @see rcCompactHeightfield, rcMedianFilterWalkableArea
void rcMarkBoxArea(rcContext* ctx, const float* bmin, const float* bmax, unsigned char areaId,
				   rcCompactHeightfield& chf)
{
	rcAssert(ctx);
	
	ctx->startTimer(RC_TIMER_MARK_BOX_AREA);

	int minx = (int)((bmin[0]-chf.bmin[0])/chf.cs);
	int miny = (int)((bmin[1]-chf.bmin[1])/chf.ch);
	int minz = (int)((bmin[2]-chf.bmin[2])/chf.cs);
	int maxx = (int)((bmax[0]-chf.bmin[0])/chf.cs);
	int maxy = (int)((bmax[1]-chf.bmin[1])/chf.ch);
	int maxz = (int)((bmax[2]-chf.bmin[2])/chf.cs);
	
	if (maxx < 0) return;
	if (minx >= chf.width) return;
	if (maxz < 0) return;
	if (minz >= chf.height) return;

	if (minx < 0) minx = 0;
	if (maxx >= chf.width) maxx = chf.width-1;
	if (minz < 0) minz = 0;
	if (maxz >= chf.height) maxz = chf.height-1;	
	
	for (int z = minz; z <= maxz; ++z)
	{
		for (int x = minx; x <= maxx; ++x)
		{
			const rcCompactCell& c = chf.cells[x+z*chf.width];
			for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i)
			{
				rcCompactSpan& s = chf.spans[i];
				if ((int)s.y >= miny && (int)s.y <= maxy)
				{
					if (chf.areas[i] != RC_NULL_AREA)
						chf.areas[i] = areaId;
				}
			}
		}
	}

	ctx->stopTimer(RC_TIMER_MARK_BOX_AREA);

}

static int pointInPoly(int nvert, const float* verts, const float* p)
{
	int i, j, c = 0;
	for (i = 0, j = nvert-1; i < nvert; j = i++)
	{
		const float* vi = &verts[i*3];
		const float* vj = &verts[j*3];
		if (((vi[2] > p[2]) != (vj[2] > p[2])) &&
			(p[0] < (vj[0]-vi[0]) * (p[2]-vi[2]) / (vj[2]-vi[2]) + vi[0]) )
			c = !c;
	}
	return c;
}

/// @par
///
/// The value of spacial parameters are in world units.
/// 
/// The y-values of the polygon vertices are ignored. So the polygon is effectively 
/// projected onto the xz-plane at @p hmin, then extruded to @p hmax.
/// 
/// @see rcCompactHeightfield, rcMedianFilterWalkableArea
void rcMarkConvexPolyArea(rcContext* ctx, const float* verts, const int nverts,
						  const float hmin, const float hmax, unsigned char areaId,
						  rcCompactHeightfield& chf)
{
	rcAssert(ctx);
	
	ctx->startTimer(RC_TIMER_MARK_CONVEXPOLY_AREA);

	float bmin[3], bmax[3];
	rcVcopy(bmin, verts);
	rcVcopy(bmax, verts);
	for (int i = 1; i < nverts; ++i)
	{
		rcVmin(bmin, &verts[i*3]);
		rcVmax(bmax, &verts[i*3]);
	}
	bmin[1] = hmin;
	bmax[1] = hmax;

	int minx = (int)((bmin[0]-chf.bmin[0])/chf.cs);
	int miny = (int)((bmin[1]-chf.bmin[1])/chf.ch);
	int minz = (int)((bmin[2]-chf.bmin[2])/chf.cs);
	int maxx = (int)((bmax[0]-chf.bmin[0])/chf.cs);
	int maxy = (int)((bmax[1]-chf.bmin[1])/chf.ch);
	int maxz = (int)((bmax[2]-chf.bmin[2])/chf.cs);
	
	if (maxx < 0) return;
	if (minx >= chf.width) return;
	if (maxz < 0) return;
	if (minz >= chf.height) return;
	
	if (minx < 0) minx = 0;
	if (maxx >= chf.width) maxx = chf.width-1;
	if (minz < 0) minz = 0;
	if (maxz >= chf.height) maxz = chf.height-1;	
	
	
	// TODO: Optimize.
	for (int z = minz; z <= maxz; ++z)
	{
		for (int x = minx; x <= maxx; ++x)
		{
			const rcCompactCell& c = chf.cells[x+z*chf.width];
			for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i)
			{
				rcCompactSpan& s = chf.spans[i];
				if (chf.areas[i] == RC_NULL_AREA)
					continue;
				if ((int)s.y >= miny && (int)s.y <= maxy)
				{
					float p[3];
					p[0] = chf.bmin[0] + (x+0.5f)*chf.cs; 
					p[1] = 0;
					p[2] = chf.bmin[2] + (z+0.5f)*chf.cs; 

					if (pointInPoly(nverts, verts, p))
					{
						chf.areas[i] = areaId;
					}
				}
			}
		}
	}

	ctx->stopTimer(RC_TIMER_MARK_CONVEXPOLY_AREA);
}

/// @par
///
/// The value of spacial parameters are in world units.
/// 
/// @see rcCompactHeightfield, rcMedianFilterWalkableArea
void rcMarkCylinderArea(rcContext* ctx, const float* pos,
						const float r, const float h, unsigned char areaId,
						rcCompactHeightfield& chf)
{
	rcAssert(ctx);
	
	ctx->startTimer(RC_TIMER_MARK_CYLINDER_AREA);
	
	float bmin[3], bmax[3];
	bmin[0] = pos[0] - r;
	bmin[1] = pos[1];
	bmin[2] = pos[2] - r;
	bmax[0] = pos[0] + r;
	bmax[1] = pos[1] + h;
	bmax[2] = pos[2] + r;
	const float r2 = r*r;
	
	int minx = (int)((bmin[0]-chf.bmin[0])/chf.cs);
	int miny = (int)((bmin[1]-chf.bmin[1])/chf.ch);
	int minz = (int)((bmin[2]-chf.bmin[2])/chf.cs);
	int maxx = (int)((bmax[0]-chf.bmin[0])/chf.cs);
	int maxy = (int)((bmax[1]-chf.bmin[1])/chf.ch);
	int maxz = (int)((bmax[2]-chf.bmin[2])/chf.cs);
	
	if (maxx < 0) return;
	if (minx >= chf.width) return;
	if (maxz < 0) return;
	if (minz >= chf.height) return;
	
	if (minx < 0) minx = 0;
	if (maxx >= chf.width) maxx = chf.width-1;
	if (minz < 0) minz = 0;
	if (maxz >= chf.height) maxz = chf.height-1;	
	
	
	for (int z = minz; z <= maxz; ++z)
	{
		for (int x = minx; x <= maxx; ++x)
		{
			const rcCompactCell& c = chf.cells[x+z*chf.width];
			for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i)
			{
				rcCompactSpan& s = chf.spans[i];
				
				if (chf.areas[i] == RC_NULL_AREA)
					continue;
				
				if ((int)s.y >= miny && (int)s.y <= maxy)
				{
					const float sx = chf.bmin[0] + (x+0.5f)*chf.cs; 
					const float sz = chf.bmin[2] + (z+0.5f)*chf.cs; 
					const float dx = sx - pos[0];
					const float dz = sz - pos[2];
					
					if (dx*dx + dz*dz < r2)
					{
						chf.areas[i] = areaId;
					}
				}
			}
		}
	}
	
	ctx->stopTimer(RC_TIMER_MARK_CYLINDER_AREA);
}
#endif
}