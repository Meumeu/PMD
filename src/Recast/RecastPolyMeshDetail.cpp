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

#include "RecastPolyMeshDetail.h"
#include "RecastPolyMesh.h"
#include "RecastCompactHeightfield.h"
#include "RecastCompactHeightfield.h"
#include "cdt/quadedge.h"

#include <boost/foreach.hpp>

#include <climits>
#include <cfloat>
#include <cmath>

#include <deque>
#include <list>
#include <utility>
#include <algorithm>

namespace Recast
{
struct CompactSpanPosition
{
	CompactSpanPosition(int _x, int _z, const CompactSpan * _span) : x(_x), z(_z), span(_span) {}
	CompactSpanPosition(IntVertex const & v, const CompactSpan * _span) : x(v.x), z(v.z), span(_span) {}
	CompactSpanPosition() : x(0), z(0), span(0) {}
	int x, z;
	const CompactSpan * span;
};

struct Bounds
{
	int xmin;
	int xmax;
	int zmin;
	int zmax;
	
	Bounds() : xmin(INT_MAX), xmax(INT_MIN), zmin(INT_MAX), zmax(INT_MIN) {};
	Bounds & operator<<(IntVertex const & v)
	{
		xmin = std::min(xmin, v.x - 1);
		xmax = std::max(xmax, v.x + 1);
		zmax = std::min(zmin, v.z - 1);
		zmax = std::max(zmax, v.z + 1);
		
		return *this;
	}
	
	int xsize() const { return xmax - xmin; }
	int zsize() const { return zmax - zmin; }	
};

class HeightData
{
	Bounds bounds;
	std::vector<int> height;
	float cs;
	float ics;
	float ch;

	IntVertex toIntVertex(FloatVertex const& v) const
	{
		int ix = std::floor(v.x * ics);
		int iz = std::floor(v.z * ics);
		return IntVertex(ix, 0, iz);
	}
public:
	FloatVertex toFloatVertex(IntVertex const& v) const {return FloatVertex(v.x * cs, v.y * ch, v.z * cs);}
	
	int getHeight(IntVertex const & v) const
	{
		if (v.x < bounds.xmin || v.x >= bounds.xmax || v.z < bounds.zmin || v.z >= bounds.zmax) return INT_MIN;
		return height[(v.x - bounds.xmin) + (v.z - bounds.zmin) * bounds.xsize()];
	}

	std::pair<IntVertex, float> getFarthestPointFromTriangle(FloatVertex const& a, FloatVertex const& b, FloatVertex const& c) const
	{
		IntVertex const& a1 = toIntVertex(a);
		IntVertex const& b1 = toIntVertex(b);
		IntVertex const& c1 = toIntVertex(c);

		int den = IntVertex::det2d(b1-a1, c1-a1);
		int sqrden = den * den;

		Bounds bnds;
		bnds << a1 << b1 << c1;
		IntVertex farthest;
		float maxdistance = -1;
		
		for(int x = bnds.xmin; x < bnds.xmax; ++x)
		{
			for(int z = bnds.zmin; z < bnds.zmax; ++z)
			{
				IntVertex m(x,0,z);
				int lambda = den * IntVertex::det2d(m-a1, c1-a1);
				int mu = den * IntVertex::det2d(b1-a1, m-a1);
				if (lambda >= 0 && mu >= 0 && lambda + mu < sqrden)
				{
					m.y = getHeight(m);
					float y = a.y + (b.y * lambda + c.y * mu) / (float)sqrden;
					if (std::abs(y - m.y * ch) > maxdistance)
					{
						farthest = m;
						maxdistance = std::abs(y - m.y * ch);
					}
				}
			}
		}
		
		return std::make_pair(farthest, maxdistance);
	}
	
	float getHeight(FloatVertex const & v) const
	{
		int ix = std::floor(v.x * ics + 0.01f);
		int iz = std::floor(v.z * ics + 0.01f);
		
		ix = std::min(std::max(ix, bounds.xmin), bounds.xmax - 1);
		iz = std::min(std::max(iz, bounds.zmin), bounds.zmax - 1);
		
		IntVertex iv(ix, 0, iz);
		
		int h = getHeight(iv);
		if (h != INT_MIN) return h * ch;
		
		static const IntVertex offset[8] =
		{
			IntVertex(-1,  0,  0),
			IntVertex(-1,  0, -1),
			IntVertex( 0,  0, -1),
			IntVertex( 1,  0, -1),
			IntVertex( 1,  0,  0),
			IntVertex( 1,  0,  1),
			IntVertex( 0,  0,  1),
			IntVertex(-1,  0,  1)
		};
		
		float dmin = FLT_MAX;
		for(int i = 0; i < 8; ++i)
		{
			const IntVertex nv = iv + offset[i];
			const int nh = getHeight(nv);
			if (nh == INT_MIN) continue;
			
			const float d = std::abs(v.y - nh * ch);
			if (d < dmin)
			{
				dmin = d;
				h = nh;
			}
		}
		
		assert(h != INT_MIN);
		
		return h * ch;
	}
	
