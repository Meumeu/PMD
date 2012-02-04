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
 
#ifndef RECAST_CONTOURSET_H
#define RECAST_CONTOURSET_H

#include "Recast.h"

namespace Recast
{
/// Represents a simple, non-overlapping contour in field space.
struct Contour
{
	struct Vertex
	{
		Vertex(int _x, int _y, int _z, int _flag = 0) :
			x(_x), y(_y), z(_z), flag(_flag) {}
		int x, y, z;
		int flag;
		
		bool operator<(Vertex const & v) const
		{
			return (x < v.x) || ((x == v.x) && (z < v.z));
		}
		
		bool operator>(Vertex const & v) const
		{
			return (x > v.x) || ((x == v.x) && (z > v.z));
		}
	};
	
	std::vector<Vertex> verts;      ///< Simplified contour vertex and connection data.
	std::vector<Vertex> rverts;     ///< Raw contour vertex and connection data.
	
	unsigned short reg;	///< The region id of the contour.
	unsigned char area;	///< The area id of the contour.
};

/// Represents a group of related contours.
/// @ingroup recast
class ContourSet
{
public:
	ContourSet(CompactHeightfield& chf, const float maxError, const int maxEdgeLen, const int buildFlags);
	
	std::vector<Contour> _conts;    ///< An array of the contours in the set.
private:
	
	float _cs;			///< The size of each cell. (On the xz-plane.)
	float _ch;			///< The height of each cell. (The minimum increment along the y-axis.)
	int _xsize;			///< The width of the set. (Along the x-axis in cell units.) 
	int _zsize;			///< The height of the set. (Along the z-axis in cell units.) 
	int _borderSize;		///< The AABB border size used to generate the source data from which the contours were derived.
};
}

#endif
