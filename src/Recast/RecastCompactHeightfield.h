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

#ifndef RECAST_COMPACTHEIGHTFIELD_H
#define RECAST_COMPACTHEIGHTFIELD_H

#include <boost/scoped_array.hpp>

namespace Recast
{
class Heightfield;

/// Represents a span of unobstructed space within a compact heightfield.
struct CompactSpan
{
	unsigned short y;			///< The lower extent of the span. (Measured from the heightfield's base.)
	unsigned short reg;			///< The id of the region the span belongs to. (Or zero if not in a region.)
	//unsigned int con : 24;		///< Packed neighbor connection data.
	unsigned int h : 8;			///< The height of the span.  (Measured from #y.)
	bool walkable;
	unsigned short dist;			///< Border distance data (???)
	
	const CompactSpan * neighbours[4];
};

/// A compact, static heightfield representing unobstructed space.
/// @ingroup recast
class CompactHeightfield
{
public:
	CompactHeightfield(const int walkableHeight, const int walkableClimb, const Heightfield& hf);
	
private:
	CompactHeightfield(const CompactHeightfield&) { abort(); };
	CompactHeightfield & operator=(const CompactHeightfield&) { abort(); };
	
	int _xmin;
	int _zmin;
	int _xsize;					///< The width of the heightfield. (Along the x-axis in cell units.)
	int _zsize;					///< The height of the heightfield. (Along the z-axis in cell units.)
	int _walkableHeight;			///< The walkable height used during the build of the field.  (See: rcConfig::walkableHeight)
	int _walkableClimb;			///< The walkable climb used during the build of the field. (See: rcConfig::walkableClimb)
	int _borderSize;				///< The AABB border size used during the build of the field. (See: rcConfig::borderSize)
	unsigned short _maxDistance;	///< The maximum distance value of any span within the field. 
	unsigned short _maxRegions;	///< The maximum region id of any span within the field. 
	//float _bmin[3];				///< The minimum bounds in world space. [(x, y, z)]
	//float _bmax[3];				///< The maximum bounds in world space. [(x, y, z)]
	float _cs;					///< The size of each cell. (On the xz-plane.)
	float _ch;					///< The height of each cell. (The minimum increment along the y-axis.)
	boost::scoped_array<std::vector<CompactSpan> > _cells;		///< Array of cells. [Size: #width*#height]
};
}

#endif
