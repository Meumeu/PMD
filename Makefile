.PHONY: all clean install
.SILENT:

OGRE_PLUGINS_DIR=`pkg-config OGRE --variable=plugindir`

CXXFLAGS = `pkg-config --cflags OGRE OIS CEGUI-OGRE | sed s/-I/-I/g` -I$(SRCDIR)/bullet -I$(SRCDIR)/Recast/Include -std=c++0x -march=corei7-avx
LDFLAGS = `pkg-config --libs OGRE OIS CEGUI-OGRE` -lboost_filesystem -lboost_system

CXXFLAGS += -DOGRE_PLUGINS_DIR=\"${OGRE_PLUGINS_DIR}\"

CXXFLAGS_REL = -O2 -DNDEBUG
CXXFLAGS_DBG = -g

ifeq (y,$(PHYSICS_DEBUG))
CXXFLAGS += -DPHYSICS_DEBUG
endif

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

OTHER_MODELS = resources/models/Sinbad.zip resources/models/Examples.material resources/models/rockwall.tga resources/models/Debug.material
GUI_FILES = resources/gui/DejaVuSans-10.font \
	resources/gui/DejaVuSans.ttf \
	resources/gui/MainMenu.layout \
	resources/gui/TaharezLook.imageset \
	resources/gui/TaharezLook.looknfeel \
	resources/gui/TaharezLook.scheme \
	resources/gui/TaharezLook.tga \
	resources/gui/TaharezLookWidgetAliases.scheme \
	resources/gui/TaharezLookWidgets.scheme

LEVEL_FILES = resources/levels/level1/level.txt

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
	$(MKDIR) -p $(PREFIX)/bin $(PREFIX)/share/pmd/models $(PREFIX)/share/pmd/gui $(PREFIX)/share/pmd/levels
	$(CP) dist/bin/poniesmustdie $(PREFIX)/bin/poniesmustdie
	$(CP) $(BLENDERZIP) $(PREFIX)/share/pmd/models
	$(CP) $(OTHER_MODELS) $(PREFIX)/share/pmd/models
	$(CP) $(GUI_FILES) $(PREFIX)/share/pmd/gui
	$(CP) -r resources/levels/* $(PREFIX)/share/pmd/levels

dist/bin/poniesmustdie: $(OBJS) Makefile
	echo Linking $(notdir $@)
	$(MKDIR) -p dist/bin
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR)/release/bullet/%.o: $(SRCDIR)/bullet/%.cpp Makefile
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_REL) -c -MMD $< -MT $@ -MF $(patsubst %.o,%.d,$@) -o $@ -fpermissive

$(OBJDIR)/release/%.o: $(SRCDIR)/%.cpp Makefile
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_REL) -Wall -Werror -c -MMD $< -MT $@ -MF $(patsubst %.o,%.d,$@) -o $@

$(OBJDIR)/debug/bullet/%.o: $(SRCDIR)/bullet/%.cpp Makefile
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_DBG) -c -MMD $< -MT $@ -MF $(patsubst %.o,%.d,$@) -o $@ -fpermissive

$(OBJDIR)/debug/%.o: $(SRCDIR)/%.cpp Makefile
	echo Building $(notdir $@)
	$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_DBG) -Wall -Werror -c -MMD $< -MT $@ -MF $(patsubst %.o,%.d,$@) -o $@

dist/share/pmd/models/%.zip: $(BLENDERDIR)/%.blend
	echo Generating $(notdir $@)
ifeq (,$(BLENDER))
	echo Blender not found, set the BLENDER variable
	exit 1
else
	$(MKDIR) -p $(dir $@) $(dir $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%.log,$@))
	$(BLENDER) -b $< -P tools/io_export_ogreDotScene.py -P tools/export.py -- $@ > $(patsubst dist/share/pmd/%.zip,$(OBJDIR)/%.log,$@) 2>&1
endif

-include $(patsubst %.o,%.d,$(OBJS))
