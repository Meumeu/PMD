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

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include "Recast.h"
#include "RecastAssert.h"
#include <boost/foreach.hpp>

namespace Recast
{
/// @par
///
/// Allows the formation of walkable regions that will flow over low lying 
/// objects such as curbs, and up structures such as stairways. 
/// 
/// Two neighboring spans are walkable if: <tt>rcAbs(currentSpan.smax - neighborSpan.smax) < waklableClimb</tt>
/// 
/// @warning Will override the effect of #rcFilterLedgeSpans.  So if both filters are used, call
/// #rcFilterLedgeSpans after calling this filter. 
///
/// @see rcHeightfield, rcConfig
void Heightfield::FilterLowHangingWalkableObstacles(rcContext* ctx, const int walkableClimb)
{
	rcAssert(ctx);

	ctx->startTimer(RC_TIMER_FILTER_LOW_OBSTACLES);
	
	BOOST_FOREACH(Heightfield::SpanMap::value_type& i, _spans)
	{
		int x = i.first.first;
		int y = i.first.second;

		Span* ps = 0;
		bool previousWalkable = false;
		unsigned char previousArea = RC_NULL_AREA;

		BOOST_FOREACH(Span& s, i.second)
		{
			const bool walkable = s._area != RC_NULL_AREA;
			// If current span is not walkable, but there is walkable
			// span just below it, mark the span above it walkable too.
			if (!walkable && previousWalkable)
			{
				if (rcAbs((int)s._smax - (int)ps->_smax) <= walkableClimb)
					s._area = previousArea;
			}
			// Copy walkable flag so that it cannot propagate
			// past multiple non-walkable objects.
			previousWalkable = walkable;
			previousArea = s._area;

			ps = &s;
		}
	}

	ctx->stopTimer(RC_TIMER_FILTER_LOW_OBSTACLES);
}

/// @par
///
/// A ledge is a span with one or more neighbors whose maximum is further away than @p walkableClimb
/// from the current span's maximum.
/// This method removes the impact of the overestimation of conservative voxelization 
/// so the resulting mesh will not have regions hanging in the air over ledges.
/// 
/// A span is a ledge if: <tt>rcAbs(currentSpan.smax - neighborSpan.smax) > walkableClimb</tt>
/// 
/// @see rcHeightfield, rcConfig
void Heightfield::FilterLedgeSpans(rcContext* ctx, const int walkableHeight, const int walkableClimb)
{
	rcAssert(ctx);
	
	ctx->startTimer(RC_TIMER_FILTER_BORDER);

	const int MAX_HEIGHT = 0xffff;
	
	BOOST_FOREACH(Heightfield::SpanMap::value_type& i, _spans)
	{
		int x = i.first.first;
		int y = i.first.second;
		for(std::list<Span>::iterator s = i.second.begin(), end = i.second.end();
			s != end;
			++s)
		{
			// Skip non walkable spans.
			if (s->_area == RC_NULL_AREA)
				continue;
			
			const int bot = (int)(s->_smax);
			std::list<Span>::iterator next = s; ++next;
			const int top = (next != end) ? (int)(next->_smin) : MAX_HEIGHT;
			
			// Find neighbours minimum height.
			int minh = MAX_HEIGHT;

			// Min and max height of accessible neighbours.
			int asmin = s->_smax;
			int asmax = s->_smax;

			for (int dir = 0; dir < 4; ++dir)
			{
				int dx = x + rcGetDirOffsetX(dir);
				int dy = y + rcGetDirOffsetY(dir);
				Heightfield::SpanMap::iterator SpanList = _spans.find(std::make_pair(dx, dy));

				// Skip neighbours which are out of bounds.
				if (dx < 0 || dy < 0 || dx >= _width || dy >= _height)
				{
					minh = rcMin(minh, -walkableClimb - bot);
					continue;
				}

				// From minus infinity to the first span.
				Heightfield::SpanMap::value_type::second_type::iterator ns = SpanList->second.begin();
				
				int nbot = -walkableClimb;
				int ntop = (ns != SpanList->second.end()) ? (int)ns->_smin : MAX_HEIGHT;
				// Skip neightbour if the gap between the spans is too small.
				if (rcMin(top,ntop) - rcMax(bot,nbot) > walkableHeight)
					minh = rcMin(minh, nbot - bot);
				
				// Rest of the spans.
				for (Heightfield::SpanMap::value_type::second_type::iterator ns = SpanList->second.begin(), end = SpanList->second.end();
					ns != end;
					++ns)
				{
					nbot = (int)ns->_smax;
					Heightfield::SpanMap::value_type::second_type::iterator ns_next = ns; ++ns_next;
					ntop = (ns_next != end) ? (int)ns_next->_smin : MAX_HEIGHT;
					// Skip neightbour if the gap between the spans is too small.
					if (rcMin(top,ntop) - rcMax(bot,nbot) > walkableHeight)
					{
						minh = rcMin(minh, nbot - bot);
					
						// Find min/max accessible neighbour height. 
						if (rcAbs(nbot - bot) <= walkableClimb)
						{
							if (nbot < asmin) asmin = nbot;
							if (nbot > asmax) asmax = nbot;
						}
						
					}
				}
			}
			
			// The current span is close to a ledge if the drop to any
			// neighbour span is less than the walkableClimb.
			if (minh < -walkableClimb)
				s->_area = RC_NULL_AREA;
				
			// If the difference between all neighbours is too large,
			// we are at steep slope, mark the span as ledge.
			if ((asmax - asmin) > walkableClimb)
			{
				s->_area = RC_NULL_AREA;
			}
		}
	}

	// Mark border spans.
	/*for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			for (rcSpan* s = solid.spans[x + y*w]; s; s = s->next)
			{
			}
		}
	}*/
	
	ctx->stopTimer(RC_TIMER_FILTER_BORDER);
}	

/// @par
///
/// For this filter, the clearance above the span is the distance from the span's 
/// maximum to the next higher span's minimum. (Same grid column.)
/// 
/// @see rcHeightfield, rcConfig
void Heightfield::FilterWalkableLowHeightSpans(rcContext* ctx, int walkableHeight)
{
	rcAssert(ctx);
	
	ctx->startTimer(RC_TIMER_FILTER_WALKABLE);
	
	const int MAX_HEIGHT = 0xffff;
	
	// Remove walkable flag from spans which do not have enough
	// space above them for the agent to stand there.
	BOOST_FOREACH(Heightfield::SpanMap::value_type& i, _spans)
	{
		for(std::list<Span>::iterator s = i.second.begin(), end = i.second.end();
			s != end;
			++s)
		{
			std::list<Span>::iterator s_next = s; ++s_next;
			const int bot = (int)(s->_smax);
			const int top = (s_next != end) ? (int)(s_next->_smin) : MAX_HEIGHT;
			if ((top - bot) <= walkableHeight)
				s->_area = RC_NULL_AREA;
		}
	}
	
	ctx->stopTimer(RC_TIMER_FILTER_WALKABLE);
}
}