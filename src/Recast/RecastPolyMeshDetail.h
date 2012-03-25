//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
// Copyright 2012 Patrick Nicolas <patricknicolas@laposte.net>
// Copyright 2012 Guillaume Meunier <guillaume.meunier@centraliens.net>
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

#ifndef RECAST_POLYMESHDETAIL_H
#define RECAST_POLYMESHDETAIL_H

#include "Recast.h"

namespace Recast
{
class PolyMesh;
class CompactHeightfield;

class PolyMeshDetail
{	
public:
	PolyMeshDetail(PolyMesh const & pm, CompactHeightfield const & chf, const float sampleDist, const float sampleMaxError);
	struct Triangle
	{
		Triangle(FloatVertex v1, FloatVertex v2, FloatVertex v3)
		{
			_vertices[0] = v1;
			_vertices[1] = v2;
			_vertices[2] = v3;
			_neighbours[0] = -1;
			_neighbours[1] = -1;
			_neighbours[2] = -1;
		}
		FloatVertex _vertices[3];
		int _neighbours[3];
	};
private:
	std::vector<Triangle> _meshes;       ///< Triangle and neighbour data
};
}

#endif // RECAST_POLYMESHDETAIL_H
