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
#include <map>
#include <set>
#include <vector>
#include <climits>

#include "RecastCompactHeightfield.h"
#include <list>

namespace Recast
{
struct CompactHeightfield::Region
{
	Region(unsigned int _id) : spanCount(0), id(_id), areatype(0), remap(false), visited(false) {}
	int spanCount;
	unsigned int id;
	unsigned int areatype;
	bool remap;
	bool visited;
	std::set<unsigned int> connections;
	std::set<unsigned int> floors;
	
	bool isConnectedToBorder()
	{
		BOOST_FOREACH(unsigned int const & conn, connections)
		{
			if (conn == 0) return true;
		}
		
		return false;
	}
	
	bool canMergeWith(Region& r)
	{
		if (areatype != r.areatype) return false;
		
		BOOST_FOREACH(unsigned int const & conn, connections)
		{
			if (conn == r.id) return false;
		}
		
		BOOST_FOREACH(unsigned int const & floor, floors)
		{
			if (floor == r.id) return false;
		}
		
		return true;
	}
	
	void replaceNeighbours(unsigned int oldID, unsigned int newID)
	{
		std::set<unsigned int>::iterator it = connections.find(oldID);
		if (it != connections.end())
		{
			connections.erase(it);
			connections.insert(newID);
		}
		
		it = floors.find(oldID);
		if (it != floors.end())
		{
			floors.erase(it);
			floors.insert(newID);
		}
	}
	
