#include "Pathfinding.h"
#include "../DebugDrawer.h"

#include <OgreColourValue.h>

namespace Pathfinding
{
void NavMesh::DebugDraw()
{
	if (DrawHeightfield) DebugDrawHeightfield();

	if (DrawCompactHeightfield) DebugDrawCompactHeightfield();

	if (DrawRawContours) DebugDrawRawContours();

	if (DrawContours) DebugDrawContours();

	if (DrawPolyMesh) DebugDrawPolyMesh();

	if (DrawPolyMeshDetail) DebugDrawPolyMeshDetail();
}

void NavMesh::DebugDrawHeightfield()
{
	if (!hf) return;

	const float* orig = hf->bmin;

	const float cs = hf->cs;

	const float ch = hf->ch;

	const int w = hf->width;

	const int h = hf->height;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			// Deriving the minimum corner of the grid location.
			float fx = orig[0] + x * cs;
			float fz = orig[2] + y * cs;
			// The base span in the column. (May be null.)
			const rcSpan* s = hf->spans[x + y*w];

			while (s)
			{
				// Detriving the minium and maximum world position of the span.
				float fymin = orig[1] + s->smin * ch;
				float fymax = orig[1] + s->smax * ch;
				// Do other things with the span before moving up the column.

				Ogre::Vector3 v[8];
				v[0] = Ogre::Vector3(fx, fymin, fz);
				v[1] = Ogre::Vector3(fx, fymin, fz + cs);
				v[2] = Ogre::Vector3(fx + cs, fymin, fz + cs);
				v[3] = Ogre::Vector3(fx + cs, fymin, fz);
				v[4] = Ogre::Vector3(fx + cs, fymax, fz + cs);
				v[5] = Ogre::Vector3(fx, fymax, fz + cs);
				v[6] = Ogre::Vector3(fx, fymax, fz);
				v[7] = Ogre::Vector3(fx + cs, fymax, fz);

				DebugDrawer::getSingleton().drawCuboid(v, s->area ? Ogre::ColourValue::Blue : Ogre::ColourValue::Green, true);

				s = s->next;
			}
		}
	}
}

void NavMesh::DebugDrawCompactHeightfield()
{
	if (!chf) return;

	const float cs = chf->cs;

	const float ch = chf->ch;

	for (int y = 0; y < chf->height; ++y)
	{
		for (int x = 0; x < chf->width; ++x)
		{
			// Deriving the minimum corner of the grid location.
			const float fx = chf->bmin[0] + x * cs;
			const float fz = chf->bmin[2] + y * cs;

			// Get the cell for the grid location then iterate
			// up the column.
			const rcCompactCell& c = chf->cells[x+y*chf->width];

			for (unsigned i = c.index, ni = c.index + c.count; i < ni; ++i)
			{
				const rcCompactSpan& s = chf->spans[i];

				// Deriving the minimum (floor) of the span.
				const float fy = chf->bmin[1] + (s.y + 1) * ch;

				// Testing the area assignment of the span.
				if (chf->areas[i] != RC_NULL_AREA)
				{
					Ogre::Vector3 v[8];
					v[0] = Ogre::Vector3(fx, fy + 0.1, fz);
					v[1] = Ogre::Vector3(fx, fy + 0.1, fz + chf->cs);
					v[2] = Ogre::Vector3(fx + chf->cs, fy + 0.1, fz + chf->cs);
					v[3] = Ogre::Vector3(fx + chf->cs, fy + 0.1, fz);
					v[4] = Ogre::Vector3(fx + chf->cs, fy + 0.11, fz + chf->cs);
					v[5] = Ogre::Vector3(fx, fy + 0.11, fz + chf->cs);
					v[6] = Ogre::Vector3(fx, fy + 0.11, fz);
					v[7] = Ogre::Vector3(fx + chf->cs, fy + 0.11, fz);

					DebugDrawer::getSingleton().drawCuboid(v, Ogre::ColourValue::Blue, true);
				}
			}
		}
	}
}

