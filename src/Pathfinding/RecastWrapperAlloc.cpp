#include "Pathfinding.h"

namespace Pathfinding
{
void NavMesh::Free()
{
	navmesh.reset();

	if (dmesh)
	{
		rcFreePolyMeshDetail(dmesh);
		dmesh = 0;
	}

	if (mesh)
	{
		rcFreePolyMesh(mesh);
		mesh = 0;
	}

	if (cset)
	{
		rcFreeContourSet(cset);
		cset = 0;
	}

	if (chf)
	{
		rcFreeCompactHeightfield(chf);
		chf = 0;
	}

	if (hf)
	{
		rcFreeHeightField(hf);
		hf = 0;
	}
}

void NavMesh::Alloc()
{
	try
	{
		hf = rcAllocHeightfield();

		if (!hf) throw std::bad_alloc();

		chf = rcAllocCompactHeightfield();

		if (!chf) throw std::bad_alloc();

		cset = rcAllocContourSet();

		if (!cset) throw std::bad_alloc();

		mesh = rcAllocPolyMesh();

		if (!mesh) throw std::bad_alloc();

		dmesh = rcAllocPolyMeshDetail();

		if (!dmesh) throw std::bad_alloc();

		navmesh = std::shared_ptr<dtNavMesh>(dtAllocNavMesh(), dtFreeNavMesh);

		if (!navmesh) throw std::bad_alloc();
	}
	catch (...)
	{
		Free();
		throw;
	}
}

void NavMesh::Reset()
{
	Free();
	Alloc();
}

// Same default values as RecastDemo
NavMesh::NavMesh() : CellSize(0.3),
		CellHeight(0.2),
		AgentMaxSlope(M_PI / 4),
		AgentHeight(2.0),
		AgentMaxClimb(0.9),
		AgentRadius(0.6),
		EdgeMaxLen(12),
		EdgeMaxError(1.3),
		RegionMinSize(8),
		RegionMergeSize(20),
		VertsPerPoly(6),
		DetailSampleDist(6),
		DetailSampleMaxError(1),
		QueryExtent(2, 4, 2),
		hf(0), chf(0), cset(0), mesh(0), dmesh(0),
		DrawHeightfield(false),
		DrawCompactHeightfield(false),
		DrawRawContours(false),
		DrawContours(false),
		DrawPolyMesh(false),
		DrawPolyMeshDetail(false)
{
	memset(&cfg, 0, sizeof(cfg));
	bmin[0] = FLT_MAX;
	bmin[1] = FLT_MAX;
	bmin[2] = FLT_MAX;
	bmax[0] = -FLT_MAX;
	bmax[1] = -FLT_MAX;
	bmax[2] = -FLT_MAX;
}

NavMesh::~NavMesh()
{
	Free();
}
}

