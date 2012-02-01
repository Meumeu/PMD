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
#include <vector>
#include <map>

#include "Recast.h"

namespace Recast
{
class Heightfield;

/// Represents a span of unobstructed space within a compact heightfield.
struct CompactSpan
{
	CompactSpan(int bottom, int height, bool walkable = true):
		_bottom(bottom), _height(height), _regionID(0), _walkable(walkable), _flag(0) {}
	int _bottom;                     ///< The lower extent of the span. (Measured from the heightfield's base.)
	int _height;                     ///< The height of the span.  (Measured from #_bottom.)
	unsigned int _regionID;          ///< The id of the region the span belongs to. (Or zero if not in a region.)
	bool _walkable;
	char _flag;
	unsigned short _borderDistance;   ///< Border distance data (number of cells x2, sqrt(2) == 1.5)
	
	CompactSpan * neighbours[Direction::End];
};

/// A compact, static heightfield representing unobstructed space.
/// @ingroup recast
class CompactHeightfield
{
	friend class ContourSet;
public:
/// Builds a compact heightfield representing open space, from a heightfield representing solid space.
///  @ingroup recast
///  @param[in]		walkableHeight	Minimum floor to 'ceiling' height that will still allow the floor area 
///  								to be considered walkable. [Limit: >= 3 * cs] [Units: wu]
///  @param[in]		walkableClimb	Maximum ledge height that is considered to still be traversable. 
///  								[Limit: >=0] [Units: wu]
///  @param[in]		hf				The heightfield to be compacted.
	CompactHeightfield(const float walkableHeight, const float walkableClimb, const Heightfield& hf, const float radius,
			   const bool filterLowHangingWalkableObstacles = true, const bool filterLedgeSpans = true,
			   const int borderSize = 1);
	
	typedef std::map<CompactSpan *, unsigned int> IntMap;
	void buildRegions(const int minRegionArea, const int mergeRegionSize);

private:
	struct Region;
	
	CompactHeightfield(const CompactHeightfield&);
	CompactHeightfield & operator=(const CompactHeightfield&);
	
	// The following functions are the different steps performed by buildRegions()
	void buildDistanceField();
	void blurDistanceField(IntMap& out, int threshold = 1);
	bool fillRegion(Recast::CompactSpan& span, unsigned int level, unsigned int region, Recast::CompactHeightfield::IntMap& regionMap, Recast::CompactHeightfield::IntMap& distanceMap);
	void paintRectRegion(const int minx, const int maxx, const int minz, const int maxz, const unsigned int regionID, IntMap& regionMap);
	void expandRegions(const int maxIter, const unsigned int level, IntMap& regionMap, IntMap& distanceMap);
	void findConnections(std::vector<Region> & regions, IntMap& regionMap);
	void removeTooSmallRegions(std::vector<Region> & regions, const int minRegionArea);
	void mergeTooSmallRegions(std::vector<Region> & regions, const int mergeRegionSize);
	void compressRegionId(std::vector<Region> & regions, IntMap& regionMap);
	void filterSmallRegions(const int minRegionArea, const int mergeRegionSize, IntMap& regionMap);
	
	
	void erodeWalkableArea(int radius);
public:
	int _xmin;
	int _zmin;
	int _xsize;					///< The width of the heightfield. (Along the x-axis in cell units.)
	int _zsize;					///< The height of the heightfield. (Along the z-axis in cell units.)
	int _walkableHeight;			///< The walkable height used during the build of the field.  (See: rcConfig::walkableHeight)
	int _walkableClimb;			///< The walkable climb used during the build of the field. (See: rcConfig::walkableClimb)
	int _borderSize;				///< The AABB border size used during the build of the field. (See: rcConfig::borderSize)
	unsigned short _maxDistance;	///< The maximum distance value of any span within the field. 
	unsigned short _maxRegions;	///< The maximum region id of any span within the field. 
	float _cs;					///< The size of each cell. (On the xz-plane.)
	float _ch;					///< The height of each cell. (The minimum increment along the y-axis.)
	boost::scoped_array<std::vector<CompactSpan> > _cells;		///< Array of cells. [Size: #width*#height]
};
}

#endif
