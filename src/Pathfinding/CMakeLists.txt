SET(Pathfinding_HDRS
	src/Pathfinding/Detour/DetourAlloc.h
	src/Pathfinding/Detour/DetourAssert.h
	src/Pathfinding/Detour/DetourCommon.h
	src/Pathfinding/Detour/DetourNavMesh.h
	src/Pathfinding/Detour/DetourNavMeshBuilder.h
	src/Pathfinding/Detour/DetourNavMeshQuery.h
	src/Pathfinding/Detour/DetourNode.h
	src/Pathfinding/Detour/DetourStatus.h
	src/Pathfinding/Pathfinding.h
	src/Pathfinding/Recast/Recast.h
	src/Pathfinding/Recast/RecastAlloc.h
	src/Pathfinding/Recast/RecastAssert.h
	
	PARENT_SCOPE)

set(Pathfinding_SRCS
	src/Pathfinding/Detour/DetourAlloc.cpp
	src/Pathfinding/Detour/DetourCommon.cpp
	src/Pathfinding/Detour/DetourNavMesh.cpp
	src/Pathfinding/Detour/DetourNavMeshBuilder.cpp
	src/Pathfinding/Detour/DetourNavMeshQuery.cpp
	src/Pathfinding/Detour/DetourNode.cpp
	src/Pathfinding/Recast/Recast.cpp
	src/Pathfinding/Recast/RecastAlloc.cpp
	src/Pathfinding/Recast/RecastArea.cpp
	src/Pathfinding/Recast/RecastContour.cpp
	src/Pathfinding/Recast/RecastFilter.cpp
	src/Pathfinding/Recast/RecastLayers.cpp
	src/Pathfinding/Recast/RecastMesh.cpp
	src/Pathfinding/Recast/RecastMeshDetail.cpp
	src/Pathfinding/Recast/RecastRasterization.cpp
	src/Pathfinding/Recast/RecastRegion.cpp
	src/Pathfinding/RecastDebug.cpp
	src/Pathfinding/RecastWrapperAlloc.cpp
	src/Pathfinding/RecastWrapperBuild.cpp
	src/Pathfinding/RecastWrapperQuery.cpp
	src/Pathfinding/RecastWrapperUtils.cpp
	
	PARENT_SCOPE)