void NavMesh::DebugDrawRawContours()
{
	if (!cset) return;

	const float cs = cset->cs;

	const float ch = cset->ch;

	for (int i = 0; i < cset->nconts; ++i)
	{
		rcContour const & cont = cset->conts[i];
		unsigned int r = cont.rverts[3];
		Ogre::ColourValue c(((r / 16) % 4) * 0.333, ((r / 4) % 4) * 0.333, (r % 4) * 0.333);

		for (int j = 0; j < cont.nrverts; ++j)
		{
			int k = (j + 1) % cont.nrverts;

			Ogre::Vector3 a(cset->bmin[0] + cont.rverts[4*j+0] * cs,
					cset->bmin[1] + cont.rverts[4*j+1] * ch,
					cset->bmin[2] + cont.rverts[4*j+2] * cs);

			Ogre::Vector3 b(cset->bmin[0] + cont.rverts[4*k+0] * cs,
					cset->bmin[1] + cont.rverts[4*k+1] * ch,
					cset->bmin[2] + cont.rverts[4*k+2] * cs);

			DebugDrawer::getSingleton().drawLine(a, b, c);
		}
	}
}

void NavMesh::DebugDrawContours()
{
	if (!cset) return;

	const float cs = cset->cs;

	const float ch = cset->ch;

	for (int i = 0; i < cset->nconts; ++i)
	{
		rcContour const & cont = cset->conts[i];
		unsigned int r = cont.verts[3] & RC_CONTOUR_REG_MASK;
		Ogre::ColourValue c(((r / 16) % 4) * 0.333, ((r / 4) % 4) * 0.333, (r % 4) * 0.333);

		for (int j = 0; j < cont.nverts; ++j)
		{
			int k = (j + 1) % cont.nverts;

			Ogre::Vector3 a(cset->bmin[0] + cont.verts[4*j+0] * cs,
					cset->bmin[1] + cont.verts[4*j+1] * ch,
					cset->bmin[2] + cont.verts[4*j+2] * cs);

			Ogre::Vector3 b(cset->bmin[0] + cont.verts[4*k+0] * cs,
					cset->bmin[1] + cont.verts[4*k+1] * ch,
					cset->bmin[2] + cont.verts[4*k+2] * cs);

			DebugDrawer::getSingleton().drawLine(a, b, c);
		}
	}
}

void NavMesh::DebugDrawPolyMesh()
{
	if (!mesh) return;
}

void NavMesh::DebugDrawPolyMeshDetail()
{
	if (!dmesh) return;

	for (int i = 0; i < dmesh->nmeshes; ++i)
	{
		const unsigned int* meshDef = &dmesh->meshes[i*4];
		const unsigned int baseVerts = meshDef[0];
		const unsigned int baseTri = meshDef[2];
		const int ntris = (int)meshDef[3];

		for (int j = 0; j < std::min(ntris, 10); ++j)
		{
			const unsigned char * tri = &dmesh->tris[(baseTri + j) * 4];

			Ogre::Vector3 abc[3];
			abc[0] = Ogre::Vector3(dmesh->verts[3 * (baseVerts + tri[0])],
					       dmesh->verts[3 * (baseVerts + tri[0]) + 1],
					       dmesh->verts[3 * (baseVerts + tri[0]) + 2]);
			abc[1] = Ogre::Vector3(dmesh->verts[3 * (baseVerts + tri[1])],
					       dmesh->verts[3 * (baseVerts + tri[1]) + 1],
					       dmesh->verts[3 * (baseVerts + tri[1]) + 2]);
			abc[2] = Ogre::Vector3(dmesh->verts[3 * (baseVerts + tri[2])],
					       dmesh->verts[3 * (baseVerts + tri[2]) + 1],
					       dmesh->verts[3 * (baseVerts + tri[2]) + 2]);

			Ogre::ColourValue col(((i / 16) % 4) * 0.333, ((i / 4) % 4) * 0.333, (i % 4) * 0.333);

			DebugDrawer::getSingleton().drawTri(abc, col, true);
		}
	}
}

}
