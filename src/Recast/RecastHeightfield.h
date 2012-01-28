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

#ifndef RECAST_HEIGHTFIELD_H
#define RECAST_HEIGHTFIELD_H

#include <map>
#include <list>

#include <OgreAxisAlignedBox.h>

namespace Recast {

/// Represents a span in a heightfield.
/// @see rcHeightfield
struct Span
{
	Span(unsigned int smin, unsigned int smax, bool walkable):
	_smin(smin), _smax(smax), _walkable(walkable) {}

	void merge(Span const& other, const int flagMergeThr);

	unsigned int _smin; ///< The lower limit of the span.
	unsigned int _smax; ///< The upper limit of the span.
	bool _walkable;
	
	struct
	{
		int x, y, z;
	} _neighbours[4];
};

class rcContext;

/// A dynamic heightfield representing obstructed space.
/// @ingroup recast
class Heightfield
{
public:
	/// @param[in] walkableSlopeAngle maximum angle (in radians) a unit can walk
	Heightfield(float cellSize, float cellHeight, float walkableSlopeAngle);
	
	/// Get the list of spans
	std::list<Span> const& getSpans(int x, int z) const;
	
	/// Get cell size
	float getCellSize() const { return _cs; }
	
	/// Get cell height
	float getCellHeight() const { return _ch; }
	
	/// Get bounding box
	void getOffsetAndSize(int& xmin, int& xsize, int& zmin, int& zsize) const
	{
		xmin = _xmin;
		zmin = _zmin;
		xsize = _xmax - _xmin + 1;
		zsize = _zmax - _zmin + 1;
	}
	
	/// Rasterizes a triangle into the heightfield.
	///  @ingroup recast
	///  @param[in]		v0				Triangle vertex 0
	///  @param[in]		v1				Triangle vertex 1
	///  @param[in]		v2				Triangle vertex 2
	///  @param[in]		area			The area id of the triangle. [Limit: <= #RC_WALKABLE_AREA]
	///  @param[in]		flagMergeThr	The distance where the walkable flag is favored over the non-walkable flag.
	///  								[Limit: >= 0] [Units: vx]
	void rasterizeTriangle(Ogre::Vector3 const& v0, Ogre::Vector3 const& v1, Ogre::Vector3 const& v2,
		const int flagMergeThr = 1);
	
	/// Marks non-walkable spans as walkable if their maximum is within @p walkableClimb of a walkable neihbor.
	///  @ingroup recast
	///  @param[in,out]	ctx				The build context to use during the operation.
	///  @param[in]		walkableClimb	Maximum ledge height that is considered to still be traversable.
	///  								[Limit: >=0] [Units: vx]
	void filterLowHangingWalkableObstacles(rcContext* ctx, const int walkableClimb);
	
	/// Marks spans that are ledges as not-walkable.
	///  @ingroup recast
	///  @param[in,out]	ctx				The build context to use during the operation.
	///  @param[in]		walkableHeight	Minimum floor to 'ceiling' height that will still allow the floor area to
	///  								be considered walkable. [Limit: >= 3] [Units: vx]
	///  @param[in]		walkableClimb	Maximum ledge height that is considered to still be traversable.
	///  								[Limit: >=0] [Units: vx]
	void filterLedgeSpans(rcContext* ctx, const int walkableHeight, const int walkableClimb);
	
	/// Marks walkable spans as not walkable if the clearence above the span is less than the specified height.
	///  @ingroup recast
	///  @param[in,out]	ctx				The build context to use during the operation.
	///  @param[in]		walkableHeight	Minimum floor to 'ceiling' height that will still allow the floor area to
	///  								be considered walkable. [Limit: >= 3] [Units: vx]
	void filterWalkableLowHeightSpans(rcContext* ctx, int walkableHeight);

private:
	
	/// Adds a span to the heightfield.
	///  @ingroup recast
	///  @param[in,out] ctx          The build context to use during the operation.
	///  @param[in,out] hf           An initialized heightfield.
	///  @param[in]     x            The width index where the span is to be added.
	///[Limits: 0 <= value < rcHeightfield::width]
	///  @param[in]     y            The height index where the span is to be added.
	///[Limits: 0 <= value < rcHeightfield::height]
	///  @param[in]     smin         The minimum height of the span. [Limit: < @p smax] [Units: vx]
	///  @param[in]     smax         The maximum height of the span. [Limit: <= #RC_SPAN_MAX_HEIGHT] [Units: vx]
	///  @param[in]     area         The area id of the span. [Limit: <= #RC_WALKABLE_AREA)
	///  @param[in]     flagMergeThr The merge theshold. [Limit: >= 0] [Units: vx]
	void addSpan(const int x, const int y,
		const unsigned short smin, const unsigned short smax,
		bool walkable, const int flagMergeThr);
	
	int _xmin, _xmax;
	int _zmin, _zmax;
	float _cs;			///< The size of each cell. (On the xz-plane.)
	float _ch;			///< The height of each cell. (The minimum increment along the y-axis.)
	float _cosWalkableAngle;
	std::map<std::pair<int,int>, std::list<Span> > _spans; ///< Heightfield of spans (width*height).
};
}

#endif