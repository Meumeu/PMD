// -*- c++ -*-

#ifndef RECASTWRAPPER_H
#define RECASTWRAPPER_H

#include "Recast/Recast.h"
#include "Detour/DetourNavMesh.h"
#include "Detour/DetourNavMeshBuilder.h"
#include "Detour/DetourNavMeshQuery.h"

#include <cfloat>
#include <climits>
#include <cassert>
#include <cmath>

#include <tuple>
#include <vector>
#include <stdexcept>
#include <memory>
#include <string.h>

#include <OgreVector3.h>

#include "../DebugDrawer.h"

namespace Pathfinding
{
typedef Ogre::Vector3 Vertex;
typedef std::tuple<Vertex, Vertex, Vertex> Triangle;

class NavMesh
{

public:
	float CellSize;
	float CellHeight;
	float AgentMaxSlope;
	float AgentHeight;
	float AgentMaxClimb;
	float AgentRadius;
	float EdgeMaxLen;
	float EdgeMaxError;
	float RegionMinSize;
	float RegionMergeSize;
	int VertsPerPoly;
	float DetailSampleDist;
	float DetailSampleMaxError;

	Vertex QueryExtent;

	class Path
	{

		friend class NavMesh;
		std::shared_ptr<dtNavMesh> navmesh;

		std::vector<Vertex> vertices;

		Path(std::shared_ptr<dtNavMesh> navmeshref,
		     const float * start,
		     const float * end,
		     const float * extent,
		     const dtQueryFilter & filter);
		
	public:
		Path() {}
		typedef std::vector<Vertex>::const_iterator iterator;
		Vertex operator[](size_t i) const
		{
#ifndef NDEBUG
			if (i >= vertices.size())
				throw std::out_of_range("Pathfinding::NavMesh::Path::operator[]: out of range");
#endif
			return vertices[i];
		}

		size_t size() const
		{
			return vertices.size();
		}

		bool empty() const
		{
			return vertices.empty();
		}

		iterator begin() const
		{
			return vertices.begin();
		}

		iterator end() const
		{
			return vertices.end();
		}
	};

private:
	rcContext ctx;

	rcHeightfield * hf;
	rcCompactHeightfield * chf;
	rcContourSet * cset;
	rcPolyMesh * mesh;
	rcPolyMeshDetail * dmesh;
	std::shared_ptr<dtNavMesh> navmesh;

	rcConfig cfg;

	float bmin[3];
	float bmax[3];

	unsigned char * navData;
	int navDataSize;

	static void toRecastVertex(Vertex const & v1, float * v2);
	void updateAabb(Vertex const & v);

	std::vector<std::pair<Triangle, int> > Triangles;
	void Free();
	void Alloc();

public:
	void Reset();
	NavMesh();
	~NavMesh();

	void AddTriangle(const Vertex & v1, const Vertex & v2, const Vertex & v3, const int area);
	void Build();
	Path Query(Vertex const & start, Vertex const & end) const
	{
		float _start[3];
		float _end[3];
		float _extent[3];

		toRecastVertex(start, _start);
		toRecastVertex(end, _end);
		toRecastVertex(QueryExtent, _extent);
		dtQueryFilter filter;

		return Path(navmesh, _start, _end, _extent, filter);
	}

	bool DrawHeightfield;
	bool DrawCompactHeightfield;
	bool DrawRawContours;
	bool DrawContours;
	bool DrawPolyMesh;
	bool DrawPolyMeshDetail;

	void DebugDraw(DebugDrawer & dd);
	void DebugDrawHeightfield(DebugDrawer & dd);
	void DebugDrawCompactHeightfield(DebugDrawer & dd);
	void DebugDrawRawContours(DebugDrawer & dd);
	void DebugDrawContours(DebugDrawer & dd);
	void DebugDrawPolyMesh(DebugDrawer & dd);
	void DebugDrawPolyMeshDetail(DebugDrawer & dd);
};
}

#endif // RECASTWRAPPER_H

