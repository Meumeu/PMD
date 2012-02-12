SET(recast_SRCS
	src/Recast/RecastPolyMesh.cpp
	src/Recast/RecastPolyMeshDetail.cpp
	src/Recast/RecastCompactHeightfield.cpp
	src/Recast/RecastHeightfield.cpp
	src/Recast/RecastContourSet.cpp
	src/Recast/RecastCompactHeightfieldArea.cpp
	src/Recast/RecastRegion.cpp

	PARENT_SCOPE
)

SET(recast_HDRS
	src/Recast/Recast.h
	src/Recast/RecastHeightfield.h
	src/Recast/RecastCompactHeightfield.h
	src/Recast/RecastContourSet.h
	src/Recast/RecastPolyMesh.h
	src/Recast/RecastPolyMeshDetail.h

	PARENT_SCOPE
)

