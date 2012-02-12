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

#include <set>

#include "Recast.h"
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace Recast
{
class PolyMesh;
class CompactHeightfield;

class PolyMeshDetail
{
public:
	class DelaunayTriangulation
	{
	public:
		DelaunayTriangulation(FloatVertex const & v1, FloatVertex const & v2, FloatVertex const & v3) {}
		DelaunayTriangulation & operator<<(FloatVertex const & v);
		DelaunayTriangulation & operator<<(const std::vector< Recast::FloatVertex >& vertices);
	};
private:
	//std::vector<Triangle> meshes;
	
public:
	PolyMeshDetail(PolyMesh const & pm, CompactHeightfield const & chf, const float sampleDist, const float sampleMaxError);
};
}

#endif // RECAST_POLYMESHDETAIL_H
