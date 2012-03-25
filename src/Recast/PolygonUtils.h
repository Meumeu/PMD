//
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

#ifndef POLYGON_UTILS_H
#define POLYGON_UTILS_H

#include <stddef.h>
#include <vector>
#include <cassert>

#include <boost/foreach.hpp>
#include <boost/cstdint.hpp>

using boost::uint8_t;

namespace Recast
{

template<typename polygon_t, typename vertex_t>
void fillPolygonNeighbours(std::vector<polygon_t>& polys, size_t nVertices)
{
	typedef std::map<std::pair<vertex_t, vertex_t>, std::vector<std::pair<size_t, uint8_t> > > PolygonMap;

	PolygonMap pmap;
	for(size_t i = 0, size = polys.size(); i < size; ++i)
	{
		for(uint8_t j = 0, k = nVertices - 1; j < nVertices; k = j++)
		{
			vertex_t const & v1 = polys[i]._vertices[j];
			vertex_t const & v2 = polys[i]._vertices[k];
			if (v1 < v2)
			{
				pmap[std::make_pair(v1, v2)].push_back(std::make_pair(i, j));
			}
			else
			{
				pmap[std::make_pair(v2, v1)].push_back(std::make_pair(i, j));
			}
		}
	}

	BOOST_FOREACH(typename PolygonMap::value_type const & i, pmap)
	{
		if (i.second.size() == 1) continue;
		assert(i.second.size() == 2);

		size_t poly1 = i.second[0].first;
		size_t poly2 = i.second[1].first;
		uint8_t edge1 = i.second[0].second;
		uint8_t edge2 = i.second[1].second;

		assert(polys[poly1]._neighbours[edge1] == -1);
		assert(polys[poly2]._neighbours[edge2] == -1);

		polys[poly1]._neighbours[edge1] = poly2;
		polys[poly2]._neighbours[edge2] = poly1;
	}

}
	
}

#endif
