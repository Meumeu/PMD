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

#include <float.h>

#include "RecastHeightfield.h"

#include <algorithm>
#include <cmath>

#include <boost/foreach.hpp>

#include <climits>

namespace Recast
{

typedef std::map<std::pair<int,int>, std::list<Span> > span_container_t;

Heightfield::Heightfield(float cellSize, float cellHeight, float walkableSlopeAngle, int flagMergeThr):
	_xmin(INT_MAX), _xmax(INT_MIN),
	_zmin(INT_MAX), _zmax(INT_MIN),
	_cs(cellSize), _ch(cellHeight),
	_cosWalkableAngle(std::cos(walkableSlopeAngle)),
	_flagMergeThr(flagMergeThr)
{
}

void Span::merge(Span const& other, const int flagMergeThr)
{
	_smin = std::min(other._smin, _smin);
	_smax = std::max(other._smax, _smax);
	if (std::abs((int)_smax - (int)other._smax) <= flagMergeThr)
	{
		_walkable |= other._walkable;
	}
}

void Heightfield::addSpan(const int x, const int z,
	const unsigned short smin, const unsigned short smax,
	bool walkable)
{
	_xmin = std::min(_xmin, x);
	_xmax = std::max(_xmax, x);
	_zmin = std::min(_zmin, z);
	_zmax = std::max(_zmax, z);
	Span s(smin, smax, walkable);

	span_container_t::iterator cell = _spans.find(std::make_pair(x,z));
	//If the cell is empty, add a new list
	if (cell == _spans.end())
	{
		std::list<Span> newList;
		newList.push_back(s);
		_spans.insert(std::make_pair(std::make_pair(x,z), newList));
		return;
	}
	std::list<Span> & intervals = cell->second;
	std::list<Span>::iterator target= intervals.begin();
	std::list<Span>::iterator end = intervals.end();
	while(target != end)
	{
		if (target->_smin > s._smax)
		{
			intervals.insert(target, s);
			return;
		}
		else if (target->_smax >= s._smin)
		{
			// Merge spans, remove the current one
			s.merge(*target, _flagMergeThr);
			target = intervals.erase(target);
		}
		else
		{
			target++;
		}
	}
	
	intervals.insert(end, s);
}

std::list<Span> const& Heightfield::getSpans(int x, int z) const
{
	static std::list<Span> empty;
	std::map<std::pair<int,int>, std::list<Span> >::const_iterator it = _spans.find(std::make_pair(x, z));
	if (it == _spans.end())
		return empty;

	return it->second;
}

class PlaneDistance
{
public:
	PlaneDistance(Ogre::Plane const & _p) : p(_p) {}
	float operator()(Ogre::Vector3 const & v) { return p.getDistance(v); }
private:
	Ogre::Plane p;
};

static std::vector<Ogre::Vector3> clipPoly(std::vector<Ogre::Vector3> const & in, Ogre::Plane const & p)
{
	std::vector<Ogre::Vector3> out;
	
	std::vector<float> d;
	d.resize(in.size());
	std::transform(in.begin(), in.end(), d.begin(), PlaneDistance(p));
	
	size_t n = in.size();

	for(size_t i = 0, j = n - 1; i < n; j = i, ++i)
	{
		bool iVertexOnTheCorrectSide = d[i] >= 0;
		bool jVertexOnTheCorrectSide = d[j] >= 0;
		if (iVertexOnTheCorrectSide != jVertexOnTheCorrectSide)
		{
			out.push_back(in[j] - (in[j] - in[i]) * d[j] / (d[j] - d[i]));
		}
		if (jVertexOnTheCorrectSide)
		{
			out.push_back(in[j]);
		}
	}

	return out;
}

static bool getPolyMinMax(
	std::vector<Ogre::Vector3> in,
	float x, float z, float cs, float ch,
	int& min, int& max)
{
	std::vector<Ogre::Vector3> clippedIn = clipPoly(clipPoly(clipPoly(clipPoly(in,
		Ogre::Plane(Ogre::Vector3::UNIT_X, x)),
		Ogre::Plane(Ogre::Vector3::NEGATIVE_UNIT_X, -x-cs)),
		Ogre::Plane(Ogre::Vector3::UNIT_Z, z)),
		Ogre::Plane(Ogre::Vector3::NEGATIVE_UNIT_Z, -z-cs));

	if (clippedIn.size() < 3) return false;

	float fmin, fmax;
	fmin = fmax = in[0].y;
	for(size_t i = 1; i < clippedIn.size(); ++i)
	{
		fmin = std::min(fmin, clippedIn[i].y);
		fmax = std::max(fmax, clippedIn[i].y);
	}

	min = std::floor(fmin / ch);
	max = std::ceil(fmax / ch);
	
	
	return true;
}

static bool isWalkable(Ogre::Vector3 const& v0, Ogre::Vector3 const& v1, Ogre::Vector3 const& v2, float cosWalkableAngle)
{
	//return std::abs((v1-v0).crossProduct(v2-v0).normalisedCopy().y) > cosWalkableAngle;
	return (v1-v0).crossProduct(v2-v0).normalisedCopy().y > cosWalkableAngle;
}

void Heightfield::rasterizeTriangle(Ogre::Vector3 const& v0, Ogre::Vector3 const& v1, Ogre::Vector3 const& v2)
{
	Ogre::AxisAlignedBox tribox;
	tribox.merge(v0);
	tribox.merge(v1);
	tribox.merge(v2);
	
	std::vector<Ogre::Vector3> triangle;
	triangle.push_back(v0);
	triangle.push_back(v1);
	triangle.push_back(v2);

	int x  = std::floor(tribox.getMinimum().x / _cs);
	int x1 = std::ceil (tribox.getMaximum().x / _cs);
	int z0 = std::floor(tribox.getMinimum().z / _cs);
	int z1 = std::ceil (tribox.getMaximum().z / _cs);
	
	bool walkable = isWalkable(v0, v1, v2, _cosWalkableAngle);
	
	for(; x < x1; ++x)
	{
		for(int z = z0; z < z1; ++z)
		{
			int min, max;
			if (getPolyMinMax(triangle, x * _cs, z * _cs, _cs, _ch, min, max))
			{
				addSpan(x, z, min, max, walkable);
			}
		}
	}
}


}
