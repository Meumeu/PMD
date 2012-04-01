#include "Pathfinding.h"

namespace Pathfinding
{
    void NavMesh::AddTriangle(const Vertex & v1, const Vertex & v2, const Vertex & v3, const int area)
    {
	assert(area >= 0);
	assert(area <= UCHAR_MAX);
	std::pair<Triangle, int> tri(Triangle(v1, v2, v3), area);
	Triangles.push_back(tri);
	updateAabb(v1);
	updateAabb(v2);
	updateAabb(v3);
    }

    void NavMesh::Build()
    {
	memset(&cfg, 0, sizeof(cfg));
	cfg.cs = CellSize;
	cfg.ch = CellHeight;
	cfg.walkableSlopeAngle = AgentMaxSlope;
	cfg.walkableHeight = (int)ceilf(AgentHeight / cfg.ch);
	cfg.walkableClimb = (int)floorf(AgentMaxClimb / cfg.ch);
	cfg.walkableRadius = (int)ceilf(AgentRadius / cfg.cs);
	cfg.maxEdgeLen = (int)(EdgeMaxLen / CellSize);
	cfg.maxSimplificationError = EdgeMaxError;
	cfg.minRegionArea = (int)(RegionMinSize*RegionMinSize);		// Note: area = size*size
	cfg.mergeRegionArea = (int)(RegionMergeSize*RegionMergeSize);	// Note: area = size*size
	cfg.maxVertsPerPoly = VertsPerPoly;
	cfg.detailSampleDist = DetailSampleDist < 0.9f ? 0 : CellSize * DetailSampleDist;
	cfg.detailSampleMaxError = CellHeight * DetailSampleMaxError;

	rcVcopy(cfg.bmin, bmin);
	rcVcopy(cfg.bmax, bmax);
	cfg.bmin[0] -= cfg.borderSize * cfg.cs;
	cfg.bmin[2] -= cfg.borderSize * cfg.cs;
	cfg.bmax[0] += cfg.borderSize * cfg.cs;
	cfg.bmax[2] += cfg.borderSize * cfg.cs;
	    
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	if (cfg.maxVertsPerPoly > DT_VERTS_PER_POLYGON)
	    throw std::range_error("maxVertsPerPoly > 6");

	Reset();

	cfg.width  = (cfg.bmax[0] - cfg.bmin[0]) / cfg.cs + 1;
	cfg.height = (cfg.bmax[2] - cfg.bmin[2]) / cfg.cs + 1;
	    
	if (!rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
	    throw std::bad_alloc();
	    
	for(std::pair<Triangle, int> const & tri : Triangles)
	{
	    const int flagMergeThreshold = 0;
		
	    float v1[3]; toRecastVertex(std::get<0>(tri.first), v1);
	    float v2[3]; toRecastVertex(std::get<1>(tri.first), v2);
	    float v3[3]; toRecastVertex(std::get<2>(tri.first), v3);
		
	    rcRasterizeTriangle(&ctx, v1, v2, v3, tri.second, *hf, flagMergeThreshold);
	}
	    
	rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *hf);
	rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
	rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *hf);

	if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf))
	    throw std::bad_alloc();

	if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
	    throw std::bad_alloc();

	if (!rcBuildDistanceField(&ctx, *chf))
	    throw std::bad_alloc();

	if (!rcBuildRegions(&ctx, *chf, 0 /* border size */, cfg.minRegionArea, cfg.mergeRegionArea))
	    throw std::bad_alloc();

	if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
	    throw std::bad_alloc();

	if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *mesh))
	    throw std::bad_alloc();
	
	if (!rcBuildPolyMeshDetail(&ctx, *mesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
	    throw std::bad_alloc();
	
	// TODO: changer les flags cf Sample_SoloMesh.cpp:594
	for(int i = 0; i < mesh->npolys; ++i)
	{
		mesh->flags[i] = 1;
	}
	
	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));
	
	params.verts = mesh->verts;
	params.vertCount = mesh->nverts;
	params.polys = mesh->polys;
	params.polyAreas = mesh->areas;
	params.polyFlags = mesh->flags;
	params.polyCount = mesh->npolys;
	params.nvp = mesh->nvp;
	
	params.detailMeshes = dmesh->meshes;
	params.detailVerts = dmesh->verts;
	params.detailVertsCount = dmesh->nverts;
	params.detailTris = dmesh->tris;
	params.detailTriCount = dmesh->ntris;

	params.walkableHeight = cfg.walkableHeight;
	params.walkableClimb = cfg.walkableClimb;
	rcVcopy(params.bmin, mesh->bmin);
	rcVcopy(params.bmax, mesh->bmax);
	params.cs = cfg.cs;
	params.ch = cfg.ch;
	params.buildBvTree = true;

	if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
	    throw std::bad_alloc();
	    
	navmesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
    }
}
