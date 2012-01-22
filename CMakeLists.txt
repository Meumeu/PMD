cmake_minimum_required(VERSION 2.6)

project(poniesmustdie)

if(WIN32)
	set(CMAKE_MODULE_PATH "$ENV{OGRE_HOME}/CMake/;${CMAKE_MODULE_PATH}")
endif(WIN32)

if(UNIX)
	if(EXISTS "/usr/local/lib/OGRE/cmake")
	  set(CMAKE_MODULE_PATH "/usr/local/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
	elseif(EXISTS "/usr/lib/OGRE/cmake")
	  set(CMAKE_MODULE_PATH "/usr/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
	elseif(EXISTS "/usr/share/OGRE/cmake/modules")
	  set(CMAKE_MODULE_PATH "/usr/share/OGRE/cmake/modules;${CMAKE_MODULE_PATH}")
	elseif(EXISTS "/usr/share/OGRE/cmake")
	  set(CMAKE_MODULE_PATH "/usr/share/OGRE/cmake/;${CMAKE_MODULE_PATH}")
	else ()
	  message(SEND_ERROR "Failed to find module path.")
	endif()
endif(UNIX)

if (CMAKE_BUILD_TYPE STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-D_DEBUG)
else()
	add_definitions(-DNDEBUG)
endif()

if (PHYSICS_DEBUG STREQUAL "y")
	add_definitions(-DPHYSICS_DEBUG)
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist")

find_package(OGRE REQUIRED)
find_package(OIS REQUIRED)
pkg_check_modules(CEGUI-OGRE REQUIRED CEGUI-OGRE)
use_pkgconfig(CEGUI_PKGC CEGUI-OGRE)

if(NOT OIS_FOUND)
	message(SEND_ERROR "Failed to find OIS.")
endif()

if (OGRE_BUILD_PLATFORM_IPHONE)
	message(SEND_ERROR "Unsupported platform.")
endif()

# Find Boost
if (WIN32 OR APPLE)
	set(Boost_USE_STATIC_LIBS TRUE)
else ()
	# Statically linking boost to a dynamic Ogre build doesn't work on Linux 64bit
	set(Boost_USE_STATIC_LIBS ${OGRE_STATIC})
endif ()
if (MINGW)
	# this is probably a bug in CMake: the boost find module tries to look for
	# boost libraries with name libboost_*, but CMake already prefixes library
	# search names with "lib". This is the workaround.
	set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "")
endif ()
set(Boost_ADDITIONAL_VERSIONS "1.46.1" "1.44")
# Components that need linking (NB does not include header-only components like bind)
set(OGRE_BOOST_COMPONENTS thread date_time filesystem system)
find_package(Boost COMPONENTS ${OGRE_BOOST_COMPONENTS} QUIET)
if (NOT Boost_FOUND)
	# Try again with the other type of libs
	set(Boost_USE_STATIC_LIBS NOT ${Boost_USE_STATIC_LIBS})
	find_package(Boost COMPONENTS ${OGRE_BOOST_COMPONENTS} QUIET)
endif()
find_package(Boost QUIET)

# Set up referencing of Boost
include_directories(${Boost_INCLUDE_DIR})
add_definitions(-DBOOST_ALL_NO_LIB)
set(OGRE_LIBRARIES ${OGRE_LIBRARIES} ${Boost_LIBRARIES} ${CEGUI-OGRE_LIBRARIES})

set (HDRS
	src/AppStateManager.h
	src/Game.h
	src/pmd.h
	src/environment.h
	src/CharacterController.h
	src/CharacterAnimation.h
	src/AppState.h
	src/RigidBody.h
	src/MainMenu.h

	src/btOgre/BtOgreGP.h
	src/btOgre/BtOgrePG.h
	src/btOgre/BtOgreExtras.h

	src/bullet/btBulletCollisionCommon.h
	src/bullet/BulletDynamics/Vehicle/btVehicleRaycaster.h
	src/bullet/BulletDynamics/Vehicle/btWheelInfo.h
	src/bullet/BulletDynamics/Vehicle/btRaycastVehicle.h
	src/bullet/BulletDynamics/ConstraintSolver/btHingeConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btContactSolverInfo.h
	src/bullet/BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btHinge2Constraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btSolverConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btUniversalConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h
	src/bullet/BulletDynamics/ConstraintSolver/btContactConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btConeTwistConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btTypedConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btJacobianEntry.h
	src/bullet/BulletDynamics/ConstraintSolver/btSliderConstraint.h
	src/bullet/BulletDynamics/ConstraintSolver/btSolverBody.h
	src/bullet/BulletDynamics/ConstraintSolver/btConstraintSolver.h
	src/bullet/BulletDynamics/Character/btKinematicCharacterController.h
	src/bullet/BulletDynamics/Character/btCharacterControllerInterface.h
	src/bullet/BulletDynamics/Dynamics/btSimpleDynamicsWorld.h
	src/bullet/BulletDynamics/Dynamics/btRigidBody.h
	src/bullet/BulletDynamics/Dynamics/btActionInterface.h
	src/bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h
	src/bullet/BulletDynamics/Dynamics/btDynamicsWorld.h
	src/bullet/BulletCollision/Gimpact/gim_geometry.h
	src/bullet/BulletCollision/Gimpact/btGImpactShape.h
	src/bullet/BulletCollision/Gimpact/btClipPolygon.h
	src/bullet/BulletCollision/Gimpact/gim_radixsort.h
	src/bullet/BulletCollision/Gimpact/gim_array.h
	src/bullet/BulletCollision/Gimpact/btTriangleShapeEx.h
	src/bullet/BulletCollision/Gimpact/btGImpactQuantizedBvh.h
	src/bullet/BulletCollision/Gimpact/gim_geom_types.h
	src/bullet/BulletCollision/Gimpact/gim_clip_polygon.h
	src/bullet/BulletCollision/Gimpact/gim_math.h
	src/bullet/BulletCollision/Gimpact/btGenericPoolAllocator.h
	src/bullet/BulletCollision/Gimpact/btBoxCollision.h
	src/bullet/BulletCollision/Gimpact/gim_box_collision.h
	src/bullet/BulletCollision/Gimpact/btGImpactMassUtil.h
	src/bullet/BulletCollision/Gimpact/btQuantization.h
	src/bullet/BulletCollision/Gimpact/btGImpactBvh.h
	src/bullet/BulletCollision/Gimpact/gim_tri_collision.h
	src/bullet/BulletCollision/Gimpact/btGeometryOperations.h
	src/bullet/BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h
	src/bullet/BulletCollision/Gimpact/gim_contact.h
	src/bullet/BulletCollision/Gimpact/gim_hash_table.h
	src/bullet/BulletCollision/Gimpact/gim_bitset.h
	src/bullet/BulletCollision/Gimpact/gim_memory.h
	src/bullet/BulletCollision/Gimpact/gim_linear_math.h
	src/bullet/BulletCollision/Gimpact/gim_box_set.h
	src/bullet/BulletCollision/Gimpact/gim_basic_geometry_operations.h
	src/bullet/BulletCollision/Gimpact/btContactProcessing.h
	src/bullet/BulletCollision/CollisionShapes/btTetrahedronShape.h
	src/bullet/BulletCollision/CollisionShapes/btMaterial.h
	src/bullet/BulletCollision/CollisionShapes/btBox2dShape.h
	src/bullet/BulletCollision/CollisionShapes/btStaticPlaneShape.h
	src/bullet/BulletCollision/CollisionShapes/btTriangleShape.h
	src/bullet/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h
	src/bullet/BulletCollision/CollisionShapes/btConvexShape.h
	src/bullet/BulletCollision/CollisionShapes/btMinkowskiSumShape.h
	src/bullet/BulletCollision/CollisionShapes/btConeShape.h
	src/bullet/BulletCollision/CollisionShapes/btCylinderShape.h
	src/bullet/BulletCollision/CollisionShapes/btTriangleCallback.h
	src/bullet/BulletCollision/CollisionShapes/btOptimizedBvh.h
	src/bullet/BulletCollision/CollisionShapes/btTriangleMesh.h
	src/bullet/BulletCollision/CollisionShapes/btBoxShape.h
	src/bullet/BulletCollision/CollisionShapes/btCollisionShape.h
	src/bullet/BulletCollision/CollisionShapes/btCapsuleShape.h
	src/bullet/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h
	src/bullet/BulletCollision/CollisionShapes/btCollisionMargin.h
	src/bullet/BulletCollision/CollisionShapes/btConvex2dShape.h
	src/bullet/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h
	src/bullet/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h
	src/bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h
	src/bullet/BulletCollision/CollisionShapes/btConvexHullShape.h
	src/bullet/BulletCollision/CollisionShapes/btCompoundShape.h
	src/bullet/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.h
	src/bullet/BulletCollision/CollisionShapes/btShapeHull.h
	src/bullet/BulletCollision/CollisionShapes/btConcaveShape.h
	src/bullet/BulletCollision/CollisionShapes/btConvexPointCloudShape.h
	src/bullet/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h
	src/bullet/BulletCollision/CollisionShapes/btTriangleInfoMap.h
	src/bullet/BulletCollision/CollisionShapes/btStridingMeshInterface.h
	src/bullet/BulletCollision/CollisionShapes/btTriangleBuffer.h
	src/bullet/BulletCollision/CollisionShapes/btConvexInternalShape.h
	src/bullet/BulletCollision/CollisionShapes/btPolyhedralConvexShape.h
	src/bullet/BulletCollision/CollisionShapes/btMultiSphereShape.h
	src/bullet/BulletCollision/CollisionShapes/btConvexPolyhedron.h
	src/bullet/BulletCollision/CollisionShapes/btTriangleMeshShape.h
	src/bullet/BulletCollision/CollisionShapes/btEmptyShape.h
	src/bullet/BulletCollision/CollisionShapes/btUniformScalingShape.h
	src/bullet/BulletCollision/CollisionShapes/btSphereShape.h
	src/bullet/BulletCollision/BroadphaseCollision/btDbvt.h
	src/bullet/BulletCollision/BroadphaseCollision/btDispatcher.h
	src/bullet/BulletCollision/BroadphaseCollision/btOverlappingPairCache.h
	src/bullet/BulletCollision/BroadphaseCollision/btQuantizedBvh.h
	src/bullet/BulletCollision/BroadphaseCollision/btMultiSapBroadphase.h
	src/bullet/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h
	src/bullet/BulletCollision/BroadphaseCollision/btDbvtBroadphase.h
	src/bullet/BulletCollision/BroadphaseCollision/btBroadphaseInterface.h
	src/bullet/BulletCollision/BroadphaseCollision/btAxisSweep3.h
	src/bullet/BulletCollision/BroadphaseCollision/btOverlappingPairCallback.h
	src/bullet/BulletCollision/BroadphaseCollision/btSimpleBroadphase.h
	src/bullet/BulletCollision/BroadphaseCollision/btBroadphaseProxy.h
	src/bullet/BulletCollision/CollisionDispatch/btCollisionConfiguration.h
	src/bullet/BulletCollision/CollisionDispatch/btCollisionObject.h
	src/bullet/BulletCollision/CollisionDispatch/btManifoldResult.h
	src/bullet/BulletCollision/CollisionDispatch/btCollisionWorld.h
	src/bullet/BulletCollision/CollisionDispatch/btCollisionDispatcher.h
	src/bullet/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h
	src/bullet/BulletCollision/CollisionDispatch/btCollisionCreateFunc.h
	src/bullet/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h
	src/bullet/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btBoxBoxDetector.h
	src/bullet/BulletCollision/CollisionDispatch/SphereTriangleDetector.h
	src/bullet/BulletCollision/CollisionDispatch/btUnionFind.h
	src/bullet/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btGhostObject.h
	src/bullet/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btSimulationIslandManager.h
	src/bullet/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h
	src/bullet/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btRaycastCallback.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btPointCollector.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btDiscreteCollisionDetectorInterface.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btConvexPenetrationDepthSolver.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btGjkEpa2.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btConvexCast.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btSimplexSolverInterface.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btManifoldPoint.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btPersistentManifold.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h
	src/bullet/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h
	src/bullet/LinearMath/btTransform.h
	src/bullet/LinearMath/btAabbUtil2.h
	src/bullet/LinearMath/btGeometryUtil.h
	src/bullet/LinearMath/btList.h
	src/bullet/LinearMath/btQuadWord.h
	src/bullet/LinearMath/btRandom.h
	src/bullet/LinearMath/btQuickprof.h
	src/bullet/LinearMath/btConvexHull.h
	src/bullet/LinearMath/btQuaternion.h
	src/bullet/LinearMath/btPoolAllocator.h
	src/bullet/LinearMath/btMatrix3x3.h
	src/bullet/LinearMath/btIDebugDraw.h
	src/bullet/LinearMath/btGrahamScan2dConvexHull.h
	src/bullet/LinearMath/btDefaultMotionState.h
	src/bullet/LinearMath/btConvexHullComputer.h
	src/bullet/LinearMath/btMinMax.h
	src/bullet/LinearMath/btHashMap.h
	src/bullet/LinearMath/btStackAlloc.h
	src/bullet/LinearMath/btMotionState.h
	src/bullet/LinearMath/btAlignedAllocator.h
	src/bullet/LinearMath/btSerializer.h
	src/bullet/LinearMath/btTransformUtil.h
	src/bullet/LinearMath/btAlignedObjectArray.h
	src/bullet/LinearMath/btVector3.h
	src/bullet/LinearMath/btScalar.h
	src/bullet/btBulletDynamicsCommon.h
)

set(SRCS
	src/CharacterController.cpp
	src/Game.cpp
	src/AppStateManager.cpp
	src/RigidBody.cpp
	src/CharacterAnimation.cpp
	src/environment.cpp
	src/Game_setup.cpp
	src/main.cpp
	src/MainMenu.cpp

	src/btOgre/BtOgre.cpp

	src/bullet/BulletDynamics/Vehicle/btWheelInfo.cpp
	src/bullet/BulletDynamics/Vehicle/btRaycastVehicle.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btPoint2PointConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btSliderConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btConeTwistConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btHinge2Constraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btUniversalConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btHingeConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btContactConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btTypedConstraint.cpp
	src/bullet/BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.cpp
	src/bullet/BulletDynamics/Character/btKinematicCharacterController.cpp
	src/bullet/BulletDynamics/Dynamics/btSimpleDynamicsWorld.cpp
	src/bullet/BulletDynamics/Dynamics/btRigidBody.cpp
	src/bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.cpp
	src/bullet/BulletCollision/Gimpact/btGImpactShape.cpp
	src/bullet/BulletCollision/Gimpact/btTriangleShapeEx.cpp
	src/bullet/BulletCollision/Gimpact/gim_box_set.cpp
	src/bullet/BulletCollision/Gimpact/btGImpactBvh.cpp
	src/bullet/BulletCollision/Gimpact/btGenericPoolAllocator.cpp
	src/bullet/BulletCollision/Gimpact/btGImpactCollisionAlgorithm.cpp
	src/bullet/BulletCollision/Gimpact/gim_contact.cpp
	src/bullet/BulletCollision/Gimpact/btContactProcessing.cpp
	src/bullet/BulletCollision/Gimpact/gim_tri_collision.cpp
	src/bullet/BulletCollision/Gimpact/gim_memory.cpp
	src/bullet/BulletCollision/Gimpact/btGImpactQuantizedBvh.cpp
	src/bullet/BulletCollision/CollisionShapes/btOptimizedBvh.cpp
	src/bullet/BulletCollision/CollisionShapes/btTriangleMesh.cpp
	src/bullet/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btUniformScalingShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btConvexInternalShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btConvexPolyhedron.cpp
	src/bullet/BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btConeShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btBoxShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btCylinderShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btCompoundShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btShapeHull.cpp
	src/bullet/BulletCollision/CollisionShapes/btConvexHullShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btTetrahedronShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btSphereShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btTriangleMeshShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btBox2dShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp
	src/bullet/BulletCollision/CollisionShapes/btEmptyShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btConcaveShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btMultiSphereShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btStaticPlaneShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btConvex2dShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btTriangleBuffer.cpp
	src/bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btTriangleCallback.cpp
	src/bullet/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp
	src/bullet/BulletCollision/CollisionShapes/btConvexShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btStridingMeshInterface.cpp
	src/bullet/BulletCollision/CollisionShapes/btCollisionShape.cpp
	src/bullet/BulletCollision/CollisionShapes/btCapsuleShape.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btDbvt.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btMultiSapBroadphase.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btDispatcher.cpp
	src/bullet/BulletCollision/BroadphaseCollision/btAxisSweep3.cpp
	src/bullet/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp
	src/bullet/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp
	src/bullet/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp
	src/bullet/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp
	src/bullet/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btCollisionWorld.cpp
	src/bullet/BulletCollision/CollisionDispatch/btUnionFind.cpp
	src/bullet/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp
	src/bullet/BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp
	src/bullet/BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp
	src/bullet/BulletCollision/CollisionDispatch/btCollisionObject.cpp
	src/bullet/BulletCollision/CollisionDispatch/btGhostObject.cpp
	src/bullet/BulletCollision/CollisionDispatch/btManifoldResult.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btConvexCast.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp
	src/bullet/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp
	src/bullet/LinearMath/btQuickprof.cpp
	src/bullet/LinearMath/btSerializer.cpp
	src/bullet/LinearMath/btConvexHullComputer.cpp
	src/bullet/LinearMath/btAlignedAllocator.cpp
	src/bullet/LinearMath/btGeometryUtil.cpp
	src/bullet/LinearMath/btConvexHull.cpp
)

include_directories( ${OIS_INCLUDE_DIRS}
	${OGRE_INCLUDE_DIRS}
	${CEGUI-OGRE_INCLUDE_DIRS}
	"src/bullet"
	"src/btOgre"
)

add_executable(poniesmustdie WIN32 ${HDRS} ${SRCS})

add_subdirectory(blender)
add_dependencies(poniesmustdie generate_models)

target_link_libraries(poniesmustdie ${OGRE_LIBRARIES} ${OIS_LIBRARIES} boost_filesystem boost_system)

if(CMAKE_COMPILER_IS_GNUCC)
	add_definitions(-Wall)
endif(CMAKE_COMPILER_IS_GNUCC)


if(WIN32)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_definitions(-DPATH_RenderSystem_GL=\"RenderSystem_GL_d.dll\")
		add_definitions(-DPATH_Plugin_OctreeSceneManager=\"Plugin_OctreeSceneManager_d.dll\")
	elseif (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
		add_definitions(-DPATH_RenderSystem_GL=\"RenderSystem_GL.dll\")
		add_definitions(-DPATH_Plugin_OctreeSceneManager=\"Plugin_OctreeSceneManager.dll\")
	endif()
else()
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		get_filename_component(OGRE_PLUGINS_DIR ${OGRE_RenderSystem_GL_LIBRARY_DBG} PATH)
		add_definitions(-DOGRE_PLUGINS_DIR=\"${OGRE_PLUGINS_DIR}\")
	elseif (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
		get_filename_component(OGRE_PLUGINS_DIR ${OGRE_RenderSystem_GL_LIBRARY_REL} PATH)
		add_definitions(-DOGRE_PLUGINS_DIR=\"${OGRE_PLUGINS_DIR}\")
	endif()
endif()

add_definitions(-DPATH_RESOURCES=\"${CMAKE_INSTALL_PREFIX}/share/pmd\")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dist/bin)

# post-build copy for win32
if(WIN32 AND NOT MINGW)
	add_custom_command( TARGET poniesmustdie PRE_BUILD
		COMMAND if not exist .\\dist\\bin mkdir .\\dist\\bin )
	add_custom_command( TARGET poniesmustdie POST_BUILD
		COMMAND copy \"$(TargetPath)\" .\\dist\\bin )
endif(WIN32 AND NOT MINGW)

if(MINGW OR UNIX)
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/dist/bin)
endif(MINGW OR UNIX)


install(TARGETS poniesmustdie
	RUNTIME DESTINATION bin
	CONFIGURATIONS All)

install(FILES resources/models/Sinbad.zip
		resources/models/Examples.material
		resources/models/r2skin.jpg
		resources/models/rockwall.tga
	DESTINATION share/pmd/models)

install(
	DIRECTORY resources/levels
	DESTINATION share/pmd)

install(FILES resources/gui/DejaVuSans-10.font
		resources/gui/DejaVuSans.ttf
		resources/gui/MainMenu.layout
		resources/gui/TaharezLook.imageset
		resources/gui/TaharezLook.looknfeel
		resources/gui/TaharezLook.scheme
		resources/gui/TaharezLook.tga
		resources/gui/TaharezLookWidgetAliases.scheme
		resources/gui/TaharezLookWidgets.scheme
	DESTINATION share/pmd/gui)

if(WIN32)
	# NOTE: for the 1.7.1 sdk the OIS dll is called OIS.dll instead of libOIS.dll
	# so you'll have to change that to make it work with 1.7.1
	install(FILES ${OGRE_PLUGIN_DIR_REL}/OgreMain.dll
		${OGRE_PLUGIN_DIR_REL}/RenderSystem_GL.dll
		${OGRE_PLUGIN_DIR_REL}/Plugin_OctreeSceneManager.dll
		${OGRE_PLUGIN_DIR_REL}/OIS.dll
		DESTINATION bin
		CONFIGURATIONS Release RelWithDebInfo
	)

	install(FILES ${OGRE_PLUGIN_DIR_DBG}/OgreMain_d.dll
		${OGRE_PLUGIN_DIR_DBG}/RenderSystem_GL_d.dll
		${OGRE_PLUGIN_DIR_REL}/Plugin_OctreeSceneManager_d.dll
		${OGRE_PLUGIN_DIR_DBG}/OIS_d.dll
		DESTINATION bin
		CONFIGURATIONS Debug
	)

   # as of sdk 1.7.2 we need to copy the boost dll's as well
   # because they're not linked statically (it worked with 1.7.1 though)
   install(FILES ${Boost_DATE_TIME_LIBRARY_RELEASE}
      ${Boost_THREAD_LIBRARY_RELEASE}
      DESTINATION bin
      CONFIGURATIONS Release RelWithDebInfo
   )

   install(FILES ${Boost_DATE_TIME_LIBRARY_DEBUG}
      ${Boost_THREAD_LIBRARY_DEBUG}
      DESTINATION bin
      CONFIGURATIONS Debug
   )
endif(WIN32)

if(UNIX)

endif(UNIX)

