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

set(CMAKE_DEBUG_POSTFIX "_d")

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist")

find_package(OGRE REQUIRED)
find_package(OIS REQUIRED)

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
set(OGRE_LIBRARIES ${OGRE_LIBRARIES} ${Boost_LIBRARIES})

file(GLOB_RECURSE HDRS src/*.h)

file(GLOB_RECURSE SRCS src/*.cpp)

include_directories( ${OIS_INCLUDE_DIRS}
	${OGRE_INCLUDE_DIRS}
	"src/bullet"
)

add_executable(poniesmustdie WIN32 ${HDRS} ${SRCS})

add_subdirectory(blender)
add_dependencies(poniesmustdie generate_models)

set_target_properties(poniesmustdie PROPERTIES DEBUG_POSTFIX _d)

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
		add_definitions(-DPATH_RenderSystem_GL=\"${OGRE_RenderSystem_GL_LIBRARY_DBG}\")
		add_definitions(-DPATH_Plugin_OctreeSceneManager=\"${OGRE_Plugin_OctreeSceneManager_LIBRARY_DBG}\")
	elseif (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
		add_definitions(-DPATH_RenderSystem_GL=\"${OGRE_RenderSystem_GL_LIBRARY_REL}\")
		add_definitions(-DPATH_Plugin_OctreeSceneManager=\"${OGRE_Plugin_OctreeSceneManager_LIBRARY_REL}\")
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

configure_file(${CMAKE_SOURCE_DIR}/src/resources.cfg.in
	${CMAKE_INSTALL_PREFIX}/share/pmd/resources.cfg)

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

