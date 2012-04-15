#ifndef DEBUGDRAWER_H_INCLUDED
#define DEBUGDRAWER_H_INCLUDED

#include <OgreSceneNode.h>
#include <map>
#include <boost/cstdint.hpp>
using boost::uint64_t;

typedef std::pair<Ogre::Vector3, Ogre::ColourValue> VertexPair;

#define DEFAULT_ICOSPHERE_RECURSION_LEVEL	1

class IcoSphere
{
public:
	struct TriangleIndices
	{
		int v1, v2, v3;
 
		TriangleIndices(int _v1, int _v2, int _v3) : v1(_v1), v2(_v2), v3(_v3) {}
 
		bool operator < (const TriangleIndices &o) const { return v1 < o.v1 && v2 < o.v2 && v3 < o.v3; }
	};
 
	struct LineIndices
	{
		int v1, v2;
 
		LineIndices(int _v1, int _v2) : v1(_v1), v2(_v2) {}
 
		bool operator == (const LineIndices &o) const
		{
			return (v1 == o.v1 && v2 == o.v2) || (v1 == o.v2 && v2 == o.v1);
		}
	};
 
	IcoSphere();
	~IcoSphere();
 
	void create(int recursionLevel);
	void addToLineIndices(int baseIndex, std::vector<int> *target);
	int addToVertices(std::vector<VertexPair> *target, const Ogre::Vector3 &position, const Ogre::ColourValue &colour, float scale);
	void addToTriangleIndices(int baseIndex, std::vector<int> *target);
 
private:
	int addVertex(const Ogre::Vector3 &vertex);
	void addLineIndices(int index0, int index1);
	void addTriangleLines(int index0, int index1, int index2);
	int getMiddlePoint(int index0, int index1);
	void addFace(int index0, int index1, int index2);
 
	void removeLineIndices(int index0, int index1);
 
	std::vector<Ogre::Vector3> vertices;
	std::vector<LineIndices> lineIndices;
	std::vector<int> triangleIndices;
	std::vector<TriangleIndices> faces;
	std::map<uint64_t, int> middlePointIndexCache;
	int index;
};
 
class DebugDrawer
{
	DebugDrawer();
	DebugDrawer(DebugDrawer &);
public:
	DebugDrawer(Ogre::SceneManager *_sceneManager, float _fillAlpha);
	~DebugDrawer();
 
	void build();
 
	void setIcoSphereRecursionLevel(int recursionLevel);
 
	void drawLine(const Ogre::Vector3 &start, const Ogre::Vector3 &end, const Ogre::ColourValue &colour);
	void drawCircle(const Ogre::Vector3 &centre, float radius, int segmentsCount, const Ogre::ColourValue& colour, bool isFilled = false);
	void drawCylinder(const Ogre::Vector3 &centre, float radius, int segmentsCount, float height, const Ogre::ColourValue& colour, bool isFilled = false);
	void drawQuad(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, bool isFilled = false);
	void drawTri(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, bool isFilled = false);
	void drawCuboid(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, bool isFilled = false);
	void drawSphere(const Ogre::Vector3 &centre, float radius, const Ogre::ColourValue& colour, bool isFilled = false);
	void drawTetrahedron(const Ogre::Vector3 &centre, float scale, const Ogre::ColourValue& colour, bool isFilled = false);

	bool getEnabled() { return isEnabled; }
	void setEnabled(bool _isEnabled)
	{
		sceneNode->setVisible(_isEnabled);
		isEnabled = _isEnabled;
	}  
	void switchEnabled() { setEnabled(!isEnabled); }
	
	void clear();
 
private:
 
	Ogre::SceneManager *sceneManager;
	Ogre::ManualObject *manualObject;
	Ogre::SceneNode * sceneNode;
	float fillAlpha;
	IcoSphere icoSphere;
	
	bool isEnabled;
 
	std::vector<VertexPair> lineVertices, triangleVertices;
	std::vector<int> lineIndices, triangleIndices;
 
	int linesIndex, trianglesIndex;

	void initialise();
	void shutdown();
 
	void buildLine(const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::ColourValue& colour, float alpha = 1.0f);
	void buildQuad(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, float alpha = 1.0f);
	void buildTri(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, float alpha = 1.0f);
	void buildFilledQuad(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, float alpha = 1.0f);
	void buildFilledTriangle(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, float alpha = 1.0f);
	void buildCuboid(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, float alpha = 1.0f);
	void buildFilledCuboid(const Ogre::Vector3 *vertices, const Ogre::ColourValue& colour, float alpha = 1.0f);
 
	void buildCircle(const Ogre::Vector3 &centre, float radius, int segmentsCount, const Ogre::ColourValue& colour, float alpha = 1.0f);
	void buildFilledCircle(const Ogre::Vector3 &centre, float radius, int segmentsCount, const Ogre::ColourValue& colour, float alpha = 1.0f);
 
	void buildCylinder(const Ogre::Vector3 &centre, float radius, int segmentsCount, float height, const Ogre::ColourValue& colour, float alpha = 1.0f);
	void buildFilledCylinder(const Ogre::Vector3 &centre, float radius, int segmentsCount, float height, const Ogre::ColourValue& colour, float alpha = 1.0f);

	void buildTetrahedron(const Ogre::Vector3 &centre, float scale, const Ogre::ColourValue &colour, float alpha = 1.0f);
	void buildFilledTetrahedron(const Ogre::Vector3 &centre, float scale, const Ogre::ColourValue &colour, float alpha = 1.0f);

	int addLineVertex(const Ogre::Vector3 &vertex, const Ogre::ColourValue &colour);
	void addLineIndices(int index1, int index2);
 
	int addTriangleVertex(const Ogre::Vector3 &vertex, const Ogre::ColourValue &colour);
	void addTriangleIndices(int index1, int index2, int index3);
 
	void addQuadIndices(int index1, int index2, int index3, int index4);
};
 
#endif