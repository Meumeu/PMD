.PHONY: all clean install
.SILENT:

OGRE_PLUGINS_DIR=`pkg-config OGRE --variable=plugindir`

OGRE_RenderSystem_GL_LIBRARY_DBG = $(OGRE_PLUGINS_DIR)/RenderSystem_GL.so
OGRE_RenderSystem_GL_LIBRARY_REL = $(OGRE_PLUGINS_DIR)/RenderSystem_GL.so
OGRE_Plugin_OctreeSceneManager_LIBRARY_DBG = $(OGRE_PLUGINS_DIR)/Plugin_OctreeSceneManager.so
OGRE_Plugin_OctreeSceneManager_LIBRARY_REL = $(OGRE_PLUGINS_DIR)/Plugin_OctreeSceneManager.so

CXXFLAGS = `pkg-config --cflags OGRE OIS CEGUI-OGRE` -I$(SRCDIR)/bullet
LDFLAGS = `pkg-config --libs OGRE OIS CEGUI-OGRE` -lboost_filesystem -lboost_system

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
MV=mv

PREFIX=/usr/local
CXXFLAGS += -DPATH_RESOURCES=\"$(PREFIX)/share/pmd\"

ifeq ($(wildcard /usr/bin/blender),/usr/bin/blender)
BLENDER=/usr/bin/blender
else ifeq ($(wildcard /usr/bin/blender-2.60),/usr/bin/blender-2.60)
BLENDER=/usr/bin/blender-2.60
endif

BLENDERDIR=blender
SRCDIR=src
OBJDIR=objects

SRC = $(shell find $(SRCDIR)/ -name *.cpp)
BLENDERSRC = $(shell find $(BLENDERDIR) -name *.blend)
BLENDERZIP = $(patsubst $(BLENDERDIR)/%.blend,dist/share/pmd/models/%.zip,$(BLENDERSRC))

OTHER_MODELS = resources/meshes/Sinbad.zip resources/textures/Examples.material resources/textures/rockwall.tga

ifeq ($(DEBUG),y)
	OBJS=$(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/debug/%.o,$(SRC))
else
	OBJS=$(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/release/%.o,$(SRC))
endif

all: dist/bin/poniesmustdie $(BLENDERZIP)

clean:
	-$(RM) -r $(OBJDIR) dist

install: dist/bin/poniesmustdie $(BLENDERZIP)
	echo Installing to $(PREFIX)
	$(MKDIR) -p $(PREFIX)/bin $(PREFIX)/share/pmd/models
	$(CP) dist/bin/poniesmustdie $(PREFIX)/bin/poniesmustdie
	$(CP) $(BLENDERZIP) $(PREFIX)/share/pmd/models
	$(CP) $(OTHER_MODELS) $(PREFIX)/share/pmd/models

dist/bin/poniesmustdie: $(OBJS)
	echo Linking $(notdir $@)
	$(MKDIR) -p dist/bin
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR)/release/bullet/%.o: $(SRCDIR)/bullet/%.cpp
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_REL) -c -MMD $< -MT $@ -MF $(patsubst %.o,%.d,$@) -o $@

$(OBJDIR)/release/%.o: $(SRCDIR)/%.cpp
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_REL) -Wall -Werror -c -MMD $< -MT $@ -MF $(patsubst %.o,%.d,$@) -o $@

$(OBJDIR)/debug/bullet/%.o: $(SRCDIR)/bullet/%.cpp
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_DBG) -c -MMD $< -MT $@ -MF $(patsubst %.o,%.d,$@) -o $@

$(OBJDIR)/debug/%.o: $(SRCDIR)/%.cpp
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_DBG) -Wall -Werror -c -MMD $< -MT $@ -MF $(patsubst %.o,%.d,$@) -o $@

dist/share/pmd/models/%.zip: $(BLENDERDIR)/%.blend
	echo Generating $(notdir $@)
ifeq (,$(BLENDER))
	echo Blender not found, set the BLENDER variable
	exit 1
else
	$(MKDIR) -p $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%-out,$@)
	$(MKDIR) -p $(dir $@)
	$(RM) -r $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%-out,$@)/*
	$(BLENDER) -b $< -P tools/io_export_ogreDotScene.py -P tools/export.py -- $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%-out/,$@) > $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%.log,$@) 2>&1
	$(MV) $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%-out,$@)/.material $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%-out,$@)/$(patsubst %.zip,%,$(notdir $@)).material
	-$(RM) $(patsubst %.zip,%-tmp.zip,$@)
	$(ZIP) -j $(patsubst %.zip,%-tmp.zip,$@) $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%-out,$@)/*.mesh
	$(ZIP) -j $(patsubst %.zip,%-tmp.zip,$@) $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%-out,$@)/*.skeleton
	$(ZIP) -j $(patsubst %.zip,%-tmp.zip,$@) $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%-out,$@)/*.material
	$(MV) $(patsubst %.zip,%-tmp.zip,$@) $@
endif

-include $(patsubst %.o,%.d,$(OBJS))
