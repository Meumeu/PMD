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
	Span(unsigned int smin, unsigned int smax, unsigned int area):
	_smin(smin), _smax(smax), _area(area) {}

	void merge(Span const& other, const int flagMergeThr);

	unsigned int _smin : 13; ///< The lower limit of the span. [Limit: < #smax]
	unsigned int _smax : 13; ///< The upper limit of the span. [Limit: <= #RC_SPAN_MAX_HEIGHT]
	unsigned int _area : 6;  ///< The area id assigned to the span.
};

/// A dynamic heightfield representing obstructed space.
/// @ingroup recast
class Heightfield
{
public:
	Heightfield(Ogre::AxisAlignedBox const& boundingBox, float cellSize, float cellHeight);

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
		const unsigned char area, const int flagMergeThr);

	/// Gets the total number of spans in the heightfield.
	unsigned int getSpanCount();
	
	/// Rasterizes a triangle into the heightfield.
	///  @ingroup recast
	///  @param[in]		v0				Triangle vertex 0
	///  @param[in]		v1				Triangle vertex 1
	///  @param[in]		v2				Triangle vertex 2
	///  @param[in]		area			The area id of the triangle. [Limit: <= #RC_WALKABLE_AREA]
	///  @param[in]		flagMergeThr	The distance where the walkable flag is favored over the non-walkable flag.
	///  								[Limit: >= 0] [Units: vx]
	void rasterizeTriangle(Ogre::Vector3 const& v0, Ogre::Vector3 const& v1, Ogre::Vector3 const& v2,
		const unsigned char area, const int flagMergeThr = 1);
private:
	int _width;			///< The width of the heightfield. (Along the x-axis in cell units.)
	int _height;			///< The height of the heightfield. (Along the z-axis in cell units.)

	//FIXME: boundingbox is probably useless now
	Ogre::AxisAlignedBox _boundingBox; ///< Bounding box (world units)
	//float bmin[3];  	///< The minimum bounds in world space. [(x, y, z)]
	//float bmax[3];		///< The maximum bounds in world space. [(x, y, z)]
	float _cs;			///< The size of each cell. (On the xz-plane.)
	float _ch;			///< The height of each cell. (The minimum increment along the y-axis.)
	std::map<std::pair<int,int>, std::list<Span> > _spans; ///< Heightfield of spans (width*height).
};
}

#endif