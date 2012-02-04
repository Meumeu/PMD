//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
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

#include <climits>
#include <boost/foreach.hpp>

#include "Recast.h"
#include "RecastCompactHeightfield.h"
#include "RecastContourSet.h"
#include <iostream>

namespace Recast
{

static int getCornerHeight(const CompactSpan& span, int dir)
{
	int dirp = (dir + 1) % 4;
	int CornerHeight = span._bottom;
	
	const CompactSpan * span1 = span.neighbours[dir];
	if (span1)
	{
		CornerHeight = std::max<int>(CornerHeight, span1->_bottom);
		span1 = span1->neighbours[dirp];
		if (span1)
		{
			CornerHeight = std::max<int>(CornerHeight, span1->_bottom);
		}
	}
	
	span1 = span.neighbours[dirp];
	if (span1)
	{
		CornerHeight = std::max<int>(CornerHeight, span1->_bottom);
		span1 = span1->neighbours[dir];
		if (span1)
		{
			CornerHeight = std::max<int>(CornerHeight, span1->_bottom);
		}
	}
	
	return CornerHeight;
}

static bool isBorderVertex(const CompactSpan& span, int dir)
{
	int dirp = (dir + 1) % 4;
	int regions[4] = {span._regionID, 0, 0, 0};
	bool walkable[4] = {span._walkable, false, false, false};
	
	const CompactSpan * span1 = span.neighbours[dir];
	if (span1)
	{
		regions[1] = span1->_regionID;
		walkable[1] = span1->_walkable;
		span1 = span1->neighbours[dirp];
		if (span1)
		{
			regions[2] = span1->_regionID;
			walkable[2] = span1->_walkable;
		}
	}
	
	span1 = span.neighbours[dirp];
	if (span1)
	{
		regions[3] = span1->_regionID;
		walkable[3] = span1->_walkable;
		span1 = span1->neighbours[dir];
		if (span1)
		{
			regions[2] = span1->_regionID;
			walkable[2] = span1->_walkable;
		}
	}
	
	if (regions[0] == 0 && !walkable[0]) return false;
	if (regions[1] == 0 && !walkable[1]) return false;
	if (regions[2] == 0 && !walkable[2]) return false;
	if (regions[3] == 0 && !walkable[3]) return false;
	
	for(int j = 0; j < 4; j++)
	{
		const int a = j;
		const int b = (j + 1) % 4;
		const int c = (j + 2) % 4;
		const int d = (j + 3) % 4;
		
		if ((walkable[a] == walkable[b]) && 
		    (walkable[c] == walkable[d]) &&
		    (regions[a] == regions[b]) &&
		    (regions[a] & regions[b] & RC_BORDER_REG) &&
		    !((regions[c] | regions[d]) & RC_BORDER_REG))
			return true;
	}
	
	return false;
}

static float calcPointSegmentSqrDistance(Contour::Vertex const & p, Contour::Vertex const & a, Contour::Vertex const & b)
{
	float abx = b.x - a.x;
	float abz = b.z - a.z;
	float dx  = p.x - a.x;
	float dz  = p.z - a.z;
	
	float d = abx * abx + abz * abz;
	float t = abx * dx + abz * dz;
	
	if (d > 0) t /= d;
	
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	
	dx = a.x + t * abx - p.x;
	dz = a.z + t * abz - p.z;
	
	return dx * dx + dz * dz;
}

static float calcPolygonArea(Contour const & cont)
{
	std::vector<Contour::Vertex> const & verts = cont.verts;
	int n = verts.size();
	int area = 0;
	
	for(int i = 0, j = n - 1; i < n; j = i++)
	{
		area += verts[i].x * verts[j].z - verts[j].x * verts[i].z;
	}
	
	return (area + 1) / 2;
}

static bool ileft(Contour::Vertex const & a, Contour::Vertex const & b, Contour::Vertex const & c)
{
	return (b.x - a.x) * (c.z - a.z) - (b.z * a.z) * (c.x - a.x) <= 0;
}

static std::pair<int, int> getClosestVertices(Contour const & ca, Contour const & cb)
{
	std::vector<Contour::Vertex> const & vertsa = ca.verts;
	std::vector<Contour::Vertex> const & vertsb = cb.verts;
	int closest = INT_MAX;
	int ia = -1;
	int ib = -1;
	
	const int na = vertsa.size();
	const int nb = vertsb.size();
	
	for(int i = 0; i < na; ++i)
	{
		Contour::Vertex const & va_prev = vertsa[(i + na - 1) % na];
		Contour::Vertex const & va = vertsa[i];
		Contour::Vertex const & va_next = vertsa[(i + 1) % na];
		
		for(int j = 0; j < nb; j++)
		{
			Contour::Vertex const & vb = vertsb[j];
			const int d = (va.x - vb.x) * (va.x - vb.x) + (va.z - vb.z) * (va.z - vb.z);
			if (d < closest && ileft(va_prev, va, vb) && ileft(va, va_next, vb))
			{
				closest = d;
				ia = i;
				ib = j;
			}
		}
	}
	
	return std::make_pair<int, int>(ia, ib);
}

static void mergeContours(Contour & ca, Contour & cb)
{
	std::vector<Contour::Vertex> out;
	std::pair<int, int> idx = getClosestVertices(ca, cb);
	
	const int na = ca.verts.size();
	const int nb = cb.verts.size();
	out.reserve(na + nb + 2);
	
	for(int i = 0; i <= na; i++)
	{
		out.push_back(ca.verts[(i+idx.first) % na]);
	}
	
	for(int i = 0; i <= nb; i++)
	{
		out.push_back(cb.verts[(i+idx.second) % nb]);
	}

	ca.verts.swap(out);
	cb.verts.clear();
}

static std::vector<Contour::Vertex> walkContour(int x, int z, CompactSpan & start)
{
	int dir = 0;
	std::vector<Contour::Vertex> out;
	
	while((start._tmpData & (1 << dir)) == 0) ++dir;
	
	const int startdir = dir;
	
	CompactSpan * currentSpan = &start;
	int iter = 40000;
	do
	{
		if (currentSpan->_tmpData & (1 << dir))
		{
			Contour::Vertex v(x, getCornerHeight(*currentSpan, dir), z);
			switch(dir)
			{
				case Direction::Left:
					v.z++;
					break;
					
				case Direction::Forward:
					v.x++;
					v.z++;
					break;
					
				case Direction::Right:
					v.x++;
					break;
					
				case Direction::Backward:
					break;
					
				default:
					throw std::runtime_error("Unreachable");
			}
			
			if (currentSpan->neighbours[dir])
			{
				v.flag = currentSpan->neighbours[dir]->_regionID;
				if (currentSpan->neighbours[dir]->_walkable != start._walkable)
					v.flag |= RC_AREA_BORDER;
			}
			
			if (isBorderVertex(*currentSpan, dir))
				v.flag |= RC_BORDER_VERTEX;
			
			out.push_back(v);
			
			currentSpan->_tmpData &= ~(1 << dir); // Remove visited edges
			dir = (dir + 1) % 4; // Rotate clockwise
		}
		else
		{
			x = x + Direction::getXOffset(dir);
			z = z + Direction::getZOffset(dir);
			currentSpan = currentSpan->neighbours[dir];
			if (!currentSpan) throw std::runtime_error("Should not happen");
			dir = (dir + 3) % 4; // Rotate counterclockwise
		}
		
		if (--iter == 0) throw std::runtime_error("Unable to close contour");
	} while(currentSpan != &start || dir != startdir);
	
	return out;
}

static std::vector<Contour::Vertex> simplifyContour(std::vector<Contour::Vertex> const & verts, const float maxError, const int maxEdgeLen, const int buildFlags)
{
	assert(verts.size() > 0);
	
	std::vector<Contour::Vertex> out;
	
	// Add initial points.
	bool hasConnections = false;
	BOOST_FOREACH(Contour::Vertex const &i, verts)
	{
		if ((i.flag & RC_CONTOUR_REG_MASK) != 0)
		{
			hasConnections = true;
			break;
		}
	}
	
	if (hasConnections)
	{
		// The contour has some portals to other regions.
		// Add a new point to every location where the region changes.
		for(std::vector<Contour::Vertex>::const_iterator it = verts.begin(); it != verts.end(); ++it)
		{
			std::vector<Contour::Vertex>::const_iterator next = it; ++next;
			if (next == verts.end()) next = verts.begin();
			
			if (((it->flag & RC_CONTOUR_REG_MASK) != (next->flag & RC_CONTOUR_REG_MASK)) ||
			(it->flag & RC_AREA_BORDER) != (next->flag & RC_AREA_BORDER))
			{
				out.push_back(Contour::Vertex(it->x, it->y, it->z, it - verts.begin()));
			}
		}
	}
	
	if (out.empty())
	{
		// If there is no connections at all,
		// create some initial points for the simplification process. 
		// Find lower-left and upper-right vertices of the contour.
		
		std::vector<Contour::Vertex>::const_iterator lowerleft = verts.begin();
		std::vector<Contour::Vertex>::const_iterator upperright = verts.begin();
		for(std::vector<Contour::Vertex>::const_iterator it = verts.begin(); it != verts.end(); ++it)
		{
			if (*it < *lowerleft) lowerleft = it;
			if (*it > *upperright) upperright = it;
		}
		
		Contour::Vertex ll = *lowerleft; ll.flag = lowerleft - verts.begin();
		Contour::Vertex ur = *upperright; ur.flag = upperright - verts.begin();
		
		out.push_back(ll);
		out.push_back(ur);
	}
	
	// Add points until all raw points are within
	// error tolerance to the simplified shape.
	const int n = verts.size();
	size_t i = 0;
	while(i < out.size())
	{
		const int ii = (i + 1) % out.size();
		const Contour::Vertex a = out[i];
		const Contour::Vertex b = out[ii];
		
		// Traverse the segment in lexilogical order so that the
		// max deviation is calculated similarly when traversing
		// opposite segments.
		int inc = 0, ci = 0, endi = 0;
		if (b > a)
		{
			inc = 1;
			ci = (a.flag + inc) % n;
			endi = b.flag;
		}
		else
		{
			inc = n - 1;
			ci = (b.flag + inc) % n;
			endi = a.flag;
		}
		
		float maxd = 0;
		int maxi = -1;
		// Tessellate only outer edges or edges between areas.
		if (((verts[ci].flag & RC_CONTOUR_REG_MASK) == 0) || (verts[ci].flag & RC_AREA_BORDER))
		{
			// Find maximum deviation from the segment.
			while(ci != endi)
			{
				float d = calcPointSegmentSqrDistance(verts[ci], a, b);
				if (d > maxd)
				{
					maxd = d;
					maxi = ci;
				}
				ci = (ci+inc) % n;
			}
		}
		
		// If the max deviation is larger than accepted error,
		// add new point, else continue to next segment.
		if (maxi != -1 && maxd > maxError * maxError)
		{
			Contour::Vertex v = verts[maxi]; v.flag = maxi;
			out.insert(out.begin() + i + 1, v);
		}
		else
		{
			++i;
		}
	}
	
	// Split too long edges.
	if (maxEdgeLen > 0 && (buildFlags & (RC_CONTOUR_TESS_WALL_EDGES|RC_CONTOUR_TESS_AREA_EDGES)) != 0)
	{
		size_t i = 0;
		const int maxEdgeLenSqr = maxEdgeLen * maxEdgeLen;
		
		while(i < out.size())
		{
			Contour::Vertex const& a = out[i];
			Contour::Vertex const& b = out[(i + 1) % out.size()];
			const int ci = (a.flag + 1) % n;
			int maxi = -1;
			
			if (((buildFlags & RC_CONTOUR_TESS_WALL_EDGES) && ((verts[ci].flag & RC_CONTOUR_REG_MASK) == 0)) || // Wall edges.
			    ((buildFlags & RC_CONTOUR_TESS_AREA_EDGES) && (verts[ci].flag & RC_AREA_BORDER)))               // Edges between areas.
			{
				const int dx = a.x - b.x;
				const int dz = a.z - b.z;
				if (dx*dx+dz*dz > maxEdgeLenSqr)
				{
					// Round based on the segments in lexilogical order so that the
					// max tesselation is consistent regardles in which direction
					// segments are traversed.
					const int n2 = b.flag < a.flag ? (b.flag + n - a.flag) : (b.flag - a.flag);
					if (n2 > 1)
					{
						if (b > a)
						{
							maxi = (a.flag + n2 / 2) % n;
						}
						else
						{
							maxi = (a.flag + (n2 + 1) / 2) % n;
						}
					}
				}
			}
			
			if (maxi != -1)
			{
				Contour::Vertex v = verts[maxi];
				v.flag = maxi;
				out.insert(out.begin() + i + 1, v);
			}
			else
			{
				++i;
			}
		}
	}
	
	// The edge vertex flag is take from the current raw point,
	// and the neighbour region is take from the next raw point.
	BOOST_FOREACH(Contour::Vertex& v, out)
	{
		const Contour::Vertex& a = verts[(v.flag + 1) % n];
		const Contour::Vertex& b = verts[v.flag];
		
		v.flag = (a.flag & RC_CONTOUR_REG_MASK) | (b.flag & RC_BORDER_VERTEX);
	}
	
	return out;
}

static void removeDegenerateSegments(std::vector<Contour::Vertex> & verts)
{
	std::vector<Contour::Vertex>::iterator it = verts.begin();
	
	while(it != verts.end())
	{
		std::vector<Contour::Vertex>::iterator next = it; ++next;
		if (next == verts.end()) next = verts.begin();
		
		if (it->x == next->x && it->z == next->z)
			it = verts.erase(it);
		else
			++it;
	}
}

static void markBoundaries(boost::scoped_array<std::vector<CompactSpan> > & cells, int xsize, int zsize)
{
	for(int z = 0; z < zsize; ++z)
	{
		for(int x = 0; x < xsize; ++x)
		{
			std::vector<CompactSpan> & cell = cells[x + z * xsize];
			for(int i = 0, n = cell.size(); i < n; ++i)
			{
				if (!cell[i]._regionID || (cell[i]._regionID & RC_BORDER_REG))
				{
					cell[i]._tmpData = 0;
				}
				else
				{
					cell[i]._tmpData = 0xf;
					for(int dir = Direction::Begin; dir < Direction::End; ++dir)
					{
						const unsigned int r = cell[i].neighbours[dir] ? cell[i].neighbours[dir]->_regionID : 0;
						if (r == cell[i]._regionID)
							cell[i]._tmpData &= ~(1 << dir);
					}
				}
			}
		}
	}
}

ContourSet::ContourSet(Recast::CompactHeightfield& chf, const float maxError, const int maxEdgeLen, const int buildFlags) :
	_cs(chf._cs),
	_ch(chf._ch),
	_xsize(chf._xsize - 2 * chf._borderSize),
	_zsize(chf._zsize - 2 * chf._borderSize),
	_borderSize(chf._borderSize)
{
	// Mark boundaries
	markBoundaries(chf._cells, chf._xsize, chf._zsize);
	
	for(int z = 0; z < chf._zsize; ++z)
	{
		for(int x = 0; x < chf._xsize; ++x)
		{
			std::vector<CompactSpan> & cell = chf._cells[x + z * chf._xsize];
			for(int i = 0, n = cell.size(); i < n; ++i)
			{
				if (cell[i]._tmpData == 0xf) cell[i]._tmpData = 0;
				if (cell[i]._tmpData == 0) continue;
				if (cell[i]._regionID == 0) continue;
				if (cell[i]._regionID & RC_BORDER_REG) continue;
				
				Contour cont;
				cont.rverts = walkContour(x, z, cell[i]);
				cont.verts = simplifyContour(cont.rverts, maxError, maxEdgeLen, buildFlags);
				removeDegenerateSegments(cont.verts);
				
				if (cont.verts.size() < 3) continue;
				
				cont.reg = cell[i]._regionID;
				cont.area = cell[i]._walkable;
				
				if (_borderSize > 0)
				{
					for(int j = 0, m = cont.verts.size(); j < m; ++j)
					{
						cont.verts[j].x -= _borderSize;
						cont.verts[j].z -= _borderSize;
					}
					for(int j = 0, m = cont.rverts.size(); j < m; ++j)
					{
						cont.rverts[j].x -= _borderSize;
						cont.rverts[j].z -= _borderSize;
					}
				}
				
				_conts.push_back(cont);
			}
		}
	}

	// Check and merge droppings.
	// Sometimes the previous algorithms can fail and create several contours
	// per area. This pass will try to merge the holes into the main region.
	BOOST_FOREACH(Contour& c, _conts)
	{
		if (calcPolygonArea(c) >= 0) continue; // Check if the contour is would backwards.
		
		// Find another contour which has the same region ID.
		BOOST_FOREACH(Contour& c2, _conts)
		{
			if ((&c != &c2) && !c2.verts.empty() && (c.reg == c2.reg) && (calcPolygonArea(c2) != 0))
			{
				mergeContours(c2, c);
				break;
			}
		}
	}
}

}
