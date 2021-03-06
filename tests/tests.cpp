#ifdef _MSC_VER
#pragma warning(disable:4305)
#pragma warning(disable:4244)

#define M_PI 3.1415926535897932384626433832795
#endif

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>

#include <iostream>
#include <fstream>

extern ContactAddedCallback	gContactAddedCallback;
static bool CustomMaterialCombinerCallback(
	btManifoldPoint& cp,
	const btCollisionObject* colObj0,
	int partId0,
	int index0,
	const btCollisionObject* colObj1,
	int partId1,
	int index1)
{

///make sure colObj0 is the triangle mesh

   //if (enable)
   {
      btAdjustInternalEdgeContacts(cp,colObj1,colObj0, partId1,index1);
   }

   return false;
}

int main(int argc, char * argv[])
{
	btCollisionConfiguration * CollisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher * Dispatcher = new btCollisionDispatcher(CollisionConfiguration);
	btBroadphaseInterface * OverlappingPairCache = new btDbvtBroadphase();
	btConstraintSolver * Solver = new btSequentialImpulseConstraintSolver();

	btDynamicsWorld * World = new btDiscreteDynamicsWorld(Dispatcher, OverlappingPairCache, Solver, CollisionConfiguration);

	World->setGravity(btVector3(0, -20, 0));
	
	btTriangleMesh Trimesh;

	for(float x = 0; x < 100; x += 1)
	{
		for(float z = -3; z < 3; z += 1)
		{
			btVector3 v1(x+0, 0, z+0);
			btVector3 v2(x+0, 0, z+1);
			btVector3 v3(x+1, 0, z+0);
			btVector3 v4(x+1, 0, z+1);
			Trimesh.addTriangle(v1, v2, v3);
			Trimesh.addTriangle(v2, v3, v4);
		}
	}
	
	btBvhTriangleMeshShape TrimeshShape(&Trimesh, true);

	btRigidBody * body = new btRigidBody(0, new btDefaultMotionState, &TrimeshShape);
	World->addRigidBody(body);

	btTriangleInfoMap * triinfomap = new btTriangleInfoMap();
	btGenerateInternalEdgeInfo(&TrimeshShape, triinfomap);
	gContactAddedCallback = CustomMaterialCombinerCallback;
	body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK | btCollisionObject::CF_STATIC_OBJECT);

	btVector3 inertia;
	btBoxShape box(btVector3(0.3, 0.3, 0.3));
	box.calculateLocalInertia(10, inertia);

	btRigidBody * cube = new btRigidBody(10, new btDefaultMotionState(btTransform(btQuaternion::getIdentity(), btVector3(1, 0.3, 0))), &box, inertia);
	World->addRigidBody(cube);

	cube->setLinearVelocity(btVector3(15,0,0.1));
	//cube->setFriction(0);

	body->setContactProcessingThreshold(0);
	cube->setContactProcessingThreshold(0);

	std::fstream f("out.txt", std::fstream::out);

	const float dt = 1.0 / 60.0;
	for(float t = 0; t < 10; t += dt)
	{
		World->stepSimulation(1.0/60.0,1000,1.0/60.0);
		btVector3 pos = cube->getCenterOfMassPosition();
		
		f.width(12);
		f << std::left << t;
		f.width(12);
		f << pos.x();
		f.width(12);
		f << pos.y();
		f.width(12);
		f << pos.z();
		f << "\n";
	}

	return 0;
}