	HeightData(CompactHeightfield const & chf, IntVertex const * poly) :
		cs(chf._cs), ics(1. / chf._cs), ch(chf._ch)
	{
		static const IntVertex offset[9] =
		{
			IntVertex(0,  0,  0),
			IntVertex(-1, 0, -1),
			IntVertex(0,  0, -1),
			IntVertex(1,  0, -1),
			IntVertex(1,  0,  0),
			IntVertex(1,  0,  1),
			IntVertex(0,  0,  1),
			IntVertex(-1, 0,  1),
			IntVertex(-1, 0,  0)
		};
		
		int n = 0;
		IntVertex centre(0, 0, 0);
		
		for(size_t i = 0; i < PolyMesh::Polygon::NVertices; ++i)
		{
			bounds << poly[i];
			centre += poly[i];
			++n;
		}
		centre /= n;
		height.resize(bounds.xsize() * bounds.zsize(), 0);
		
		// Add the vertices to the stack
		std::vector<CompactSpanPosition> stack;
		for(size_t i = 0; i < PolyMesh::Polygon::NVertices; ++i)
		{
			std::pair<int, CompactSpanPosition> bestdistpos(INT_MAX, CompactSpanPosition());
			for(size_t j = 0; j < 9; ++j)
			{
				std::pair<int, const CompactSpan *> distpos = chf.findSpan(poly[i] + offset[j]);
				
				if (distpos.first < bestdistpos.first)
					bestdistpos = std::make_pair(distpos.first, CompactSpanPosition(poly[i] + offset[j], distpos.second));
			}
			
			if (bestdistpos.second.span) stack.push_back(bestdistpos.second);
		}
		
		
		// Find the polygon centre
		CompactSpanPosition centre_position;
		while(!stack.empty())
		{
			CompactSpanPosition pos = stack.back(); stack.pop_back();
			
			if (std::abs(pos.x - centre.x) <= 1 && std::abs(pos.z - centre.z) <= 1)
			{
				centre_position = pos;
				stack.clear();
				break;
			}
			
			for(int dir = Direction::Begin; dir < Direction::End; ++dir)
			{
				if (!pos.span->neighbours[dir]) continue;
				CompactSpan * n = pos.span->neighbours[dir];
				int nx = pos.x + Direction::getXOffset(dir);
				int nz = pos.z + Direction::getZOffset(dir);
				
				if (nx < bounds.xmin || nx >= bounds.xmax || nz < bounds.zmin || nz >= bounds.zmax) continue;
				int idx = (nx - bounds.xmin) + (nz - bounds.zmin) * bounds.xsize();
				
				if (height[idx] == 1) continue;
				
				height[idx] = 1;
				stack.push_back(CompactSpanPosition(nx, nz, n));
			}
		}
		
		if (centre_position.span == 0) return;
		
		std::deque<CompactSpanPosition> q;
		for(size_t i = 0, size = height.size(); i < size; ++i) height[i] = INT_MIN;
		
		// Mark start location
		int idx = (centre_position.x - bounds.xmin) + (centre_position.z - bounds.zmin) * bounds.xsize();
		height[idx] = centre_position.span->_bottom;
		q.push_back(centre_position);
		
		while(!q.empty())
		{
			CompactSpanPosition pos = q.front(); q.pop_front();
			
			for(int dir = Direction::Begin; dir < Direction::End; ++dir)
			{
				if (!pos.span->neighbours[dir]) continue;
				CompactSpan * n = pos.span->neighbours[dir];
				int nx = pos.x + Direction::getXOffset(dir);
				int nz = pos.z + Direction::getZOffset(dir);
				
				if (nx < bounds.xmin || nx >= bounds.xmax || nz < bounds.zmin || nz >= bounds.zmax) continue;
				int idx = (nx - bounds.xmin) + (nz - bounds.zmin) * bounds.xsize();
				
				if (height[idx] != INT_MIN) continue;
				
				height[idx] = n->_bottom;
				q.push_back(CompactSpanPosition(nx, nz, n));
			}
		}
	}
};

static float pointSegmentDistanceSqr(FloatVertex const & p, FloatVertex const & a, FloatVertex const & b)
{
	FloatVertex ab = b - a;
	FloatVertex ap = p - a;
	float d = ab.dot(ab);
	float t = ab.dot(ap);
	
	if (d > 0) t /= d;
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	
	FloatVertex v = a + t * ab -p;
	return v.dot(v);
}

class DistanceAccumulatoroid
{
public:
	DistanceAccumulatoroid(HeightData const& hd) : _hd(hd), _maxDistance(-1) {}
	void operator()(Delaunay::Point2d const& a, Delaunay::Point2d const& b, Delaunay::Point2d const& c)
	{
		FloatVertex a1(a.getX(), 0, a.getY()); a1.y = _hd.getHeight(a1);
		FloatVertex b1(b.getX(), 0, b.getY()); b1.y = _hd.getHeight(b1);
		FloatVertex c1(c.getX(), 0, c.getY()); c1.y = _hd.getHeight(c1);
		
		std::pair<IntVertex, float> x = _hd.getFarthestPointFromTriangle(a1, b1, c1);

		if (x.second > _maxDistance)
		{
			_farthestPoint = x.first;
			_maxDistance = x.second;
		}
	}
	float getFarthestDistance() { return _maxDistance; }
	FloatVertex getFarthestPoint() { return _hd.toFloatVertex(_farthestPoint); }
private:
	HeightData const& _hd;
	IntVertex _farthestPoint;
	float _maxDistance;
};

static void buildDetail(std::vector<FloatVertex> const & poly, const CompactHeightfield & /* chf */, const float sampleDist, const float sampleMaxError, const HeightData & hd)
{
	assert(poly.size() == 3);
	Delaunay::Mesh triangles(poly[0], poly[1], poly[2]);
	
	for(size_t size = poly.size(), i = 0, j = size - 1; i < size; j = i++)
	{
		std::vector<FloatVertex> edge;
		
		const FloatVertex & vi = std::min(poly[i], poly[j]);
		const FloatVertex & vj = std::max(poly[i], poly[j]);
		const FloatVertex d = vi - vj;
		const float length = std::sqrt(d.x * d.x + d.z * d.z);
		const int n = 1 + std::floor(length / sampleDist);
		
		edge.resize(n+1);
		for(int k = 0; k <= n; ++k)
		{
			FloatVertex & v = edge[k];
			float lambda = (float)k / (float)n;
			v = vj + d * lambda;
			v.y = hd.getHeight(v);
		}
		
		std::list<size_t> indices;
		indices.push_back(0);
		indices.push_back(n);
		
		for(std::list<size_t>::iterator it2 = indices.begin(), it1 = it2++; it2 != indices.end(); )
		{
			const FloatVertex & v1 = edge[*it1];
			const FloatVertex & v2 = edge[*it2];
			
			float maxd = -1;
			int idx = -1;
			for(size_t i = 1 + *it1; i < *it2; ++i)
			{
				float d = pointSegmentDistanceSqr(edge[i], v1, v2);
				if (d > maxd)
				{
					maxd = d;
					idx = i;
				}
			}
			
			if (maxd > sampleMaxError * sampleMaxError)
			{
				it2 = indices.insert(it2, idx);
			}
			else
			{
				it1 = it2++;
			}
		}
		
		indices.pop_back();
		indices.pop_front();
		BOOST_FOREACH(size_t & i, indices)
		{
			triangles.InsertSite(edge[i]);
		}
	}

	while (true)
	{
		DistanceAccumulatoroid d(hd);
		triangles.ApplyTriangles(d);
		if (d.getFarthestDistance() < sampleMaxError) break;
		triangles.InsertSite(d.getFarthestPoint());
	}
	
}

PolyMeshDetail::PolyMeshDetail(const Recast::PolyMesh& pm, const CompactHeightfield & chf, const float sampleDist, const float sampleMaxError)
{
//	meshes.reserve(pm.polys.size());
	
	for(size_t i = 0, size = pm.polys.size(); i < size; ++i)
	{
		const HeightData hd(chf, pm.polys[i].vertices);

		std::vector<FloatVertex> poly(PolyMesh::Polygon::NVertices);
		for(size_t j = 0; j < PolyMesh::Polygon::NVertices; ++j)
		{
			poly[j].x = pm.polys[i].vertices[j].x * chf._cs;
			poly[j].y = pm.polys[i].vertices[j].y * chf._ch;
			poly[j].z = pm.polys[i].vertices[j].z * chf._cs;
		}
		
		buildDetail(poly, chf, sampleDist, sampleMaxError, hd);
	}
}

}