SET(recast_SRCS
	src/Recast/RecastPolyMesh.cpp
	src/Recast/RecastContext.cpp
	src/Recast/Recast.cpp
	src/Recast/RecastMeshDetail.cpp
	src/Recast/RecastCompactHeightfieldContour.cpp
	#src/Recast/RecastCompactHeightfield.cpp
	src/Recast/RecastHeightfield.cpp
	#src/Recast/RecastHeightfieldFilter.cpp
	src/Recast/RecastAlloc.cpp
	src/Recast/RecastHeightfieldLayerSet.cpp
	src/Recast/RecastLayers.cpp
	src/Recast/RecastContourSet.cpp
	src/Recast/RecastMesh.cpp
	src/Recast/RecastCompactHeightfieldArea.cpp
	#src/Recast/RecastRasterization.cpp
	src/Recast/RecastPolyMeshDetail.cpp
	src/Recast/RecastRegion.cpp

	PARENT_SCOPE
)

SET(recast_HDRS
	src/Recast/Recast.h
	src/Recast/RecastAlloc.h
	src/Recast/RecastAssert.h
	src/Recast/RecastHeightfield.h
	PARENT_SCOPE
)