	bool merge(Region& r)
	{
		std::set<unsigned int>::iterator it1, it2;
		if ((it1 = connections.find(r.id)) == connections.end()) return false;
		if ((it2 = r.connections.find(id)) == r.connections.end()) return false;
		
		connections.insert(r.connections.begin(), r.connections.end());
		connections.erase(it1);
		connections.erase(it2);
		
		floors.insert(r.floors.begin(), r.floors.end());
		
		spanCount += r.spanCount;
		
		r.spanCount = 0;
		r.connections.clear();
		r.floors.clear();
		return true;
	}
};

void CompactHeightfield::blurDistanceField(boost::scoped_array< unsigned int >& out, int threshold)
{
	for(size_t i = 0, size = _xsize * _zsize; i < size; ++i)
	{
		BOOST_FOREACH(CompactSpan& span, _cells[i])
		{
			if (span._borderDistance <= threshold)
			{
				out[span._tmpData] = threshold;
			}
			else
			{
				unsigned int d = span._borderDistance;
				for(int dir = Direction::Begin; dir < Direction::End; ++dir)
				{
					if (span.neighbours[dir])
					{
						const int dir2 = (dir + 1) % 4;
						d += span.neighbours[dir]->_borderDistance;
						
						if (span.neighbours[dir]->neighbours[dir2])
							d += span.neighbours[dir]->neighbours[dir2]->_borderDistance;
						else
							d += span.neighbours[dir]->_borderDistance;
					}
					else
						d += 2 * span._borderDistance;
				}
				out[span._tmpData] = (d + 5) / 9;
			}
		}
	}
	for(size_t i = 0, size = _xsize * _zsize; i < size; ++i)
	{
		BOOST_FOREACH(CompactSpan& span, _cells[i])
		{
			span._borderDistance = out[span._tmpData];
		}
	}
}

bool CompactHeightfield::fillRegion(CompactSpan& span, unsigned int level, unsigned int region,
	boost::scoped_array<unsigned int>& regions,
	boost::scoped_array<unsigned int>& distances)
{
	const bool walkable = span._walkable;
	const unsigned int lvl = level > 2 ? level - 2 : 0;
	std::vector<CompactSpan *> stack;
	bool ret = false;
	
	regions[span._tmpData] = region;
	distances[span._tmpData] = 0;
	
	stack.push_back(&span);
	
	while(!stack.empty())
	{
		CompactSpan& span = *stack.back(); stack.pop_back();
		
		bool neighbourHasRegion = false;
		for(int dir = Direction::Begin; dir < Direction::End; ++dir)
		{
			if (span.neighbours[dir])
			{
				CompactSpan& neighbour = *span.neighbours[dir];
				if (neighbour._walkable != walkable) continue;
				if (regions[neighbour._tmpData] & RC_BORDER_REG) continue;
				if (regions[neighbour._tmpData] != 0 && regions[neighbour._tmpData] != region)
					neighbourHasRegion = true;
				
				const int dir2 = (dir + 1) % 4;
				if (neighbour.neighbours[dir2])
				{
					CompactSpan& neighbour2 = *neighbour.neighbours[dir2];
					if (neighbour2._walkable != walkable) continue;
					if (regions[neighbour2._tmpData] & RC_BORDER_REG) continue;
					if (regions[neighbour2._tmpData] != 0 && regions[neighbour2._tmpData] != region)
						neighbourHasRegion = true;
				}
			}
		}
		
		if (neighbourHasRegion)
		{
			regions[span._tmpData] = 0;
		}
		else
		{
			ret = true;
			for(int dir = Direction::Begin; dir < Direction::End; ++dir)
			{
				if (span.neighbours[dir])
				{
					CompactSpan& neighbour = *span.neighbours[dir];
					if (neighbour._walkable != walkable) continue;
					
					if (neighbour._borderDistance >= lvl && regions[neighbour._tmpData] == 0)
					{
						regions[neighbour._tmpData] = region;
						distances[neighbour._tmpData] = 0;
						stack.push_back(&neighbour);
					}
				}
			}
		}
	}
	
	return ret;
}

void CompactHeightfield::paintRectRegion(
	const int minx, const int maxx,
	const int minz, const int maxz,
	const unsigned int regionID, boost::scoped_array<unsigned int>& regions)
{
	for(int x = minx; x < maxx; ++x)
	{
		for(int z = minz; z < maxz; ++z)
		{
			BOOST_FOREACH(CompactSpan& span, _cells[x + z * _xsize])
			{
				if (span._walkable) regions[span._tmpData] = regionID;
			}
		}
	}
}

void CompactHeightfield::expandRegions(const int maxIter, const unsigned int level,
	boost::scoped_array<unsigned int>& regions,
	boost::scoped_array<unsigned int>& distances)
{
	std::list<CompactSpan*> stack;
	
	for(int i = 0, size = _xsize * _zsize; i < size; ++i)
	{
		BOOST_FOREACH(CompactSpan& j, _cells[i])
		{
			if ((distances[j._tmpData] >= level) && (regions[j._tmpData] == 0) && j._walkable)
				stack.push_back(&j);
		}
	}
	
	int iter = maxIter;
	bool finished = false;
	boost::scoped_array<unsigned int> regionsCopy(new unsigned int[_spanNumber]);
	boost::scoped_array<unsigned int> distancesCopy(new unsigned int[_spanNumber]);
	while((iter > 0) && (!stack.empty()) && !finished)
	{
		memcpy(&regionsCopy[0], &regions[0], sizeof(unsigned int)*_spanNumber);
		memcpy(&distancesCopy[0], &distances[0], sizeof(unsigned int)*_spanNumber);
		
		std::list<CompactSpan*>::iterator it = stack.begin(), end = stack.end();
		
		finished = true;
		
		while(it != end)
		{
			CompactSpan & i = **it;
			unsigned int reg = regions[i._tmpData];
			assert(reg == 0);
			unsigned int dist = UINT_MAX;
			
			for(int dir = Direction::Begin; dir < Direction::End; ++dir)
			{
				if (i.neighbours[dir])
				{
					CompactSpan& n = *(i.neighbours[dir]);
					const unsigned int nr = regions[n._tmpData];
					const unsigned int nd = distances[n._tmpData];
					if (n._walkable == i._walkable && nr > 0 && (nr & RC_BORDER_REG) == 0 && (nd + 2) < dist)
					{
						reg = nr;
						dist = nd + 2;
					}
				}
			}
			
			if (reg)
			{
				regionsCopy[i._tmpData] = reg;
				distancesCopy[i._tmpData] = dist;
				it = stack.erase(it);
				finished = false;
			}
			else
			{
				++it;
			}
		}
		
		memcpy(&regions[0], &regionsCopy[0], sizeof(unsigned int)*_spanNumber);
		memcpy(&distances[0], &distancesCopy[0], sizeof(unsigned int)*_spanNumber);
		
		if (level > 0)
			--iter;
	}
}

static bool isSolidEdge(CompactSpan & span, int dir, boost::scoped_array<unsigned int> & regions)
{
	unsigned int r = 0;
	if (span.neighbours[dir])
	{
		r = regions[span.neighbours[dir]->_tmpData];
	}
	
	return r != regions[span._tmpData];
}

static void walkContour(CompactSpan & span, int dir, boost::scoped_array<unsigned int> & regions, std::set<unsigned int> & connections)
{
	int dir0 = dir;
	
	unsigned int curRegion = 0;
	if (span.neighbours[dir]) curRegion = span.neighbours[dir]->_regionID;
	
	connections.insert(curRegion);
	
	int iter = 40000;
	CompactSpan * i = &span;
	do
	{
		CompactSpan& s = *i;
		
		if (isSolidEdge(s, dir, regions))
		{
			unsigned int r = 0;
			if (s.neighbours[dir]) r = s.neighbours[dir]->_regionID;
			
			if (r != curRegion)
			{
				connections.insert(r);
				curRegion = r;
			}
			
			dir = (dir + 1) % 4;
		}
		else
		{
			assert(s.neighbours[dir] != 0);
			i = s.neighbours[dir];
			dir = (dir + 3) % 4;
		}
	} while(--iter > 0 && (i != &span || dir != dir0));
}

void CompactHeightfield::findConnections(std::vector< Recast::CompactHeightfield::Region >& regions, boost::scoped_array< unsigned int >& regionMap)
{
	for(size_t i = 0, size = _xsize * _zsize; i < size; ++i)
	{
		BOOST_FOREACH(CompactSpan& j, _cells[i])
		{
			unsigned int r = regionMap[j._tmpData];
			if (r == 0 || r > _maxRegions)
				continue;
			
			Region& reg = regions[r];
			reg.spanCount++;
			
			BOOST_FOREACH(CompactSpan& k, _cells[i])
			{
				if (&j != &k)
				{
					unsigned int floorId = regionMap[k._tmpData];
					if (floorId != 0 && floorId <= _maxRegions)
						reg.floors.insert(floorId);
				}
			}
			
			if (reg.connections.size() == 0)
			{
				reg.areatype = j._walkable;
				
				for(int dir = Direction::Begin; dir < Direction::End; ++dir)
				{
					if (isSolidEdge(j, dir, regionMap))
					{
						walkContour(j, dir, regionMap, reg.connections);
						break;
					}
				}
			}
		}
	}
}

void CompactHeightfield::removeTooSmallRegions(std::vector<Region> & regions, const int minRegionArea)
{
	for(int i = 0, size = regions.size(); i < size; ++i)
	{
		Region& r = regions[i];
		if (r.id == 0 || (r.id & RC_BORDER_REG)) continue;
		if (r.spanCount == 0) continue;
		if (r.visited) continue;
		
		bool connectsToBorder = false;
		int spanCount = 0;
		
		r.visited = true;
		std::vector<int> trace;
		std::vector<int> stack; stack.push_back(i);
		while(!stack.empty())
		{
			int j = stack.back();
			stack.pop_back();
			trace.push_back(j);
			Region& r2 = regions[j];
			
			spanCount += r2.spanCount;
			BOOST_FOREACH(unsigned int const & conn, r2.connections)
			{
				if (conn & RC_BORDER_REG)
				{
					connectsToBorder = true;
				}
				else
				{
					Region& neighbour = regions[conn];
					if (neighbour.visited) continue;
					if (neighbour.id == 0) continue;
					if (neighbour.id & RC_BORDER_REG) continue;
					
					stack.push_back(neighbour.id);
					neighbour.visited = true;
				}
			}
		}
		
		// If the accumulated regions size is too small, remove it.
                // Do not remove areas which connect to tile borders
                // as their size cannot be estimated correctly and removing them
                // can potentially remove necessary areas.
		if (spanCount < minRegionArea && !connectsToBorder)
		{
			BOOST_FOREACH(int& j, trace)
			{
				regions[j].spanCount = 0;
				regions[j].id = 0;
			}
		}
	}
}

void CompactHeightfield::mergeTooSmallRegions(std::vector<Region> & regions, const int mergeRegionSize)
{
	while(1)
	{
		bool regionMerged = false;
		
		BOOST_FOREACH(Region& r, regions)
		{
			if (r.id == 0) continue;
			if (r.id & RC_BORDER_REG) continue;
			if (r.spanCount == 0) continue;
			if (r.spanCount > mergeRegionSize && r.isConnectedToBorder()) continue;
			
			int smallest = INT_MAX;
			unsigned int mergeID = 0;
			BOOST_FOREACH(unsigned int const & conn, r.connections)
			{
				if (conn & RC_BORDER_REG) continue;
				Region& r2 = regions[conn];
				if (r2.id == 0 || (r2.id & RC_BORDER_REG)) continue;
				if (r2.spanCount < smallest && r.canMergeWith(r2) && r2.canMergeWith(r))
				{
					smallest = r2.spanCount;
					mergeID = r2.id;
				}
			}
			
			if (mergeID == 0) continue;
			
			unsigned int oldID = r.id;
			Region& target = regions[mergeID];
			
			if (target.merge(r))
			{
				BOOST_FOREACH(Region& r2, regions)
				{
					if (r2.id == 0) continue;
					if (r2.id & RC_BORDER_REG) continue;
					
					if (r2.id == oldID) r2.id = mergeID;
					r2.replaceNeighbours(oldID, mergeID);
				}
				regionMerged = true;
			}
		}
		
		if (!regionMerged) return;
	}
}

void CompactHeightfield::compressRegionId(std::vector< Recast::CompactHeightfield::Region >& regions, boost::scoped_array< unsigned int >& regionMap)
{
	BOOST_FOREACH(Region& r, regions)
	{
		r.remap = r.id != 0 && !(r.id & RC_BORDER_REG);
	}

	unsigned int newID = 0;
	for(int i = 0; i <= _maxRegions; ++i)
	{
		Region const & r = regions[i];
		if (!r.remap) continue;
		
		unsigned int oldID = r.id;
		newID++;
		for(int j = 0; j <= _maxRegions; ++j)
		{
			Region & r2 = regions[j];
			if (r2.id != oldID) continue;
			r2.id = newID;
			r2.remap = false;
		}
	}
	
	_maxRegions = newID;
	
	for(size_t i = 0, size = _xsize * _zsize; i < size; ++i)
	{
		BOOST_FOREACH(CompactSpan& s, _cells[i])
		{
			unsigned int & r = regionMap[s._tmpData];
			if ((r & RC_BORDER_REG) == 0) r = regions[r].id;
		}
	}
}

void CompactHeightfield::filterSmallRegions(const int minRegionArea, const int mergeRegionSize, boost::scoped_array< unsigned int >& regionMap)
{
	std::vector<Region> regions;
	
	for(int i = 0; i <= _maxRegions; ++i)
		regions.push_back(Region(i));
	
	findConnections(regions, regionMap);
	
	removeTooSmallRegions(regions, minRegionArea);
	
	mergeTooSmallRegions(regions, mergeRegionSize);
	
	compressRegionId(regions, regionMap);
}

void CompactHeightfield::buildRegions(const int minRegionArea, const int mergeRegionSize)
{
	unsigned int spanNb = 0;
	//Store an ID of each span in its tmp field
	for(int i = 0, size = _xsize * _zsize; i < size; ++i)
	{
		BOOST_FOREACH(CompactSpan& j, _cells[i])
		{
			j._tmpData = spanNb++;
		}
	}
	assert(spanNb == _spanNumber);
	boost::scoped_array<unsigned int> regions(new unsigned int[_spanNumber]),distance(new unsigned int[_spanNumber]);
	memset(&regions[0], 0, _spanNumber * sizeof(unsigned int));
	memset(&distance[0], 0, _spanNumber * sizeof(unsigned int));
	
	buildDistanceField();

	blurDistanceField(distance, 1);
	unsigned int regionID = 1;
	
	if (_borderSize > 0)
	{
		const int bx = std::min(_xsize, _borderSize);
		const int bz = std::min(_zsize, _borderSize);
		paintRectRegion(0,         bx,     0,         _zsize, regionID++ | RC_BORDER_REG, regions);
		paintRectRegion(_xsize-bx, _xsize, 0,         _zsize, regionID++ | RC_BORDER_REG, regions);
		paintRectRegion(0,         _xsize, 0,         bz,     regionID++ | RC_BORDER_REG, regions);
		paintRectRegion(0,         _xsize, _zsize-bz, bz,     regionID++ | RC_BORDER_REG, regions);
	}
	
	unsigned int level = _maxDistance;
	if (level % 2) ++level;
	
	while(level > 0)
	{
		level = (level > 2) ? (level - 2) : 0;
		
		expandRegions(8, level, regions, distance);
		
		for(int i = 0, size = _xsize * _zsize; i < size; ++i)
		{
			BOOST_FOREACH(CompactSpan& j, _cells[i])
			{
				if (j._borderDistance < level || regions[j._tmpData] != 0 || j._walkable == false)
					continue;
				if (fillRegion(j, level, regionID, regions, distance))
				{
					++regionID;
				}
			}
		}
	}
	
	expandRegions(8, 0, regions, distance);
	
	_maxRegions = regionID;
	filterSmallRegions(minRegionArea, mergeRegionSize, regions);
	
	for(int i = 0, size = _xsize * _zsize; i < size; ++i)
	{
		BOOST_FOREACH(CompactSpan& j, _cells[i])
		{
			j._regionID = regions[j._tmpData];
		}
	}
}

}