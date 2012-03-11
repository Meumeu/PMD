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

#include <boost/foreach.hpp>
#include <assert.h>
#include <list>
#include <climits>
#include <map>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>

using boost::uint8_t;

#include "RecastPolyMesh.h"
#include "RecastContourSet.h"

namespace Recast
{
typedef std::list<std::pair<bool, unsigned int> > IndexList;


void PolyMesh::fillPolygonNeighbours()
{
	typedef std::map<std::pair<IntVertex, IntVertex>, std::vector<std::pair<size_t, uint8_t> > > PolygonMap;
	
	PolygonMap pmap;
	for(size_t i = 0, size = polys.size(); i < size; ++i)
	{
		for(size_t j = 0, k = Polygon::NVertices - 1; j < Polygon::NVertices; k = j++)
		{
			IntVertex const & v1 = polys[i].vertices[j];
			IntVertex const & v2 = polys[i].vertices[k];
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
	
	BOOST_FOREACH(PolygonMap::value_type const & i, pmap)
	{
		if (i.second.size() == 1) continue;
		assert(i.second.size() == 2);
		
		size_t poly1 = i.second[0].first;
		size_t poly2 = i.second[1].first;
		uint8_t edge1 = i.second[0].second;
		uint8_t edge2 = i.second[1].second;
		
		assert(polys[poly1].neighbours[edge1] == -1);
		assert(polys[poly2].neighbours[edge2] == -1);
		
		polys[poly1].neighbours[edge1] = poly2;
		polys[poly2].neighbours[edge2] = poly1;
	}
	
}

static int det(IntVertex const & a, IntVertex const & b, IntVertex const & c)
{
	return (b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x);
}

// Returns true if c is strictly on the left of segment (a, b)
static bool left(IntVertex const & a, IntVertex const & b, IntVertex const & c)
{
	return det(a, b, c) < 0;
}

// Returns true if c is strictly on the right of segment (a, b)
static bool right(IntVertex const & a, IntVertex const & b, IntVertex const & c)
{
	return det(a, b, c) > 0;
}

// Returns true if a, b, c are colinear
static bool colinear(IntVertex const & a, IntVertex const & b, IntVertex const & c)
{
	return det(a, b, c) == 0;
}

// Returns true if c is on the left of or on segment (a, b)
static bool left_or_colinear(IntVertex const & a, IntVertex const & b, IntVertex const & c)
{
	return det(a, b, c) <= 0;
}

static bool intersect_properly(IntVertex const & a, IntVertex const & b, IntVertex const & c, IntVertex const & d)
{
	if (colinear(a, b, c)) return false;
	if (colinear(a, b, d)) return false;
	if (colinear(c, d, a)) return false;
	if (colinear(c, d, b)) return false;
	
	if (left(a, b, c) == left(a, b, d)) return false;
	if (left(c, d, a) == left(c, d, b)) return false;
	
	return true;
}

static bool between(IntVertex const & a, IntVertex const & b, IntVertex const & c)
{
	return colinear(a, b, c) && ((a <= c && c <= b) || (a >= c && c >= b));
}

static bool intersect(IntVertex const & a, IntVertex const & b, IntVertex const & c, IntVertex const & d)
{
	return intersect_properly(a, b, c, d) || between(a, b, c) || between(a, b, d) || between(c, d, a) || between(c, d, b);
}

static bool horizontal_equal(IntVertex const & a, IntVertex const & b)
{
	return a.x == b.x && a.z == b.z;
}

// Returns true if (i, j) is inside the contour
static bool diagonal(Contour const & cont, IndexList const & indices, IndexList::const_iterator i, IndexList::const_iterator j)
{
	IntVertex const & a = cont.verts[i->second];
	IntVertex const & b = cont.verts[j->second];

	// Check if (i,j) is inside the contour in the neighbourhood of i
	IndexList::const_iterator next = i, prev = i;
	++next;
	if (next == indices.end()) next = indices.begin();
	if (prev == indices.begin()) prev = indices.end();
	--prev;
	
	IntVertex const & vprev = cont.verts[prev->second];
	IntVertex const & vnext = cont.verts[next->second];
	
	if (!right(vprev, a, vnext))
	{
		if (!(left(a, b, vprev) && left(b, a, vnext))) return false;
	}
	else
	{
		if (left_or_colinear(a, b, vnext) && left_or_colinear(b, a, vprev)) return false;
	}
	
	// Check if (i, j) is a diagonal, ie. if it intersects no edge
	for(IndexList::const_iterator it = indices.begin(), end = indices.end(); it != end; ++it)
	{
		IndexList::const_iterator it2 = it; ++it2; if (it2 == end) it2 = indices.begin();
		
		if (!(i == it || i == it2 || j == it || j == it2))
		{
			IntVertex const & c = cont.verts[i->second];
			IntVertex const & d = cont.verts[j->second];
			
			if (!(horizontal_equal(a, c) || horizontal_equal(a, d) || horizontal_equal(b, c) || horizontal_equal(b, d)))
			{
				if (intersect(a, b, c, d)) return false;
			}
		}
	}

	return true;
}

static bool diagonal(Contour const & cont, IndexList const & indices, IndexList::const_iterator i)
{
	assert(!indices.empty());
	
	IndexList::const_iterator next = i, prev = i;
	++next;
	if (next == indices.end()) next = indices.begin();
	if (prev == indices.begin()) prev = indices.end();
	--prev;
	
	return diagonal(cont, indices, prev, next);
}

void PolyMesh::triangulate(Contour const & cont)
{
	IndexList indices;
	
	for(size_t i = 0; i < cont.verts.size(); ++i)
	{
		indices.push_back(std::make_pair(false, i));
	}

	IndexList::iterator it = indices.begin(), end = indices.end();
	for(size_t i = 0; i < cont.verts.size(); ++i)
	{
		assert(it != end);
		it->first = diagonal(cont, indices, it);
		++it;
	}
	
	while(indices.size() > 3)
	{
		int minlen = INT_MAX;
		IndexList::iterator end = indices.end(), min = end;
		
		for(IndexList::iterator it0 = indices.begin(); it0 != end; ++it0)
		{
			IndexList::iterator it1 = it0; ++it1; if (it1 == end) it1 = indices.begin();
			
			if (it1->first)
			{
				IndexList::iterator it2 = it1; ++it2; if (it2 == end) it2 = indices.begin();
				int dx = cont.verts[it2->second].x - cont.verts[it0->second].x;
				int dz = cont.verts[it2->second].z - cont.verts[it0->second].z;
				int len = dx * dx + dz * dz;
				if (len < minlen)
				{
					minlen = len;
					min = it0;
				}
			}
		}
		
		assert(min != end);
		
		IndexList::iterator it0 = min;
		IndexList::iterator it1 = it0; ++it1; if (it1 == end) it1 = indices.begin();
		IndexList::iterator it2 = it1; ++it2; if (it2 == end) it2 = indices.begin();
		
		polys.push_back(
			Polygon(cont.verts[it0->second],
				cont.verts[it1->second],
				cont.verts[it2->second],
				cont.reg));
		
		indices.erase(it1);
		
		it0->first = diagonal(cont, indices, it0);
		it2->first = diagonal(cont, indices, it2);
		
	}
	
	
	assert(indices.size() == 3);
	IndexList::iterator it0 = indices.begin();
	IndexList::iterator it1 = it0; ++it1; if (it1 == end) it1 = indices.begin();
	IndexList::iterator it2 = it1; ++it2; if (it2 == end) it2 = indices.begin();
	
	polys.push_back(
		Polygon(cont.verts[it0->second],
			cont.verts[it1->second],
			cont.verts[it2->second],
			cont.reg));
}
	
PolyMesh::PolyMesh(const Recast::ContourSet& cset)
{
	BOOST_STATIC_ASSERT(Polygon::NVertices == 3);
	
	BOOST_FOREACH(Contour const & cont, cset._conts)
	{
		assert(cont.verts.size() >= 3);
		triangulate(cont);
	}
	
	fillPolygonNeighbours();
}

}
