.PHONY: all clean install
.SILENT:

OGRE_PLUGINS_DIR=`pkg-config OGRE --variable=plugindir`

OGRE_RenderSystem_GL_LIBRARY_DBG = $(OGRE_PLUGINS_DIR)/RenderSystem_GL.so
OGRE_RenderSystem_GL_LIBRARY_REL = $(OGRE_PLUGINS_DIR)/RenderSystem_GL.so
OGRE_Plugin_OctreeSceneManager_LIBRARY_DBG = $(OGRE_PLUGINS_DIR)/Plugin_OctreeSceneManager.so
OGRE_Plugin_OctreeSceneManager_LIBRARY_REL = $(OGRE_PLUGINS_DIR)/Plugin_OctreeSceneManager.so

CXXFLAGS = `pkg-config --cflags OGRE OIS` -Isrc/bullet -Wall
LDFLAGS = `pkg-config --libs OGRE OIS` -lboost_filesystem -lboost_system

CXXFLAGS += -DPATH_RenderSystem_GL=\"${OGRE_RenderSystem_GL_LIBRARY_DBG}\"

CXXFLAGS_REL = -O3 -DNDEBUG
CXXFLAGS_REL += -DPATH_RenderSystem_GL=\"$(OGRE_RenderSystem_GL_LIBRARY_REL)\"
CXXFLAGS_REL += -DPATH_Plugin_OctreeSceneManager=\"$(OGRE_Plugin_OctreeSceneManager_LIBRARY_REL)\"

CXXFLAGS_DBG = -g
CXXFLAGS_DBG += -DPATH_RenderSystem_GL=\"$(OGRE_RenderSystem_GL_LIBRARY_DBG)\"
CXXFLAGS_DBG += -DPATH_Plugin_OctreeSceneManager=\"$(OGRE_Plugin_OctreeSceneManager_LIBRARY_DBG)\"

BLENDER = blender
MKDIR=mkdir
#CP=cp
#RM=rm

#PREFIX ?= $(CURDIR)/dist
CXXFLAGS += -DPATH_RESOURCES=\"$(PREFIX)/share/pmd\"

SRC = $(shell find src/ -name *.cpp)

ifeq ($(DEBUG),y)
	OBJS=$(patsubst src/%,obj/%,$(patsubst %.cpp,%-dbg.o,$(SRC)))
else
	OBJS=$(patsubst src/%,obj/%,$(patsubst %.cpp,%.o,$(SRC)))
endif

all: dist/bin/poniesmustdie dist/share/pmd/models/Pony.mesh dist/share/pmd/models/Pony.skeleton dist/share/pmd/models/PonySkin.material dist/share/pmd/models/PonyEye.material

clean:
	-$(RM) -r obj dist

install: dist/bin/poniesmustdie dist/share/pmd/models/Pony.mesh dist/share/pmd/models/Pony.skeleton dist/share/pmd/models/PonySkin.material dist/share/pmd/models/PonyEye.material
	echo Installing to $(PREFIX)
	$(MKDIR) -p $(PREFIX)/bin $(PREFIX)/share/pmd/models
	$(CP) dist/bin/poniesmustdie $(PREFIX)/bin/poniesmustdie
	$(CP) obj/models/Pony.mesh obj/models/Pony.skeleton obj/models/PonySkin.material obj/models/PonyEye.material $(PREFIX)/share/pmd/models
	sed -e s,@CMAKE_SOURCE_DIR@,$(CURDIR), -e s,@CMAKE_INSTALL_PREFIX@,$(PREFIX), src/resources.cfg.in > $(PREFIX)/share/pmd/resources.cfg

dist/share/pmd/models/Pony.mesh dist/share/pmd/models/Pony.skeleton dist/share/pmd/models/PonySkin.material dist/share/pmd/models/PonyEye.material: blender/poney.blend
	echo Generating meshes...
	$(MKDIR) -p dist/share/pmd/models
	$(BLENDER) -b $< -P tools/io_export_ogreDotScene.py -P tools/export.py -- obj/models/Pony > obj/models/pony.log 2>&1

dist/bin/poniesmustdie: $(OBJS)
	echo Linking $(notdir $@)
	$(MKDIR) -p dist/bin
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

obj/%.o: src/%.cpp
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $(patsubst src/%,obj/%,$<))
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_REL) -c -MMD $< -MT $@ -MF $(patsubst src/%,obj/%,$(patsubst %.cpp,%.d,$<)) -o $@

obj/%-dbg.o: src/%.cpp
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $(patsubst src/%,obj/%,$<))
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_DBG) -c -MMD $< -MT $@ -MF $(patsubst src/%,obj/%,$(patsubst %.cpp,%.d,$<)) -o $@


-include $(patsubst src/%,obj/%,$(patsubst %.cpp,%.d,$(SRC)))
