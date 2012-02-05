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
 
#ifndef RECAST_H
#define RECAST_H
#include <cmath>
#include <vector>
#include <stdexcept>

namespace Recast
{

class Heightfield;
class CompactHeightfield;
class ContourSet;

/// The value of PI used by Recast.
static const float RC_PI = 3.14159265f;

/// Recast log categories.
/// @see rcContext
enum rcLogCategory
{
	RC_LOG_PROGRESS = 1,	///< A progress log entry.
	RC_LOG_WARNING,			///< A warning log entry.
	RC_LOG_ERROR,			///< An error log entry.
};

/// Recast performance timer categories.
/// @see rcContext
enum rcTimerLabel
{
	/// The user defined total time of the build.
	RC_TIMER_TOTAL,
	/// A user defined build time.
	RC_TIMER_TEMP,
	/// The time to rasterize the triangles. (See: #rcRasterizeTriangle)
	RC_TIMER_RASTERIZE_TRIANGLES,
	/// The time to build the compact heightfield. (See: #rcBuildCompactHeightfield)
	RC_TIMER_BUILD_COMPACTHEIGHTFIELD,
	/// The total time to build the contours. (See: #rcBuildContours)
	RC_TIMER_BUILD_CONTOURS,
	/// The time to trace the boundaries of the contours. (See: #rcBuildContours)
	RC_TIMER_BUILD_CONTOURS_TRACE,
	/// The time to simplify the contours. (See: #rcBuildContours)
	RC_TIMER_BUILD_CONTOURS_SIMPLIFY,
	/// The time to filter ledge spans. (See: #rcFilterLedgeSpans)
	RC_TIMER_FILTER_BORDER,
	/// The time to filter low height spans. (See: #rcFilterWalkableLowHeightSpans)
	RC_TIMER_FILTER_WALKABLE,
	/// The time to apply the median filter. (See: #rcMedianFilterWalkableArea)
	RC_TIMER_MEDIAN_AREA,
	/// The time to filter low obstacles. (See: #rcFilterLowHangingWalkableObstacles)
	RC_TIMER_FILTER_LOW_OBSTACLES,
	/// The time to build the polygon mesh. (See: #rcBuildPolyMesh)
	RC_TIMER_BUILD_POLYMESH,
	/// The time to merge polygon meshes. (See: #rcMergePolyMeshes)
	RC_TIMER_MERGE_POLYMESH,
	/// The time to erode the walkable area. (See: #rcErodeWalkableArea)
	RC_TIMER_ERODE_AREA,
	/// The time to mark a box area. (See: #rcMarkBoxArea)
	RC_TIMER_MARK_BOX_AREA,
	/// The time to mark a cylinder area. (See: #rcMarkCylinderArea)
	RC_TIMER_MARK_CYLINDER_AREA,
	/// The time to mark a convex polygon area. (See: #rcMarkConvexPolyArea)
	RC_TIMER_MARK_CONVEXPOLY_AREA,
	/// The total time to build the distance field. (See: #rcBuildDistanceField)
	RC_TIMER_BUILD_DISTANCEFIELD,
	/// The time to build the distances of the distance field. (See: #rcBuildDistanceField)
	RC_TIMER_BUILD_DISTANCEFIELD_DIST,
	/// The time to blur the distance field. (See: #rcBuildDistanceField)
	RC_TIMER_BUILD_DISTANCEFIELD_BLUR,
	/// The total time to build the regions. (See: #rcBuildRegions, #rcBuildRegionsMonotone)
	RC_TIMER_BUILD_REGIONS,
	/// The total time to apply the watershed algorithm. (See: #rcBuildRegions)
	RC_TIMER_BUILD_REGIONS_WATERSHED,
	/// The time to expand regions while applying the watershed algorithm. (See: #rcBuildRegions)
	RC_TIMER_BUILD_REGIONS_EXPAND,
	/// The time to flood regions while applying the watershed algorithm. (See: #rcBuildRegions)
	RC_TIMER_BUILD_REGIONS_FLOOD,
	/// The time to filter out small regions. (See: #rcBuildRegions, #rcBuildRegionsMonotone)
	RC_TIMER_BUILD_REGIONS_FILTER,
	/// The time to build heightfield layers. (See: #rcBuildHeightfieldLayers)
	RC_TIMER_BUILD_LAYERS, 
	/// The time to build the polygon mesh detail. (See: #rcBuildPolyMeshDetail)
	RC_TIMER_BUILD_POLYMESHDETAIL,
	/// The time to merge polygon mesh details. (See: #rcMergePolyMeshDetails)
	RC_TIMER_MERGE_POLYMESHDETAIL,
	/// The maximum number of timers.  (Used for iterating timers.)
	RC_MAX_TIMERS
};

/// Specifies a configuration to use when performing Recast builds.
/// @ingroup recast
struct rcConfig
{
	/// The width of the field along the x-axis. [Limit: >= 0] [Units: vx]
	int width;

	/// The height of the field along the z-axis. [Limit: >= 0] [Units: vx]
	int height;
	
