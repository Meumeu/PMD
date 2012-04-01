#include "Pathfinding.h"

namespace Pathfinding
{
    void NavMesh::toRecastVertex(Vertex const & v1, float * v2)
    {
	v2[0] = v1.x;
	v2[1] = v1.y;
	v2[2] = v1.z;
    }

    void NavMesh::updateAabb(Vertex const & v)
    {
	bmin[0] = std::min(bmin[0], v.x);
	bmin[1] = std::min(bmin[1], v.y);
	bmin[2] = std::min(bmin[2], v.z);
    
	bmax[0] = std::max(bmax[0], v.x);
	bmax[1] = std::max(bmax[1], v.y);
	bmax[2] = std::max(bmax[2], v.z);
    }
}
