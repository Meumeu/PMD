SET(recast_SRCS
	src/Recast/RecastPolyMesh.cpp
	#src/Recast/RecastMeshDetail.cpp
	#src/Recast/RecastCompactHeightfieldContour.cpp
	src/Recast/RecastCompactHeightfield.cpp
	src/Recast/RecastHeightfield.cpp
	src/Recast/RecastAlloc.cpp
	#src/Recast/RecastLayers.cpp
	src/Recast/RecastContourSet.cpp
	#src/Recast/RecastMesh.cpp
	src/Recast/RecastCompactHeightfieldArea.cpp
	src/Recast/RecastPolyMeshDetail.cpp
	src/Recast/RecastRegion.cpp

	PARENT_SCOPE
)

SET(recast_HDRS
	src/Recast/Recast.h
	src/Recast/RecastAlloc.h
	src/Recast/RecastAssert.h
	src/Recast/RecastHeightfield.h
	src/Recast/RecastCompactHeightfield.h
	PARENT_SCOPE
)