	/// The width/height size of tile's on the xz-plane. [Limit: >= 0] [Units: vx]
	int tileSize;
	
	/// The size of the non-navigable border around the heightfield. [Limit: >=0] [Units: vx]
	int borderSize;

	/// The xz-plane cell size to use for fields. [Limit: > 0] [Units: wu] 
	float cs;

	/// The y-axis cell size to use for fields. [Limit: > 0] [Units: wu]
	float ch;

	/// The minimum bounds of the field's AABB. [(x, y, z)] [Units: wu]
	float bmin[3]; 

	/// The maximum bounds of the field's AABB. [(x, y, z)] [Units: wu]
	float bmax[3];

	/// The maximum slope that is considered walkable. [Limits: 0 <= value < 90] [Units: Degrees] 
	float walkableSlopeAngle;

	/// Minimum floor to 'ceiling' height that will still allow the floor area to 
	/// be considered walkable. [Limit: >= 3] [Units: vx] 
	int walkableHeight;
	
	/// Maximum ledge height that is considered to still be traversable. [Limit: >=0] [Units: vx] 
	int walkableClimb;
	
	/// The distance to erode/shrink the walkable area of the heightfield away from 
	/// obstructions.  [Limit: >=0] [Units: vx] 
	int walkableRadius;
	
	/// The maximum allowed length for contour edges along the border of the mesh. [Limit: >=0] [Units: vx] 
	int maxEdgeLen;
	
	/// The maximum distance a simplfied contour's border edges should deviate 
	/// the original raw contour. [Limit: >=0] [Units: wu]
	float maxSimplificationError;
	
	/// The minimum number of cells allowed to form isolated island areas. [Limit: >=0] [Units: vx] 
	int minRegionArea;
	
	/// Any regions with a span count smaller than this value will, if possible, 
	/// be merged with larger regions. [Limit: >=0] [Units: vx] 
	int mergeRegionArea;
	
	/// The maximum number of vertices allowed for polygons generated during the 
	/// contour to polygon conversion process. [Limit: >= 3] 
	int maxVertsPerPoly;
	
	/// Sets the sampling distance to use when generating the detail mesh.
	/// (For height detail only.) [Limits: 0 or >= 0.9] [Units: wu] 
	float detailSampleDist;
	
