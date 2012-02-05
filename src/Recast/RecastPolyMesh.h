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

#ifndef RECAST_POLYMESH_H
#define RECAST_POLYMESH_H

#include <vector>
#include "Recast.h"

namespace Recast{
	
class ContourSet;
class Contour;

class PolyMesh
{	
public:
	struct Polygon
	{
		Polygon(Vertex v1, Vertex v2, Vertex v3, int r) : regionID(r)
		{
			vertices[0] = v1;
			vertices[1] = v2;
			vertices[2] = v3;
			neighbours[0] = -1;
			neighbours[1] = -1;
			neighbours[2] = -1;
		}
		int regionID;
		int flags;
		Vertex vertices[3];
		int neighbours[3];
	};
	
	std::vector<Polygon> polys;       ///< Polygon and neighbour data
	
	void triangulate(Contour const & cont);
	void fillPolygonNeighbours();
	
public:
	PolyMesh(ContourSet const & cset);
};
}

#endif