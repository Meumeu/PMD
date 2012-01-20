.PHONY: all clean install
.SILENT:

OGRE_PLUGINS_DIR=`pkg-config OGRE --variable=plugindir`

OGRE_RenderSystem_GL_LIBRARY_DBG = $(OGRE_PLUGINS_DIR)/RenderSystem_GL.so
OGRE_RenderSystem_GL_LIBRARY_REL = $(OGRE_PLUGINS_DIR)/RenderSystem_GL.so
OGRE_Plugin_OctreeSceneManager_LIBRARY_DBG = $(OGRE_PLUGINS_DIR)/Plugin_OctreeSceneManager.so
OGRE_Plugin_OctreeSceneManager_LIBRARY_REL = $(OGRE_PLUGINS_DIR)/Plugin_OctreeSceneManager.so

CXXFLAGS = `pkg-config --cflags OGRE OIS` -I$(SRCDIR)/bullet -Wall
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
CP=cp
#RM=rm
ZIP=zip -q

CXXFLAGS += -DPATH_RESOURCES=\"$(PREFIX)/share/pmd\"

BLENDERDIR=blender
SRCDIR=src
OBJDIR=obj

SRC = $(shell find $(SRCDIR)/ -name *.cpp)
BLENDERSRC = $(shell find $(BLENDERDIR) -name *.blend)
BLENDERZIP = $(patsubst $(BLENDERDIR)/%.blend,dist/share/pmd/models/%.zip,$(BLENDERSRC))

ifeq ($(DEBUG),y)
	OBJS=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%-dbg.o,$(SRC)))
else
	OBJS=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(SRC)))
endif

all: dist/bin/poniesmustdie $(BLENDERZIP)

clean:
	-$(RM) -r $(OBJDIR) dist

install: dist/bin/poniesmustdie $(BLENDERZIP)
	echo Installing to $(PREFIX)
	$(MKDIR) -p $(PREFIX)/bin $(PREFIX)/share/pmd/models
	$(CP) dist/bin/poniesmustdie $(PREFIX)/bin/poniesmustdie
	$(CP) $(BLENDERZIP) $(PREFIX)/share/pmd/models
	$(CP) resources/meshes/Sinbad.zip resources/textures/Examples.material resources/textures/rockwall.tga $(PREFIX)/share/pmd/models

dist/bin/poniesmustdie: $(OBJS)
	echo Linking $(notdir $@)
	$(MKDIR) -p dist/bin
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$<))
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_REL) -c -MMD $< -MT $@ -MF $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.d,$<)) -o $@

$(OBJDIR)/%-dbg.o: $(SRCDIR)/%.cpp
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$<))
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_DBG) -c -MMD $< -MT $@ -MF $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.d,$<)) -o $@

dist/share/pmd/models/%.zip: $(BLENDERDIR)/%.blend
	echo Generating $(notdir $@)
	$(MKDIR) -p $(patsubst dist/share/pmd/%.zip,obj/%-out,$@)
	$(MKDIR) -p $(dir $@)
	$(RM) -r $(patsubst dist/share/pmd/%.zip,obj/%-out,$@)/*
	$(BLENDER) -b $< -P tools/io_export_ogreDotScene.py -P tools/export.py -- $(patsubst dist/share/pmd/%.zip,obj/%-out/,$@) > $(patsubst dist/share/pmd/%.zip,obj/%.log,$@) 2>&1
	$(RM) $@
	$(ZIP) -j $@ $(patsubst dist/share/pmd/%.zip,obj/%-out,$@)/*.mesh
	$(ZIP) -j $@ $(patsubst dist/share/pmd/%.zip,obj/%-out,$@)/*.skeleton
	$(ZIP) -j $@ $(patsubst dist/share/pmd/%.zip,obj/%-out,$@)/*.material

-include $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.d,$(SRC)))