	/// The maximum distance the detail mesh surface should deviate from heightfield
	/// data. (For height detail only.) [Limit: >=0] [Units: wu] 
	float detailSampleMaxError;
};


/// Represents a heightfield layer within a layer set.
/// @see rcHeightfieldLayerSet
struct HeightfieldLayer
{
	float _bmin[3];				///< The minimum bounds in world space. [(x, y, z)]
	float _bmax[3];				///< The maximum bounds in world space. [(x, y, z)]
	float _cs;					///< The size of each cell. (On the xz-plane.)
	float _ch;					///< The height of each cell. (The minimum increment along the y-axis.)
	int _width;					///< The width of the heightfield. (Along the x-axis in cell units.)
	int _height;					///< The height of the heightfield. (Along the z-axis in cell units.)
	int _minx;					///< The minimum x-bounds of usable data.
	int _maxx;					///< The maximum x-bounds of usable data.
	int _miny;					///< The minimum y-bounds of usable data. (Along the z-axis.)
	int _maxy;					///< The maximum y-bounds of usable data. (Along the z-axis.)
	int _hmin;					///< The minimum height bounds of usable data. (Along the y-axis.)
	int _hmax;					///< The maximum height bounds of usable data. (Along the y-axis.)
	unsigned char* _heights;		///< The heightfield. [Size: (width - borderSize*2) * (h - borderSize*2)]
	unsigned char* _areas;		///< Area ids. [Size: Same as #heights]
	unsigned char* _cons;		///< Packed neighbor connection information. [Size: Same as #heights]
};

/// Represents a set of heightfield layers.
/// @ingroup recast
/// @see rcAllocHeightfieldLayerSet, rcFreeHeightfieldLayerSet 
/*struct rcHeightfieldLayerSet
{
	rcHeightfieldLayer* layers;			///< The layers in the set. [Size: #nlayers]
	int nlayers;						///< The number of layers in the set.
};*/

/// Represents a polygon mesh suitable for use in building a navigation mesh. 
/// @ingroup recast
struct rcPolyMesh
{
	unsigned short* verts;	///< The mesh vertices. [Form: (x, y, z) * #nverts]
	unsigned short* polys;	///< Polygon and neighbor data. [Length: #maxpolys * 2 * #nvp]
	unsigned short* regs;	///< The region id assigned to each polygon. [Length: #maxpolys]
	unsigned short* flags;	///< The user defined flags for each polygon. [Length: #maxpolys]
	unsigned char* areas;	///< The area id assigned to each polygon. [Length: #maxpolys]
	int nverts;				///< The number of vertices.
	int npolys;				///< The number of polygons.
	int maxpolys;			///< The number of allocated polygons.
	int nvp;				///< The maximum number of vertices per polygon.
	float bmin[3];			///< The minimum bounds in world space. [(x, y, z)]
	float bmax[3];			///< The maximum bounds in world space. [(x, y, z)]
	float cs;				///< The size of each cell. (On the xz-plane.)
	float ch;				///< The height of each cell. (The minimum increment along the y-axis.)
	int borderSize;			///< The AABB border size used to generate the source data from which the mesh was derived.
};

/// Contains triangle meshes that represent detailed height data associated 
/// with the polygons in its associated polygon mesh object.
/// @ingroup recast
struct rcPolyMeshDetail
{
	unsigned int* meshes;	///< The sub-mesh data. [Size: 4*#nmeshes] 
	float* verts;			///< The mesh vertices. [Size: 3*#nverts] 
	unsigned char* tris;	///< The mesh triangles. [Size: 4*#ntris] 
	int nmeshes;			///< The number of sub-meshes defined by #meshes.
	int nverts;				///< The number of vertices in #verts.
	int ntris;				///< The number of triangles in #tris.
};

/// Heighfield border flag.
/// If a heightfield region ID has this bit set, then the region is a border 
/// region and its spans are considered unwalkable.
/// (Used during the region and contour build process.)
/// @see rcCompactSpan::reg
static const unsigned short RC_BORDER_REG = 0x8000;

/// Border vertex flag.
/// If a region ID has this bit set, then the associated element lies on
/// a tile border. If a contour vertex's region ID has this bit set, the 
/// vertex will later be removed in order to match the segments and vertices 
/// at tile boundaries.
/// (Used during the build process.)
/// @see rcCompactSpan::reg, #rcContour::verts, #rcContour::rverts
static const int RC_BORDER_VERTEX = 0x10000;

/// Area border flag.
/// If a region ID has this bit set, then the associated element lies on
/// the border of an area.
/// (Used during the region and contour build process.)
/// @see rcCompactSpan::reg, #rcContour::verts, #rcContour::rverts
static const int RC_AREA_BORDER = 0x20000;

/// Contour build flags.
/// @see rcBuildContours
enum rcBuildContoursFlags
{
	RC_CONTOUR_TESS_WALL_EDGES = 0x01,	///< Tessellate solid (impassable) edges during contour simplification.
	RC_CONTOUR_TESS_AREA_EDGES = 0x02,	///< Tessellate edges between areas during contour simplification.
};

/// Applied to the region id field of contour vertices in order to extract the region id.
/// The region id field of a vertex may have several flags applied to it.  So the
/// fields value can't be used directly.
/// @see rcContour::verts, rcContour::rverts
static const int RC_CONTOUR_REG_MASK = 0xffff;

/// An value which indicates an invalid index within a mesh.
/// @note This does not necessarily indicate an error.
/// @see rcPolyMesh::polys
static const unsigned short RC_MESH_NULL_IDX = 0xffff;

/// Represents the null area.
/// When a data element is given this value it is considered to no longer be 
/// assigned to a usable area.  (E.g. It is unwalkable.)
static const unsigned char RC_NULL_AREA = 0;

/// The default area id used to indicate a walkable polygon. 
/// This is also the maximum allowed area id, and the only non-null area id 
/// recognized by some steps in the build process. 
static const unsigned char RC_WALKABLE_AREA = 63;

/// The value returned by #rcGetCon if the specified direction is not connected
/// to another span. (Has no neighbor.)
static const int RC_NOT_CONNECTED = 0x3f;

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

	bool operator<=(Vertex const & v) const
	{
		return (x < v.x) || ((x == v.x) && (z <= v.z));
	}
	
	bool operator>=(Vertex const & v) const
	{
		return (x > v.x) || ((x == v.x) && (z >= v.z));
	}
};

struct Direction
{
	enum
	{
		Begin = 0,
		Left = 0,
		Forward,
		Right,
		Backward,
		End
	};

	/// Gets the standard width (x-axis) offset for the specified direction.
	///  @param[in]		dir		The direction. [Limits: 0 <= value < 4]
	///  @return The width offset to apply to the current cell position to move
	///  	in the direction.
	static int getXOffset(int dir)
	{
		switch(dir)
		{
			case Left:
				return -1;
			case Right:
				return 1;
			case Forward:
			case Backward:
				return 0;
			case End:
				throw std::range_error("End");
		}

		throw std::runtime_error("Unreachable");
	}

	/// Gets the standard height (z-axis) offset for the specified direction.
	///  @param[in]		dir		The direction. [Limits: 0 <= value < 4]
	///  @return The height offset to apply to the current cell position to move
	///  	in the direction.
	static int getZOffset(int dir)
	{
		switch(dir)
		{
			case Forward:
				return 1;
			case Backward:
				return -1;
			case Left:
			case Right:
				return 0;
			case End:
				throw std::range_error("End");
		}

		throw std::runtime_error("Unreachable");

	}
};

}
#endif // RECAST_H

///////////////////////////////////////////////////////////////////////////

// Due to the large amount of detail documentation for this file, 
// the content normally located at the end of the header file has been separated
// out to a file in /Docs/Extern.
