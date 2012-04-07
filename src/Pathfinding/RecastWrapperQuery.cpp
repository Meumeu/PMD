#include "Pathfinding.h"
#include "Detour/DetourCommon.h"

#include <boost/scoped_array.hpp>
#include <stdlib.h>

namespace Pathfinding
{
NavMesh::Path::Path(std::shared_ptr<dtNavMesh> navmeshref,
		    const float * start,
		    const float * end,
		    const float * extent,
		    const dtQueryFilter & filter) : navmesh(navmeshref), vertices()
{
	dtNavMeshQuery navmeshquery;
	
	if (dtStatusFailed(navmeshquery.init(navmesh.get(), 2048)))
	{
		throw std::bad_alloc();
	}

	dtPolyRef startpoly, endpoly;

	if (dtStatusFailed(navmeshquery.findNearestPoly(start, extent, &filter, &startpoly, 0)))
	{
		std::cerr << "Warning: cannot find start poly (" << start[0] << ", " << start[1] << ", " << start[2] << ")\n";
		return;
	}

	if (dtStatusFailed(navmeshquery.findNearestPoly(end, extent, &filter, &endpoly, 0)))
	{
		std::cerr << "Warning: cannot find end poly (" << end[0] << ", " << end[1] << ", " << end[2] << ")\n";
		return;
	}

	if (startpoly == 0)
	{
		std::cerr << "Warning: cannot find start poly (" << start[0] << ", " << start[1] << ", " << start[2] << ")\n";
		return;
	}

	if (endpoly == 0)
	{
		std::cerr << "Warning: cannot find end poly (" << end[0] << ", " << end[1] << ", " << end[2] << ")\n";
		return;
	}

	
	const int buffersize = 10000;

	int npolys = 0;

	boost::scoped_array<dtPolyRef> polys(new dtPolyRef[buffersize]);

	dtStatus sta;
	if (dtStatusFailed(sta = navmeshquery.findPath(
		startpoly, endpoly,
		start, end,
		&filter,
		polys.get(), &npolys, buffersize)))
	{
		std::cerr << "Warning: findPath failed, status = " << sta << "\n";
		return;
	}

	if (npolys > 0)
	{
		float end2[3];
		dtVcopy(end2, end);

		if (polys[npolys - 1] != endpoly)
		{
			navmeshquery.closestPointOnPoly(polys[npolys - 1], end, end2);
		}


		const int maxvertices = 10000;

		int nvertices = 0;

		boost::scoped_array<float> buffer(new float[maxvertices * 3]);

		navmeshquery.findStraightPath(start, end2, polys.get(), npolys, buffer.get(), 0, 0, &nvertices, maxvertices);
		//std::cerr << "nvertices=" << nvertices << "\n"; 
		vertices.resize(nvertices);

		for (int i = 0; i < nvertices; ++i)
			vertices[i] = Vertex(buffer[3*i], buffer[3*i+1], buffer[3*i+2]);
	}
}
}
