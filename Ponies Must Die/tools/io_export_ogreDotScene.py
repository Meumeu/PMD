# Copyright (C) 2010 Brett Hartshorn
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

bl_info = {
    "name": "OGRE Exporter (.scene, .mesh, .skeleton) and RealXtend (.txml)",
    "author": "HartsAntler, Sebastien Rombauts, and F00bar",
    "version": (0,5,5),
    "blender": (2,6,0),
    "location": "File > Export...",
    "description": "Export to Ogre xml and binary formats",
    "wiki_url": "http://code.google.com/p/blender2ogre/w/list",
    "tracker_url": "http://code.google.com/p/blender2ogre/issues/list",
    "category": "Import-Export"}

VERSION = '0.5.5'

## final TODO: 
## fix terrain collision offset bug
## add realtime transform (rotation is missing)
## fix camera rotated -90 ogre-dot-scene


## Public API ##
UI_CLASSES = []
def UI(cls): # @decorator
    ''' these classes will be toggled on and off by the user - interface toggle '''
    if cls not in UI_CLASSES:
        UI_CLASSES.append(cls)
    return cls

def hide_user_interface():
    for cls in UI_CLASSES:
        bpy.utils.unregister_class( cls )

def uid(ob):
    if ob.uid == 0:
        high = 0
        for o in bpy.data.objects:
            if o.uid > high: high = o.uid
        high += 1
        if high < 100: high = 100   # start at 100
        ob.uid = high
    return ob.uid

## END Public API ## -- (go to bottom for rest of API)
###########################################################
###########################################################
###### imports #####
import os, sys, time, hashlib, getpass, tempfile, configparser
import math, subprocess, pickle
import array, time, ctypes
from xml.sax.saxutils import XMLGenerator

try:
    import bpy, mathutils
    from bpy.props import *
except ImportError:
    sys.exit("This script is an addon for blender, you must install it from blender.")

## make sure we can import from same directory ##
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path: sys.path.append( SCRIPT_DIR )


######################### bpy RNA #########################
bpy.types.Object.uid = IntProperty(
    name="unique ID", description="unique ID for Tundra", 
    default=0, min=0, max=2**14)

# Ogre supports .dds in both directx and opengl
# http://www.ogre3d.org/forums/viewtopic.php?f=5&t=46847
_IMAGE_FORMATS =  [                                 # for EnumProperty "FORCE_IMAGE_FORMAT"
    ('NONE','NONE', 'do not convert image'),
    ('bmp', 'bmp', 'bitmap format'),
    ('jpg', 'jpg', 'jpeg format'),
    ('gif', 'gif', 'gif format'),
    ('png', 'png', 'png format'),
    ('tga', 'tga', 'targa format'),
    ('dds', 'dds', 'nvidia dds format'),
]

bpy.types.Image.use_convert_format = BoolProperty( name='use convert format', default=False )
bpy.types.Image.convert_format = EnumProperty( items=_IMAGE_FORMATS, name='convert to format',  description='converts to image format using imagemagick', default='NONE' )
bpy.types.Image.jpeg_quality = IntProperty(
    name="jpeg quality", description="quality of jpeg", 
    default=80, min=0, max=100)

bpy.types.Image.use_color_quantize = BoolProperty( name='use color quantize', default=False )
bpy.types.Image.use_color_quantize_dither = BoolProperty( name='use color quantize dither', default=True )
bpy.types.Image.color_quantize = IntProperty(
    name="color quantize", description="reduce to N colors (requires ImageMagick)", 
    default=32, min=2, max=256)

bpy.types.Image.use_resize_half = BoolProperty( name='resize by 1/2', default=False )
bpy.types.Image.use_resize_absolute = BoolProperty( name='force image resize', default=False )
bpy.types.Image.resize_x = IntProperty(
    name="resize X", description="only if image is larger than defined, use ImageMagick to resize it down", 
    default=256, min=2, max=4096)
bpy.types.Image.resize_y = IntProperty(
    name="resize Y", description="only if image is larger than defined, use ImageMagick to resize it down", 
    default=256, min=2, max=4096)

## use Material.offset_z ## bpy.types.Material.ogre_depth_bias = IntProperty

# ..Material.ogre_depth_write = AUTO|ON|OFF
bpy.types.Material.ogre_depth_write = BoolProperty( name='depth write', default=True )

#If depth-buffer checking is on, whenever a pixel is about to be written to the frame buffer the depth buffer is checked to see if the pixel is in front of all other pixels written at that point. If not, the pixel is not written. If depth checking is off, pixels are written no matter what has been rendered before.
bpy.types.Material.ogre_depth_check = BoolProperty( name='depth check', default=True )

#Sets whether this pass will use 'alpha to coverage', a way to multisample alpha texture edges so they blend more seamlessly with the background. This facility is typically only available on cards from around 2006 onwards, but it is safe to enable it anyway - Ogre will just ignore it if the hardware does not support it. The common use for alpha to coverage is foliage rendering and chain-link fence style textures.
bpy.types.Material.ogre_alpha_to_coverage = BoolProperty( name='multisample alpha edges', default=False )

#This option is usually only useful if this pass is an additive lighting pass, and is at least the second one in the technique. Ie areas which are not affected by the current light(s) will never need to be rendered. If there is more than one light being passed to the pass, then the scissor is defined to be the rectangle which covers all lights in screen-space. Directional lights are ignored since they are infinite.
#This option does not need to be specified if you are using a standard additive shadow mode, i.e. SHADOWTYPE_STENCIL_ADDITIVE or SHADOWTYPE_TEXTURE_ADDITIVE, since it is the default behaviour to use a scissor for each additive shadow pass. However, if you're not using shadows, or you're using Integrated Texture Shadows where passes are specified in a custom manner, then this could be of use to you.
bpy.types.Material.ogre_light_scissor = BoolProperty( name='light scissor', default=False )

bpy.types.Material.ogre_light_clip_planes = BoolProperty( name='light clip planes', default=False )

# TODO set description for all pyRNA
bpy.types.Material.ogre_normalise_normals = BoolProperty( name='normalise normals', default=False,
    description='''
Scaling objects causes normals to also change magnitude, which can throw off your lighting calculations. By default, the SceneManager detects this and will automatically re-normalise normals for any scaled object, but this has a cost. If you'd prefer to control this manually, call SceneManager::setNormaliseNormalsOnScale(false) and then use this option on materials which are sensitive to normals being resized.
''')

#Sets whether or not dynamic lighting is turned on for this pass or not. If lighting is turned off, all objects rendered using the pass will be fully lit. This attribute has no effect if a vertex program is used.
bpy.types.Material.ogre_lighting = BoolProperty( name='dynamic lighting', default=True )

#If colour writing is off no visible pixels are written to the screen during this pass. You might think this is useless, but if you render with colour writing off, and with very minimal other settings, you can use this pass to initialise the depth buffer before subsequently rendering other passes which fill in the colour data. This can give you significant performance boosts on some newer cards, especially when using complex fragment programs, because if the depth check fails then the fragment program is never run. 
bpy.types.Material.ogre_colour_write = BoolProperty( name='color-write', default=True )

bpy.types.Material.use_fixed_pipeline = BoolProperty( name='fixed pipeline', default=True )    # fixed pipeline is oldschool

# hidden option - gets turned on by operator
bpy.types.Material.use_material_passes = BoolProperty( name='use ogre extra material passes (layers)', default=False )

bpy.types.Material.use_in_ogre_material_pass = BoolProperty( name='Layer Toggle', default=True )

bpy.types.Material.use_ogre_advanced_options = BoolProperty( name='Show Advanced Options', default=False )

bpy.types.Material.use_ogre_parent_material = BoolProperty( name='Use Script Inheritance', default=False )

bpy.types.Material.ogre_parent_material = EnumProperty(
  name="Script Inheritence", 
  description='ogre parent material class', #default='NONE',
  items=[],
)


bpy.types.Material.ogre_polygon_mode = EnumProperty(
    items=[
            ('solid', 'solid', 'SOLID'),
            ('wireframe', 'wireframe', 'WIREFRAME'),
            ('points', 'points', 'POINTS'),
    ],
    name='faces draw type', 
    description="ogre face draw mode", 
    default='solid'
)

bpy.types.Material.ogre_shading = EnumProperty(
    items=[
            ('flat', 'flat', 'FLAT'),
            ('gouraud', 'gouraud', 'GOURAUD'),
            ('phong', 'phong', 'PHONG'),
    ],
    name='hardware shading', 
    description="Sets the kind of shading which should be used for representing dynamic lighting for this pass.", 
    default='gouraud'
)


bpy.types.Material.ogre_cull_hardware = EnumProperty(
    items=[
            ('clockwise', 'clockwise', 'CLOCKWISE'),
            ('anticlockwise', 'anticlockwise', 'COUNTER CLOCKWISE'),
            ('none', 'none', 'NONE'),
    ],
    name='hardware culling', 
    description="If the option 'cull_hardware clockwise' is set, all triangles whose vertices are viewed in clockwise order from the camera will be culled by the hardware.", 
    default='clockwise'
)


bpy.types.Material.ogre_transparent_sorting = EnumProperty(
    items=[
            ('on', 'on', 'ON'),
            ('off', 'off', 'OFF'),
            ('force', 'force', 'FORCE ON'),
    ],
    name='transparent sorting', 
    description="By default all transparent materials are sorted such that renderables furthest away from the camera are rendered first. This is usually the desired behaviour but in certain cases this depth sorting may be unnecessary and undesirable. If for example it is necessary to ensure the rendering order does not change from one frame to the next. In this case you could set the value to 'off' to prevent sorting.", 
    default='on'
)

bpy.types.Material.ogre_illumination_stage = EnumProperty(
    items=[
            ('', '', 'autodetect'),
            ('ambient', 'ambient', 'ambient'),
            ('per_light', 'per_light', 'lights'),
            ('decal', 'decal', 'decal')
    ],
    name='illumination stage', 
    description='When using an additive lighting mode (SHADOWTYPE_STENCIL_ADDITIVE or SHADOWTYPE_TEXTURE_ADDITIVE), the scene is rendered in 3 discrete stages, ambient (or pre-lighting), per-light (once per light, with shadowing) and decal (or post-lighting). Usually OGRE figures out how to categorise your passes automatically, but there are some effects you cannot achieve without manually controlling the illumination.', 
    default=''
)

_ogre_depth_func =  [
    ('less_equal', 'less_equal', '<='),
    ('less', 'less', '<'),
    ('equal', 'equal', '=='),
    ('not_equal', 'not_equal', '!='),
    ('greater_equal', 'greater_equal', '>='),
    ('greater', 'greater', '>'),
    ('always_fail', 'always_fail', 'false'),
    ('always_pass', 'always_pass', 'true'),
]
bpy.types.Material.ogre_depth_func = EnumProperty(
    items=_ogre_depth_func, 
    name='depth buffer function', 
    description='If depth checking is enabled (see depth_check) a comparison occurs between the depth value of the pixel to be written and the current contents of the buffer. This comparison is normally less_equal, i.e. the pixel is written if it is closer (or at the same distance) than the current contents', 
    default='less_equal'
)



_ogre_scene_blend_ops =  [
    ('add', 'add', 'DEFAULT'),
    ('subtract', 'subtract', 'SUBTRACT'),
    ('reverse_subtract', 'reverse_subtract', 'REVERSE SUBTRACT'),
    ('min', 'min', 'MIN'),
    ('max', 'max', 'MAX'),

]
bpy.types.Material.ogre_scene_blend_op = EnumProperty(
    items=_ogre_scene_blend_ops, 
    name='scene blending operation', 
    description='This directive changes the operation which is applied between the two components of the scene blending equation', 
    default='add'
)

_ogre_scene_blend_types =  [
    ('one zero', 'one zero', 'DEFAULT'),
    ('alpha_blend', 'alpha_blend', "The alpha value of the rendering output is used as a mask. Equivalent to 'scene_blend src_alpha one_minus_src_alpha'"),
    ('add', 'add', "The colour of the rendering output is added to the scene. Good for explosions, flares, lights, ghosts etc. Equivalent to 'scene_blend one one'."),
    ('modulate', 'modulate', "The colour of the rendering output is multiplied with the scene contents. Generally colours and darkens the scene, good for smoked glass, semi-transparent objects etc. Equivalent to 'scene_blend dest_colour zero'"),
    ('colour_blend', 'colour_blend', 'Colour the scene based on the brightness of the input colours, but dont darken. Equivalent to "scene_blend src_colour one_minus_src_colour"'),
]
for mode in 'dest_colour src_colour one_minus_dest_colour dest_alpha src_alpha one_minus_dest_alpha one_minus_src_alpha'.split():
    _ogre_scene_blend_types.append( ('one %s'%mode, 'one %s'%mode, '') )
del mode

bpy.types.Material.ogre_scene_blend = EnumProperty(
    items=_ogre_scene_blend_types, 
    name='scene blend', 
    description='blending operation of material to scene', 
    default='one zero'
)



###########################################################
_faq_ = '''

Q: i have hundres of objects, is there a way i can merge them on export only?
A: yes, just add them to a group named starting with "merge", or link the group.

Q: can i use subsurf or multi-res on a mesh with an armature?
A: yes.

Q: can i use subsurf or multi-res on a mesh with shape animation?
A: no.

Q: i don't see any objects when i export?
A: you must select the objects you wish to export.

Q: i don't see my animations when exported?
A: make sure you created an NLA strip on the armature.

Q: do i need to bake my IK and other constraints into FK on my armature before export?
A: no.

'''


_doc_installing_ = '''
Installing:
    Installing the Addon:
        You can simply copy io_export_ogreDotScene.py to your blender installation under blender/2.60/scripts/addons/
        and enable it in the user-prefs interface (CTRL+ALT+U)
        Or you can use blenders interface, under user-prefs, click addons, and click 'install-addon'
        (its a good idea to delete the old version first)

    Required:
        1. blender2.60

        2. Install Ogre Command Line tools to the default path ( C:\\OgreCommandLineTools )
            http://www.ogre3d.org/download/tools
            (Linux users may use above and Wine, or install from source, or install via apt-get install ogre-tools)

    Optional:
        3. Install NVIDIA DDS Legacy Utilities    ( install to default path )
            http://developer.nvidia.com/object/dds_utilities_legacy.html
            (Linux users will need to use Wine)

        4. Install Image Magick
            http://www.imagemagick.org

        5. Copy OgreMeshy to C:\\OgreMeshy
            If your using 64bit Windows, you may need to download a 64bit OgreMeshy
            (Linux copy to your home folder)

        6. RealXtend Tundra2
            http://blender2ogre.googlecode.com/files/realxtend-Tundra-2.1.2-OpenGL.7z
            Windows: extract to C:\Tundra2
            Linux: extract to ~/Tundra2
'''



## Options ##
AXIS_MODES =  [
    ('xyz', 'xyz', 'no swapping'),
    ('xz-y', 'xz-y', 'ogre standard'),
    ('-xzy', '-xzy', 'non standard'),
]

def swap(vec):
    if CONFIG['SWAP_AXIS'] == 'xyz': return vec
    elif CONFIG['SWAP_AXIS'] == 'xzy':
        if len(vec) == 3: return mathutils.Vector( [vec.x, vec.z, vec.y] )
        elif len(vec) == 4: return mathutils.Quaternion( [ vec.w, vec.x, vec.z, vec.y] )
    elif CONFIG['SWAP_AXIS'] == '-xzy':
        if len(vec) == 3: return mathutils.Vector( [-vec.x, vec.z, vec.y] )
        elif len(vec) == 4: return mathutils.Quaternion( [ vec.w, -vec.x, vec.z, vec.y] )
    elif CONFIG['SWAP_AXIS'] == 'xz-y':
        if len(vec) == 3: return mathutils.Vector( [vec.x, vec.z, -vec.y] )
        elif len(vec) == 4: return mathutils.Quaternion( [ vec.w, vec.x, vec.z, -vec.y] )
    else:
        print( 'unknown swap axis mode', CONFIG['SWAP_AXIS'] )
        assert 0


############ CONFIG ##############
CONFIG_PATH = bpy.utils.user_resource('CONFIG', path='scripts', create=True)
CONFIG_FILENAME = 'blender2ogre.pickle'
CONFIG_FILEPATH = os.path.join( CONFIG_PATH, CONFIG_FILENAME)



_CONFIG_DEFAULTS_ALL = {
    'TUNDRA_STREAMING' : True,
    'COPY_SHADER_PROGRAMS' : True,
    'MAX_TEXTURE_SIZE' : 4096,
    'SWAP_AXIS' : 'xz-y',         # ogre standard
    'ONLY_ANIMATED_BONES' : False,
    'FORCE_IMAGE_FORMAT' : 'NONE',
    'TOUCH_TEXTURES' : True,
#    'PATH' : '/tmp',

    'SEP_MATS' : True,

    'SCENE' : True,
    'SELONLY' : True,
    'FORCE_CAMERA' : True,
    'FORCE_LAMPS' : True,
    'MESH' : True,
    'MESH_OVERWRITE' : True,
    'ARM_ANIM' : True,
    'SHAPE_ANIM' : True,
    'ARRAY' : True,
    'MATERIALS' : True,
    'DDS_MIPS' : True,
    'TRIM_BONE_WEIGHTS' : 0.01,
    'lodLevels' : 0,
    'lodDistance' : 300,
    'lodPercent' : 40,
    'nuextremityPoints' : 0,
    'generateEdgeLists' : False,
    'generateTangents' : True,     # this is now safe - ignored if mesh is missing UVs
    'tangentSemantic' : 'uvw',
    'tangentUseParity' : 4,
    'tangentSplitMirrored' : False,
    'tangentSplitRotated' : False,
    'reorganiseBuffers' : True,
    'optimiseAnimations' : True,
}

_CONFIG_TAGS_ = 'OGRETOOLS_XML_CONVERTER OGRETOOLS_MESH_MAGICK TUNDRA_ROOT OGRE_MESHY IMAGE_MAGICK_CONVERT NVIDIATOOLS_EXE USER_MATERIALS SHADER_PROGRAMS TUNDRA_STREAMING'.split()
#_CONFIG_TAGS_.append( 'JMONKEY_ROOT' )


_CONFIG_DEFAULTS_WINDOWS = {
#    'JMONKEY_ROOT' : 'C:\\jmonkeyplatform',
    'OGRETOOLS_XML_CONVERTER' : 'C:\\OgreCommandLineTools\\OgreXmlConverter.exe',
    'OGRETOOLS_MESH_MAGICK' : 'C:\\OgreCommandLineTools\\MeshMagick.exe',
    'TUNDRA_ROOT' : 'C:\\Tundra2',
    'OGRE_MESHY' : 'C:\\OgreMeshy\\Ogre Meshy.exe',
    'IMAGE_MAGICK_CONVERT' : 'C:\\Program Files\\ImageMagick\\convert.exe',
    'NVIDIATOOLS_EXE' : 'C:\\Program Files\\NVIDIA Corporation\\DDS Utilities\\nvdxt.exe',
    'USER_MATERIALS' : 'C:\\Tundra2\\media\\materials',
    'SHADER_PROGRAMS' : 'C:\\Tundra2\\media\\materials\\programs',
}

_CONFIG_DEFAULTS_UNIX = {
#    'JMONKEY_ROOT' : '/usr/local/jmonkeyplatform',
    'OGRETOOLS_XML_CONVERTER' : '/usr/local/bin/OgreXMLConverter',  # source build is better
    'OGRETOOLS_MESH_MAGICK' : '/usr/local/bin/MeshMagick',
    'TUNDRA_ROOT' : '~/Tundra2',
    'OGRE_MESHY' : '~/OgreMeshy/Ogre Meshy.exe',
    'IMAGE_MAGICK_CONVERT' : '/usr/bin/convert',
    'NVIDIATOOLS_EXE' : '~/.wine/drive_c/Program Files/NVIDIA Corporation/DDS Utilities',
    'USER_MATERIALS' : '~/Tundra2/media/materials',
    'SHADER_PROGRAMS' : '~/Tundra2/media/materials/programs',
#    'USER_MATERIALS' : '~/ogre_src_v1-7-3/Samples/Media/materials',
#    'SHADER_PROGRAMS' : '~/ogre_src_v1-7-3/Samples/Media/materials/programs',
}

if sys.platform.startswith('linux') or sys.platform.startswith('darwin') or sys.platform.startswith('freebsd'):
    for tag in _CONFIG_DEFAULTS_UNIX:
        path = _CONFIG_DEFAULTS_UNIX[ tag ]
        if path.startswith('~'): _CONFIG_DEFAULTS_UNIX[ tag ] = os.path.expanduser( path )
        elif tag.startswith('OGRETOOLS') and not os.path.isfile( path ):
            _CONFIG_DEFAULTS_UNIX[ tag ] = os.path.join( '/usr/bin', os.path.split( path )[-1] )
    del tag
    del path


CONFIG = {}
def load_config():  # PUBLIC API
    global CONFIG
    if os.path.isfile( CONFIG_FILEPATH ):
        try:
            with open( CONFIG_FILEPATH, 'rb' ) as f:
                CONFIG = pickle.load( f )
                print('CONFIG LOADED')
        except:
            print( 'IO ERROR, can not read: %s' %CONFIG_FILEPATH )

    for tag in _CONFIG_DEFAULTS_ALL:
        if tag not in CONFIG:
            CONFIG[ tag ] = _CONFIG_DEFAULTS_ALL[ tag ]

    for tag in _CONFIG_TAGS_:
        if tag not in CONFIG:
            if sys.platform.startswith('win'):
                CONFIG[ tag ] = _CONFIG_DEFAULTS_WINDOWS[ tag ]
            elif sys.platform.startswith('linux') or sys.platform.startswith('darwin') or sys.platform.startswith('freebsd'):
                CONFIG[ tag ] = _CONFIG_DEFAULTS_UNIX[ tag ]
            else:
                print( 'ERROR: unknown platform' )
                assert 0

    for tag in _CONFIG_TAGS_: ## setup temp hidden RNA  to expose the file paths ##
        default = CONFIG[ tag ]
        func = eval( 'lambda self,con: CONFIG.update( {"%s" : self.%s} )' %(tag,tag) )
        if type(default) is bool:
            prop = BoolProperty(
                name=tag, description='updates bool setting', default=default, 
                options={'SKIP_SAVE'}, update=func
            )
        else:
            prop = StringProperty(
                name=tag, description='updates path setting', maxlen=128, default=default, 
                options={'SKIP_SAVE'}, update=func
            )
        setattr( bpy.types.WindowManager, tag, prop )
    return CONFIG
CONFIG = load_config()

def save_config():  # PUBLIC API
    print('saving config....')
    #for key in CONFIG: print( '%s =   %s' %(key, CONFIG[key]) )
    if os.path.isdir( CONFIG_PATH ):
        try:
            with open( CONFIG_FILEPATH, 'wb' ) as f:
                pickle.dump( CONFIG, f, -1 )
                print('SAVED CONFIG')
        except:
            print( 'IO ERROR, can not write: %s' %CONFIG_FILEPATH )
    else:
        print( 'ERROR: config path is missing %s' %CONFIG_PATH )


class Blender2Ogre_ConfigOp(bpy.types.Operator):
    '''operator: saves current b2ogre configuration'''  
    bl_idname = "ogre.save_config"  
    bl_label = "save config file"
    bl_options = {'REGISTER'}
    @classmethod
    def poll(cls, context): return True
    def invoke(self, context, event):
        save_config()
        Report.reset()
        Report.messages.append('SAVED %s' %CONFIG_FILEPATH)
        Report.show()
        return {'FINISHED'}


# customize missing material - red flags for users so they can quickly see what they forgot to assign a material to.
# (do not crash if no material on object - thats annoying for the user)
MISSING_MATERIAL = '''
material _missing_material_ 
{
    receive_shadows off
    technique
    {
        pass
        {
            ambient 0.1 0.1 0.1 1.0
            diffuse 0.8 0.0 0.0 1.0
            specular 0.5 0.5 0.5 1.0 12.5
            emissive 0.3 0.3 0.3 1.0
        }
    }
}
'''



############# helper functions ##############

def find_bone_index( ob, arm, groupidx):    # sometimes the groups are out of order, this finds the right index.
    if groupidx < len(ob.vertex_groups):        # reported by Slacker
        vg = ob.vertex_groups[ groupidx ]
        for i,bone in enumerate(arm.pose.bones):
            if bone.name == vg.name: return i
    else:
        print('WARNING: object vertex groups not in sync with armature', ob, arm, groupidx)


def mesh_is_smooth( mesh ):
    for face in mesh.faces:
        if face.use_smooth: return True

## this breaks if users have uv layers with same name with different indices over different objects ##
def find_uv_layer_index( uvname, material=None ):
    idx = 0
    for mesh in bpy.data.meshes:
        if material is None or material.name in mesh.materials:
            if mesh.uv_textures:
                names = [ uv.name for uv in mesh.uv_textures ]
                if uvname in names:
                    idx = names.index( uvname )
                    break   # should we check all objects using material and enforce the same index?
    return idx

def has_custom_property( a, name ):
    for prop in a.items():
        n,val = prop
        if n == name: return True


# a default plane, with simple-subsurf and displace modifier on Z
def is_strictly_simple_terrain( ob ):
    if len(ob.data.vertices) != 4 and len(ob.data.faces) != 1: return False
    elif len(ob.modifiers) < 2: return False
    elif ob.modifiers[0].type != 'SUBSURF' or ob.modifiers[1].type != 'DISPLACE': return False
    elif ob.modifiers[0].subdivision_type != 'SIMPLE': return False
    elif ob.modifiers[1].direction != 'Z': return False # disallow NORMAL and other modes
    else: return True

def get_image_textures( mat ):
    r = []
    for s in mat.texture_slots:
        if s and s.texture.type == 'IMAGE': r.append( s )
    return r


def indent( level, *args ):
    if not args: return '\t' * level
    else:
        a = ''
        for line in args:
            a += '    ' * level
            a += line
            a += '\n'
        return a

def gather_instances():
    instances = {}
    for ob in bpy.context.scene.objects:
        if ob.data and ob.data.users > 1:
            if ob.data not in instances: instances[ ob.data ] = []
            instances[ ob.data ].append( ob )
    return instances

def select_instances( context, name ):
    for ob in bpy.context.scene.objects: ob.select = False
    ob = bpy.context.scene.objects[ name ]
    if ob.data:
        inst = gather_instances()
        for ob in inst[ ob.data ]: ob.select = True
        bpy.context.scene.objects.active = ob


def select_group( context, name, options={} ):
    for ob in bpy.context.scene.objects: ob.select = False
    for grp in bpy.data.groups:
        if grp.name == name:
            #context.scene.objects.active = grp.objects
            #Note that the context is read-only. These values cannot be modified directly, though they may be changed by running API functions or by using the data API. So bpy.context.object = obj will raise an error. But bpy.context.scene.objects.active = obj will work as expected. - http://wiki.blender.org/index.php?title=Dev:2.5/Py/API/Intro&useskin=monobook
            bpy.context.scene.objects.active = grp.objects[0]
            for ob in grp.objects: ob.select = True
        else: pass


def get_objects_using_materials( mats ):
    obs = []
    for ob in bpy.data.objects:
        if ob.type == 'MESH':
            for mat in ob.data.materials:
                if mat in mats:
                    if ob not in obs: obs.append( ob )
                    break
    return obs

def get_materials_using_image( img ):
    mats = []
    for mat in bpy.data.materials:
        for slot in get_image_textures( mat ):
            if slot.texture.image == img:
                if mat not in mats: mats.append( mat )
    return mats


def get_parent_matrix( ob, objects ):
    if not ob.parent:
        return mathutils.Matrix(((1,0,0,0),(0,1,0,0),(0,0,1,0),(0,0,0,1)))   # Requiered for Blender SVN > 2.56
    else:
        if ob.parent in objects:
            return ob.parent.matrix_world.copy()
        else:
            return get_parent_matrix(ob.parent, objects)


def merge_group( group ):
    print('--------------- merge group ->', group )
    copies = []
    for ob in group.objects:
        if ob.type == 'MESH':
            print( '\t group member', ob.name )
            o2 = ob.copy(); copies.append( o2 )
            o2.data = o2.to_mesh(bpy.context.scene, True, "PREVIEW")    # collaspe modifiers
            while o2.modifiers: o2.modifiers.remove( o2.modifiers[0] )
            bpy.context.scene.objects.link( o2 )#; o2.select = True
    merged = merge( copies )
    merged.name = group.name
    merged.data.name = group.name
    return merged

def merge_objects( objects, name='_temp_', transform=None ):
    assert objects
    copies = []
    for ob in objects:
        ob.select = False
        if ob.type == 'MESH':
            o2 = ob.copy(); copies.append( o2 )
            o2.data = o2.to_mesh(bpy.context.scene, True, "PREVIEW")    # collaspe modifiers
            while o2.modifiers: o2.modifiers.remove( o2.modifiers[0] )
            if transform: o2.matrix_world =  transform * o2.matrix_local
            bpy.context.scene.objects.link( o2 )#; o2.select = True
    merged = merge( copies )
    merged.name = name
    merged.data.name = name
    return merged


def merge( objects ):
    print('MERGE', objects)
    for ob in bpy.context.selected_objects: ob.select = False
    for ob in objects:
        print('\t'+ob.name)
        ob.select = True
        assert not ob.library
    bpy.context.scene.objects.active = ob
    bpy.ops.object.join()
    return bpy.context.active_object

def get_merge_group( ob, prefix='merge' ):
    m = []
    for grp in ob.users_group:
        if grp.name.lower().startswith(prefix): m.append( grp )
    if len(m)==1:
        #if ob.data.users != 1:
        #    print( 'WARNING: an instance can not be in a merge group' )
        #    return
        return m[0]
    elif m:
        print('WARNING: an object can not be in two merge groups at the same time', ob)
        return


def wordwrap( txt ):
    r = ['']
    for word in txt.split(' '):    # do not split on tabs
        word = word.replace('\t', ' '*3)
        r[-1] += word + ' '
        if len(r[-1]) > 90: r.append( '' )
    return r


###### RPython xml dom ######
class RElement(object):
	def appendChild( self, child ): self.childNodes.append( child )
	def setAttribute( self, name, value ): self.attributes[name]=value

	def __init__(self, tag):
		self.tagName = tag
		self.childNodes = []
		self.attributes = {}

	def toprettyxml(self, lines, indent ):
		s = '<%s ' %self.tagName
		for name in self.attributes:
			value = self.attributes[name]
			s += '%s="%s" ' %(name,value)
		if not self.childNodes:
			s += '/>'; lines.append( ('\t'*indent)+s )
		else:
			s += '>'; lines.append( ('\t'*indent)+s )
			indent += 1
			for child in self.childNodes:
				child.toprettyxml( lines, indent )
			indent -= 1
			lines.append( ('\t'*indent)+'</%s>' %self.tagName )


class RDocument(object):
	def __init__(self): self.documentElement = None
	def appendChild(self,root): self.documentElement = root
	def createElement(self,tag): e = RElement(tag); e.document = self; return e
	def toprettyxml(self):
		indent = 0
		lines = []
		self.documentElement.toprettyxml( lines, indent )
		return '\n'.join( lines )


class SimpleSaxWriter():
    def __init__(self, output, encoding, top_level_tag, attrs):
        xml_writer = XMLGenerator(output, encoding, True)
        xml_writer.startDocument()
        xml_writer.startElement(top_level_tag, attrs)
        self._xml_writer = xml_writer
        self.top_level_tag = top_level_tag
        self.ident=4
        self._xml_writer.characters('\n')

    def start_tag(self, name, attrs):
        self._xml_writer.characters(" " * self.ident)
        self._xml_writer.startElement(name, attrs)
        self.ident += 4
        self._xml_writer.characters('\n')

    def end_tag(self, name):
        self.ident -= 4
        self._xml_writer.characters(" " * self.ident)
        self._xml_writer.endElement(name)
        self._xml_writer.characters('\n')

    def leaf_tag(self, name, attrs):
        self._xml_writer.characters(" " * self.ident)
        self._xml_writer.startElement(name, attrs)
        self._xml_writer.endElement(name)
        self._xml_writer.characters('\n')

    def close(self):
        self._xml_writer.endElement(self.top_level_tag)
        self._xml_writer.endDocument()


############## Report Hack ############
class ReportSingleton(object):
    def show(self): pass#bpy.ops.wm.call_menu( name='MiniReport' )
    def __init__(self): self.reset()
    def reset(self):
        self.materials = []
        self.meshes = []
        self.lights = []
        self.cameras = []
        self.armatures = []
        self.armature_animations = []
        self.shape_animations = []
        self.textures = []
        self.vertices = 0
        self.orig_vertices = 0
        self.faces = 0
        self.triangles = 0
        self.warnings = []
        self.errors = []
        self.messages = []
        self.paths = []

    def report(self):
        r = ['Report:']
        ex = ['Extended Report:']
        if self.errors:
            r.append( '  ERRORS:' )
            for a in self.errors: r.append( '    . %s' %a )

        #if not bpy.context.selected_objects:
        #    self.warnings.append('YOU DID NOT SELECT ANYTHING TO EXPORT')
        if self.warnings:
            r.append( '  WARNINGS:' )
            for a in self.warnings: r.append( '    . %s' %a )

        if self.messages:
            r.append( '  MESSAGES:' )
            for a in self.messages: r.append( '    . %s' %a )
        if self.paths:
            r.append( '  PATHS:' )
            for a in self.paths: r.append( '    . %s' %a )


        if self.vertices:
            r.append( '  Original Vertices: %s' %self.orig_vertices)
            r.append( '  Exported Vertices: %s' %self.vertices )
            r.append( '  Original Faces: %s' %self.faces )
            r.append( '  Exported Triangles: %s' %self.triangles )
            ## TODO report file sizes, meshes and textures

        for tag in 'meshes lights cameras armatures armature_animations shape_animations materials textures'.split():
            attr = getattr(self, tag)
            if attr:
                name = tag.replace('_',' ').upper()
                r.append( '  %s: %s' %(name, len(attr)) )
                if attr:
                    ex.append( '  %s:' %name )
                    for a in attr: ex.append( '    . %s' %a )

        txt = '\n'.join( r )
        ex = '\n'.join( ex )        # console only - extended report
        print('_'*80)
        print(txt)
        print(ex)
        print('_'*80)
        return txt

Report = ReportSingleton()





class MiniReport(bpy.types.Menu):
    bl_label = "Mini-Report | (see console for full report)"
    def draw(self, context):
        layout = self.layout
        txt = Report.report()
        for line in txt.splitlines():
            layout.label(text=line)


#################### New Physics ####################

_physics_modes =  [
    ('NONE', 'NONE', 'no physics'),
    ('RIGID_BODY', 'RIGID_BODY', 'rigid body'),
    ('SOFT_BODY', 'SOFT_BODY', 'soft body'),
]

bpy.types.Object.physics_mode = EnumProperty(
    items = _physics_modes, 
    name = 'physics mode', 
    description='physics mode', 
    default='NONE'
)

bpy.types.Object.physics_friction = FloatProperty(
    name='Simple Friction', description='physics friction', default=0.1, min=0.0, max=1.0)

bpy.types.Object.physics_bounce = FloatProperty(
    name='Simple Bounce', description='physics bounce', default=0.01, min=0.0, max=1.0)


bpy.types.Object.collision_terrain_x_steps = IntProperty(
    name="Ogre Terrain: x samples", description="resolution in X of height map", 
    default=64, min=4, max=8192)
bpy.types.Object.collision_terrain_y_steps = IntProperty(
    name="Ogre Terrain: y samples", description="resolution in Y of height map", 
    default=64, min=4, max=8192)

_collision_modes =  [
    ('NONE', 'NONE', 'no collision'),
    ('PRIMITIVE', 'PRIMITIVE', 'primitive collision type'),
    ('MESH', 'MESH', 'triangle-mesh or convex-hull collision type'),
    ('DECIMATED', 'DECIMATED', 'auto-decimated collision type'),
    ('COMPOUND', 'COMPOUND', 'children primitive compound collision type'),
    ('TERRAIN', 'TERRAIN', 'terrain (height map) collision type'),
]

bpy.types.Object.collision_mode = EnumProperty(
    items = _collision_modes, 
    name = 'primary collision mode', 
    description='collision mode', 
    default='NONE'
)


bpy.types.Object.subcollision = BoolProperty(
    name="collision compound", description="member of a collision compound", default=False)


######################

def _get_proxy_decimate_mod( ob ):
    proxy = None
    for child in ob.children:
        if child.subcollision and child.name.startswith('DECIMATED'):
            for mod in child.modifiers:
                if mod.type == 'DECIMATE': return mod

def bake_terrain( ob, normalize=True ):
    assert ob.collision_mode == 'TERRAIN'
    terrain = None
    for child in ob.children:
        if child.subcollision and child.name.startswith('TERRAIN'):
            terrain = child
            break
    assert terrain
    data = terrain.to_mesh(bpy.context.scene, True, "PREVIEW")
    raw = [ v.co.z for v in data.vertices ]
    Zmin = min( raw )
    Zmax = max( raw )
    depth = Zmax-Zmin
    m = 1.0 / depth

    rows = []
    i = 0
    for x in range( ob.collision_terrain_x_steps ):
        row = []
        for y in range( ob.collision_terrain_y_steps ):
            v = data.vertices[ i ]
            if normalize: z = (v.co.z - Zmin) * m
            else: z = v.co.z
            row.append( z )
            i += 1
        if x%2: row.reverse()   # blender grid prim zig-zags
        rows.append( row )

    return {'data':rows, 'min':Zmin, 'max':Zmax, 'depth':depth}

def save_terrain_as_NTF( path, ob ):    # Tundra format - hardcoded 16x16 patch format
    info = bake_terrain( ob )
    url = os.path.join( path, '%s.ntf'%ob.data.name )
    f = open(url, "wb")
    buf = array.array("I")  # header
    xs = ob.collision_terrain_x_steps
    ys = ob.collision_terrain_y_steps
    xpatches = int(xs/16)
    ypatches = int(ys/16)
    header = [ xpatches, ypatches ]
    buf.fromlist( header )
    buf.tofile(f)
    ########## body ###########
    rows = info['data']
    for x in range( xpatches ):
        for y in range( ypatches ):
            patch = []
            for i in range(16):
                for j in range(16):
                    v = rows[ (x*16)+i ][ (y*16)+j ]
                    patch.append( v )
            buf = array.array("f")
            buf.fromlist( patch )
            buf.tofile(f)
    f.close()
    path,name = os.path.split(url)
    R = {
        'url':url, 'min':info['min'], 'max':info['max'], 'path':path, 'name':name,
        'xpatches': xpatches, 'ypatches': ypatches,
        'depth':info['depth'],
    }
    return R


class OgreCollisionOp(bpy.types.Operator):
    '''ogre collision'''  
    bl_idname = "ogre.set_collision"  
    bl_label = "modify collision"
    bl_options = {'REGISTER'}

    MODE = StringProperty(name="toggle mode", maxlen=32, default="disable")

    @classmethod
    def poll(cls, context):
        if context.active_object and context.active_object.type == 'MESH': return True

    def get_subcollisions( self, ob, create=True ):
        r = get_subcollisions( ob )
        if not r and create:
            method = getattr(self, 'create_%s'%ob.collision_mode)
            p = method(ob)
            p.name = '%s.%s' %(ob.collision_mode, ob.name)
            p.subcollision = True
            r.append( p )
        return r

    def create_DECIMATED(self, ob):
        child = ob.copy()
        bpy.context.scene.objects.link( child )
        child.matrix_local = mathutils.Matrix()
        child.parent = ob
        child.hide_select = True
        child.draw_type = 'WIRE'
        #child.select = False
        child.lock_location = [True]*3
        child.lock_rotation = [True]*3
        child.lock_scale = [True]*3
        decmod = child.modifiers.new('proxy', type='DECIMATE')
        decmod.ratio = 0.5
        return child

    def create_TERRAIN(self, ob):
        x = ob.collision_terrain_x_steps
        y = ob.collision_terrain_y_steps
        #################################
        #pos = ob.matrix_world.to_translation()
        bpy.ops.mesh.primitive_grid_add(
            x_subdivisions=x, 
            y_subdivisions=y, 
            size=1.0 )      #, location=pos )
        grid = bpy.context.active_object
        assert grid.name.startswith('Grid')
        grid.collision_terrain_x_steps = x
        grid.collision_terrain_y_steps = y
        #############################
        x,y,z = ob.dimensions
        sx,sy,sz = ob.scale
        x *= 1.0/sx
        y *= 1.0/sy
        z *= 1.0/sz
        grid.scale.x = x/2
        grid.scale.y = y/2
        grid.location.z -= z/2
        grid.data.show_all_edges = True
        grid.draw_type = 'WIRE'
        grid.hide_select = True
        #grid.select = False
        grid.lock_location = [True]*3
        grid.lock_rotation = [True]*3
        grid.lock_scale = [True]*3
        grid.parent = ob
        bpy.context.scene.objects.active = ob
        mod = grid.modifiers.new(name='temp', type='SHRINKWRAP')
        mod.wrap_method = 'PROJECT'
        mod.use_project_z = True
        mod.target = ob
        mod.cull_face = 'FRONT'
        return grid


    def invoke(self, context, event):
        ob = context.active_object
        game = ob.game

        subtype = None
        if ':' in self.MODE:
            mode, subtype = self.MODE.split(':')
            ##BLENDERBUG##ob.game.collision_bounds_type = subtype   # BUG this can not come before
            if subtype in 'BOX SPHERE CYLINDER CONE CAPSULE'.split():
                ob.draw_bounds_type = subtype
            else:
                ob.draw_bounds_type = 'POLYHEDRON'
            ob.game.collision_bounds_type = subtype  # BLENDERBUG - this must come after draw_bounds_type assignment
        else:
            mode = self.MODE
        ob.collision_mode = mode

        if ob.data.show_all_edges: ob.data.show_all_edges = False
        if ob.show_texture_space: ob.show_texture_space = False
        if ob.show_bounds: ob.show_bounds = False
        if ob.show_wire: ob.show_wire = False
        for child in ob.children:
            if child.subcollision and not child.hide: child.hide = True


        if mode == 'NONE':
            game.use_ghost = True
            game.use_collision_bounds = False

        elif mode == 'PRIMITIVE':
            game.use_ghost = False
            game.use_collision_bounds = True
            ob.show_bounds = True

        elif mode == 'MESH':
            game.use_ghost = False
            game.use_collision_bounds = True
            ob.show_wire = True

            if game.collision_bounds_type == 'CONVEX_HULL':
                ob.show_texture_space = True
            else:
                ob.data.show_all_edges = True

        elif mode == 'DECIMATED':
            game.use_ghost = True
            game.use_collision_bounds = False
            game.use_collision_compound = True

            proxy = self.get_subcollisions(ob)[0]
            if proxy.hide: proxy.hide = False
            ob.game.use_collision_compound = True  # proxy
            mod = _get_proxy_decimate_mod( ob )
            mod.show_viewport = True
            if not proxy.select:    # ugly (but works)
                proxy.hide_select = False
                proxy.select = True
                proxy.hide_select = True

            if game.collision_bounds_type == 'CONVEX_HULL':
                ob.show_texture_space = True


        elif mode == 'TERRAIN':
            game.use_ghost = True
            game.use_collision_bounds = False
            game.use_collision_compound = True

            proxy = self.get_subcollisions(ob)[0]
            if proxy.hide: proxy.hide = False


        elif mode == 'COMPOUND':
            game.use_ghost = True
            game.use_collision_bounds = False
            game.use_collision_compound = True

        else: assert 0    # unknown mode

        return {'FINISHED'}

################################
@UI
class PANEL_Physics(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_label = "Physics"
    @classmethod
    def poll(cls, context):
        if context.active_object: return True
        else: return False

    def draw(self, context):
        layout = self.layout
        ob = context.active_object
        game = ob.game

        if ob.type != 'MESH': return
        elif ob.subcollision == True:
            box = layout.box()
            if ob.parent:
                box.label(text='object is a collision proxy for: %s' %ob.parent.name)
            else:
                box.label(text='WARNING: collision proxy missing parent')
            return

        #####################
        box = layout.box()
        box.prop(ob, 'physics_mode')
        if ob.physics_mode != 'NONE':
            box.prop(game, 'mass', text='Mass')
            box.prop(ob, 'physics_friction', text='Friction', slider=True)
            box.prop(ob, 'physics_bounce', text='Bounce', slider=True)

            box.label(text="Damping:")
            box.prop(game, 'damping', text='Translation', slider=True)
            box.prop(game, 'rotation_damping', text='Rotation', slider=True)

            box.label(text="Velocity:")
            box.prop(game, "velocity_min", text="Minimum")
            box.prop(game, "velocity_max", text="Maximum")


################################
@UI
class PANEL_Collision(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_label = "Collision"
    @classmethod
    def poll(cls, context):
        if context.active_object: return True
        else: return False

    def draw(self, context):
        layout = self.layout
        ob = context.active_object
        game = ob.game

        if ob.type != 'MESH': return
        elif ob.subcollision == True:
            box = layout.box()
            if ob.parent:
                box.label(text='object is a collision proxy for: %s' %ob.parent.name)
            else:
                box.label(text='WARNING: collision proxy missing parent')
            return

        #####################
        mode = ob.collision_mode
        if mode == 'NONE':
            box = layout.box()
            op = box.operator( 'ogre.set_collision', text='Enable Collision', icon='PHYSICS' )
            op.MODE = 'PRIMITIVE:%s' %game.collision_bounds_type

        else:
            prim = game.collision_bounds_type

            box = layout.box()
            op = box.operator( 'ogre.set_collision', text='Disable Collision', icon='X' )
            op.MODE = 'NONE'
            box.prop(game, "collision_margin", text="Collision Margin", slider=True)


            box = layout.box()
            if mode == 'PRIMITIVE': box.label(text='Primitive: %s' %prim)
            else: box.label(text='Primitive')
            row = box.row()
            _icons = {
                'BOX':'MESH_CUBE', 'SPHERE':'MESH_UVSPHERE', 'CYLINDER':'MESH_CYLINDER',
                'CONE':'MESH_CONE', 'CAPSULE':'META_CAPSULE'}
            for a in 'BOX SPHERE CYLINDER CONE CAPSULE'.split():
                if prim == a and mode == 'PRIMITIVE':
                    op = row.operator( 'ogre.set_collision', text='', icon=_icons[a], emboss=True )
                    op.MODE = 'PRIMITIVE:%s' %a
                else:
                    op = row.operator( 'ogre.set_collision', text='', icon=_icons[a], emboss=False )
                    op.MODE = 'PRIMITIVE:%s' %a

            box = layout.box()
            if mode == 'MESH': box.label(text='Mesh: %s' %prim.split('_')[0] )
            else: box.label(text='Mesh')
            row = box.row()
            row.label(text='- - - - - - - - - - - - - -')
            _icons = {'TRIANGLE_MESH':'MESH_ICOSPHERE', 'CONVEX_HULL':'SURFACE_NCURVE'}
            for a in 'TRIANGLE_MESH CONVEX_HULL'.split():
                if prim == a and mode == 'MESH':
                    op = row.operator( 'ogre.set_collision', text='', icon=_icons[a], emboss=True )
                    op.MODE = 'MESH:%s' %a
                else:
                    op = row.operator( 'ogre.set_collision', text='', icon=_icons[a], emboss=False )
                    op.MODE = 'MESH:%s' %a

            box = layout.box()
            if mode == 'DECIMATED':
                box.label(text='Decimate: %s' %prim.split('_')[0] )
                row = box.row()
                mod = _get_proxy_decimate_mod( ob )
                assert mod  # decimate modifier is missing
                row.label(text='Faces: %s' %mod.face_count )
                box.prop( mod, 'ratio', text='' )

            else:
                box.label(text='Decimate')
                row = box.row()
                row.label(text='- - - - - - - - - - - - - -')

            _icons = {'TRIANGLE_MESH':'MESH_ICOSPHERE', 'CONVEX_HULL':'SURFACE_NCURVE'}
            for a in 'TRIANGLE_MESH CONVEX_HULL'.split():
                if prim == a and mode == 'DECIMATED':
                    op = row.operator( 'ogre.set_collision', text='', icon=_icons[a], emboss=True )
                    op.MODE = 'DECIMATED:%s' %a
                else:
                    op = row.operator( 'ogre.set_collision', text='', icon=_icons[a], emboss=False )
                    op.MODE = 'DECIMATED:%s' %a

            box = layout.box()
            if mode == 'TERRAIN':
                terrain = get_subcollisions( ob )[0]
                if ob.collision_terrain_x_steps != terrain.collision_terrain_x_steps or ob.collision_terrain_y_steps != terrain.collision_terrain_y_steps:
                    op = box.operator( 'ogre.set_collision', text='Rebuild Terrain', icon='MESH_GRID' )
                    op.MODE = 'TERRAIN'
                else:
                    box.label(text='Terrain:')

                row = box.row()
                row.prop( ob, 'collision_terrain_x_steps', 'X' )
                row.prop( ob, 'collision_terrain_y_steps', 'Y' )
                #box.prop( terrain.modifiers[0], 'offset' ) # gets normalized away
                box.prop( terrain.modifiers[0], 'cull_face', text='Cull' )
                box.prop( terrain, 'location' )     # TODO hide X and Y

            else:
                op = box.operator( 'ogre.set_collision', text='Terrain Collision', icon='MESH_GRID' )
                op.MODE = 'TERRAIN'

            box = layout.box()
            if mode == 'COMPOUND':
                op = box.operator( 'ogre.set_collision', text='Compound Collision', icon='ROTATECOLLECTION' )
            else:
                op = box.operator( 'ogre.set_collision', text='Compound Collision', icon='ROTATECOLLECTION' )
            op.MODE = 'COMPOUND'

@UI
class PANEL_Configure(bpy.types.Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"
    bl_label = "Ogre Configuration File"
    def draw(self, context):
        layout = self.layout
        op = layout.operator( 'ogre.save_config', text='update config file', icon='FILE' )
        for tag in _CONFIG_TAGS_:
            layout.prop( context.window_manager, tag )



##################################################################
_game_logic_intro_doc_ = '''
Hijacking the BGE

Blender contains a fully functional game engine (BGE) that is highly useful for learning the concepts of game programming by breaking it down into three simple parts: Sensor, Controller, and Actuator.  An Ogre based game engine will likely have similar concepts in its internal API and game logic scripting.  Without a custom interface to define game logic, very often game designers may have to resort to having programmers implement their ideas in purely handwritten script.  This is prone to breakage because object names then end up being hard-coded.  Not only does this lead to non-reusable code, its also a slow process.  Why should we have to resort to this when Blender already contains a very rich interface for game logic?  By hijacking a subset of the BGE interface we can make this workflow between game designer and game programmer much better.

The OgreDocScene format can easily be extened to include extra game logic data.  While the BGE contains some features that can not be easily mapped to other game engines, there are many are highly useful generic features we can exploit, including many of the Sensors and Actuators.  Blender uses the paradigm of: 1. Sensor -> 2. Controller -> 3. Actuator.  In pseudo-code, this can be thought of as: 1. on-event -> 2. conditional logic -> 3. do-action.  The designer is most often concerned with the on-events (the Sensors), and the do-actions (the Actuators); and the BGE interface provides a clear way for defining and editing those.  Its a harder task to provide a good interface for the conditional logic (Controller), that is flexible enough to fit everyones different Ogre engine and requirements, so that is outside the scope of this exporter at this time.  A programmer will still be required to fill the gap between Sensor and Actuator, but hopefully his work is greatly reduced and can write more generic/reuseable code.

The rules for which Sensors trigger which Actuators is left undefined, as explained above we are hijacking the BGE interface not trying to export and reimplement everything.  BGE Controllers and all links are ignored by the exporter, so whats the best way to define Sensor/Actuator relationships?  One convention that seems logical is to group Sensors and Actuators by name.  More complex syntax could be used in Sensor/Actuators names, or they could be completely ignored and instead all the mapping is done by the game programmer using other rules.  This issue is not easily solved so designers and the engine programmers will have to decide upon their own conventions, there is no one size fits all solution.
'''


_ogre_logic_types_doc_ = '''
Supported Sensors:
    . Collision
    . Near
    . Radar
    . Touching
    . Raycast
    . Message

Supported Actuators:
    . Shape Action*
    . Edit Object
    . Camera
    . Constraint
    . Message
    . Motion
    . Sound
    . Visibility

*note: Shape Action
The most common thing a designer will want to do is have an event trigger an animation.  The BGE contains an Actuator called "Shape Action", with useful properties like: start/end frame, and blending.  It also contains a property called "Action" but this is hidden because the exporter ignores action names and instead uses the names of NLA strips when exporting Ogre animation tracks.  The current workaround is to hijack the "Frame Property" attribute and change its name to "animation".  The designer can then simply type the name of the animation track (NLA strip).  Any custom syntax could actually be implemented here for calling animations, its up to the engine programmer to define how this field will be used.  For example: "*.explode" could be implemented to mean "on all objects" play the "explode" animation.

'''

class _WrapLogic(object):
    ## custom name hacks ##
    SwapName = {
        'frame_property' : 'animation',
    }
    def __init__(self, node):
        self.node = node
        self.name = node.name
        self.type = node.type
    def widget(self, layout):
        box = layout.box()
        row = box.row()
        row.label( text=self.type )
        row.separator()
        row.prop( self.node, 'name', text='' )
        if self.type in self.TYPES:
            for name in self.TYPES[ self.type ]:
                if name in self.SwapName:
                    box.prop( self.node, name, text=self.SwapName[name] )
                else:
                    box.prop( self.node, name )

    def xml( self, doc ):
        g = doc.createElement( self.LogicType )
        g.setAttribute('name', self.name)
        g.setAttribute('type', self.type)

        if self.type in self.TYPES:
            for name in self.TYPES[ self.type ]:
                attr = getattr( self.node, name )
                if name in self.SwapName: name = self.SwapName[name]
                a = doc.createElement( 'component' )
                g.appendChild(a)
                a.setAttribute('name', name)
                if attr is None: a.setAttribute('type', 'POINTER' )
                else: a.setAttribute('type', type(attr).__name__)

                if type(attr) in (float, int, str, bool): a.setAttribute('value', str(attr))
                elif not attr: a.setAttribute('value', '')        # None case
                elif hasattr(attr,'filepath'): a.setAttribute('value', attr.filepath)
                elif hasattr(attr,'name'): a.setAttribute('value', attr.name)
                elif hasattr(attr,'x') and hasattr(attr,'y') and hasattr(attr,'z'):
                    a.setAttribute('value', '%s %s %s' %(attr.x, attr.y, attr.z))
                else:
                    print('ERROR: unknown type', attr)

        return g

class WrapSensor( _WrapLogic ):
    LogicType = 'sensor'
    TYPES = {
        'COLLISION': ['property'],
        'MESSAGE' : ['subject'],
        'NEAR' : ['property', 'distance', 'reset_distance'],
        'RADAR'  :  ['property', 'axis', 'angle', 'distance' ],
        'RAY'  :  ['ray_type', 'property', 'material', 'axis', 'range', 'use_x_ray'],
        'TOUCH'  :  ['material'],
    }

class WrapActuator( _WrapLogic ):
    LogicType = 'actuator'
    TYPES = {
        'CAMERA'  :  ['object', 'height', 'min', 'max', 'axis'],
        'CONSTRAINT'  :  ['mode', 'limit', 'limit_min', 'limit_max', 'damping'], 
        'MESSAGE' : ['to_property', 'subject', 'body_message'],        #skipping body_type
        'OBJECT'  :  'damping derivate_coefficient force force_max_x force_max_y force_max_z force_min_x force_min_y force_min_z integral_coefficient linear_velocity mode offset_location offset_rotation proportional_coefficient reference_object torque use_local_location use_local_rotation use_local_torque use_servo_limit_x use_servo_limit_y use_servo_limit_z'.split(),
        'SOUND'  :  'cone_inner_angle_3d cone_outer_angle_3d cone_outer_gain_3d distance_3d_max distance_3d_reference gain_3d_max gain_3d_min mode pitch rolloff_factor_3d sound use_sound_3d volume'.split(),        # note .sound contains .filepath
        'VISIBILITY'  :  'apply_to_children use_occlusion use_visible'.split(),
        'SHAPE_ACTION'  :  'frame_blend_in frame_end frame_property frame_start mode property use_continue_last_frame'.split(),
        'EDIT_OBJECT'  :  'dynamic_operation linear_velocity mass mesh mode object time track_object use_3d_tracking use_local_angular_velocity use_local_linear_velocity use_replace_display_mesh use_replace_physics_mesh'.split(),
    }



class _OgreMatPass( object ):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "material"

    @classmethod
    def poll(cls, context):
        if context.active_object and context.active_object.active_material and context.active_object.active_material.use_material_passes:
            return True

    def draw(self, context):
        if not hasattr(context, "material"): return
        if not context.active_object: return
        if not context.active_object.active_material: return

        mat = context.material
        ob = context.object
        slot = context.material_slot
        layout = self.layout
        #layout.label(text=str(self.INDEX))
        if mat.use_material_passes:
            db = layout.box()
            nodes = bpyShaders.get_or_create_material_passes( mat )
            node = nodes[ self.INDEX ]
            split = db.row()
            if node.material: split.prop( node.material, 'use_in_ogre_material_pass', text='' )
            split.prop( node, 'material' )
            if not node.material:
                op = split.operator( 'ogre.helper_create_attach_material_layer', icon="PLUS", text='' )
                op.INDEX = self.INDEX

            if node.material and node.material.use_in_ogre_material_pass:
                dbb = db.box()
                ogre_material_panel( dbb, node.material, parent=mat )
                ogre_material_panel_extra( dbb, node.material )


## operator ##
class _create_new_material_layer_helper(bpy.types.Operator):
    '''helper to create new material layer'''
    bl_idname = "ogre.helper_create_attach_material_layer"
    bl_label = "creates and assigns new material to layer"
    bl_options = {'REGISTER'}
    INDEX = IntProperty(name="material layer index", description="index", default=0, min=0, max=8)
    @classmethod
    def poll(cls, context):
        if context.active_object and context.active_object.active_material and context.active_object.active_material.use_material_passes:
            return True

    def execute(self, context):
        mat = context.active_object.active_material
        nodes = bpyShaders.get_or_create_material_passes( mat )
        node = nodes[ self.INDEX ]
        node.material = bpy.data.materials.new( name='%s.LAYER%s'%(mat.name,self.INDEX) )
        node.material.use_fixed_pipeline = False
        node.material.offset_z = (self.INDEX*2) + 2     # nudge each pass by 2
        return {'FINISHED'}


@UI
class PANEL_properties_window_ogre_material( bpy.types.Panel ):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "material"
    bl_label = "Ogre Material (base pass)"

    @classmethod
    def poll( self, context ):
        if not hasattr(context, "material"): return False
        if not context.active_object: return False
        if not context.active_object.active_material: return False
        return True

    def draw(self, context):
        mat = context.material
        ob = context.object
        slot = context.material_slot
        layout = self.layout
        if not mat.use_material_passes:
            box = layout.box()
            box.operator( 'ogre.force_setup_material_passes', text="Ogre Material Layers", icon='SCENE_DATA' )

        ogre_material_panel( layout, mat )
        ogre_material_panel_extra( layout, mat )

@UI
class MatPass1( _OgreMatPass, bpy.types.Panel ): INDEX = 0; bl_label = "Ogre Material (pass%s)"%str(INDEX+1)
@UI
class MatPass2( _OgreMatPass, bpy.types.Panel ): INDEX = 1; bl_label = "Ogre Material (pass%s)"%str(INDEX+1)
@UI
class MatPass3( _OgreMatPass, bpy.types.Panel ): INDEX = 2; bl_label = "Ogre Material (pass%s)"%str(INDEX+1)
@UI
class MatPass4( _OgreMatPass, bpy.types.Panel ): INDEX = 3; bl_label = "Ogre Material (pass%s)"%str(INDEX+1)
@UI
class MatPass5( _OgreMatPass, bpy.types.Panel ): INDEX = 4; bl_label = "Ogre Material (pass%s)"%str(INDEX+1)
@UI
class MatPass6( _OgreMatPass, bpy.types.Panel ): INDEX = 5; bl_label = "Ogre Material (pass%s)"%str(INDEX+1)
@UI
class MatPass7( _OgreMatPass, bpy.types.Panel ): INDEX = 6; bl_label = "Ogre Material (pass%s)"%str(INDEX+1)
@UI
class MatPass8( _OgreMatPass, bpy.types.Panel ): INDEX = 7; bl_label = "Ogre Material (pass%s)"%str(INDEX+1)



@UI
class PANEL_Textures(bpy.types.Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "texture"
    bl_label = "Ogre Texture"
    @classmethod
    def poll(cls, context):
        if not hasattr(context, "texture_slot"):
            return False
        else: return True

    def draw(self, context):
        #if not hasattr(context, "texture_slot"):
        #    return False
        layout = self.layout
        #idblock = context_tex_datablock(context)
        slot = context.texture_slot
        if not slot or not slot.texture: return


        btype = slot.blend_type     # TODO - fix this hack if/when slots support pyRNA
        ex = False; texop = None
        if btype in TextureUnit.colour_op:
            if btype=='MIX' and slot.use_map_alpha and not slot.use_stencil:
                if slot.diffuse_color_factor >= 1.0: texop = 'alpha_blend'
                else:
                    texop = TextureUnit.colour_op_ex[ btype ]
                    ex = True
            elif btype=='MIX' and slot.use_map_alpha and slot.use_stencil:
                texop = 'blend_current_alpha'; ex=True
            elif btype=='MIX' and not slot.use_map_alpha and slot.use_stencil:
                texop = 'blend_texture_alpha'; ex=True
            else:
                texop = TextureUnit.colour_op[ btype ]
        elif btype in TextureUnit.colour_op_ex:
                texop = TextureUnit.colour_op_ex[ btype ]
                ex = True

        box = layout.box()
        row = box.row()
        if texop:
            if ex: row.prop(slot, "blend_type", text=texop, icon='NEW')
            else: row.prop(slot, "blend_type", text=texop)

        else: row.prop(slot, "blend_type", text='(invalid option)')

        if btype == 'MIX':
            row.prop(slot, "use_stencil", text="")
            row.prop(slot, "use_map_alpha", text="")

            if texop == 'blend_manual':
                row = box.row()
                row.label(text="Alpha:")
                row.prop(slot, "diffuse_color_factor", text="")

        ###########################
        if hasattr(slot.texture, 'image') and slot.texture.image:
            row = box.row()
            n = '(invalid option)'
            if slot.texture.extension in TextureUnit.tex_address_mode:
                n = TextureUnit.tex_address_mode[ slot.texture.extension ]
            row.prop(slot.texture, "extension", text=n)
            if slot.texture.extension == 'CLIP':
                row.prop(slot, "color", text="Border Color")

        row = box.row()
        if slot.texture_coords == 'UV':
            row.prop(slot, "texture_coords", text="", icon='GROUP_UVS')
            row.prop(slot, "uv_layer", text='Layer')
        elif slot.texture_coords == 'REFLECTION':
            row.prop(slot, "texture_coords", text="", icon='MOD_UVPROJECT')
            n = '(invalid option)'
            if slot.mapping in 'FLAT SPHERE'.split(): n = ''
            row.prop(slot, "mapping", text=n)
        else:
            row.prop(slot, "texture_coords", text="(invalid mapping option)")

        ########### animation and offset options ############
        split = layout.row()
        box = split.box()
        box.prop(slot, "offset", text="XY=offset,  Z=rotation")
        box = split.box()
        box.prop(slot, "scale", text="XY=scale (Z ignored)")

        box = layout.box()
        row = box.row()
        row.label(text='scrolling animation')
        #cant use if its enabled by default row.prop(slot, "use_map_density", text="")
        row.prop(slot, "use_map_scatter", text="")
        row = box.row()
        row.prop(slot, "density_factor", text="X")
        row.prop(slot, "emission_factor", text="Y")

        box = layout.box()
        row = box.row()
        row.label(text='rotation animation')
        row.prop(slot, "emission_color_factor", text="")
        row.prop(slot, "use_from_dupli", text="")

        ############# image magick #############
        if hasattr(slot.texture, 'image') and slot.texture.image:
            img = slot.texture.image
            box = layout.box()

            row = box.row()
            row.prop( img, 'use_convert_format' )
            if img.use_convert_format:
                row.prop( img, 'convert_format' )
                if img.convert_format == 'jpg':
                    box.prop( img, 'jpeg_quality' )

            row = box.row()
            row.prop( img, 'use_color_quantize', text='Reduce Colors' )
            if img.use_color_quantize:
                row.prop( img, 'use_color_quantize_dither', text='dither' )
                row.prop( img, 'color_quantize', text='colors' )

            row = box.row()
            row.prop( img, 'use_resize_half' )
            if not img.use_resize_half:
                row.prop( img, 'use_resize_absolute' )
                if img.use_resize_absolute:
                    row = box.row()
                    row.prop( img, 'resize_x' )
                    row.prop( img, 'resize_y' )

###################








########################################
############### OgreMeshy ##############
class OgreMeshyPreviewOp(bpy.types.Operator):
    '''helper to open ogremeshy'''
    bl_idname = 'ogremeshy.preview'
    bl_label = "opens ogremeshy in a subprocess"
    bl_options = {'REGISTER'}
    preview = BoolProperty(name="preview", description="fast preview", default=True)
    groups = BoolProperty(name="preview merge groups", description="use merge groups", default=False)
    mesh = BoolProperty(name="update mesh", description="update mesh (disable for fast material preview", default=True)
    @classmethod
    def poll(cls, context):
        if context.active_object and context.active_object.type in ('MESH','EMPTY') and context.mode != 'EDIT_MESH':
            if context.active_object.type == 'EMPTY' and context.active_object.dupli_type != 'GROUP': return False
            else: return True

    def execute(self, context):
        Report.reset()
        Report.messages.append('running %s' %CONFIG['OGRE_MESHY'])

        if sys.platform.startswith('linux'):    # TODO use native OgreMeshy on linux
            path = '%s/.wine/drive_c/tmp' %os.environ['HOME']
        elif sys.platform.startswith('darwin') or sys.platform.startswith('freebsd'):
            path = '/tmp'
        else:
            path = 'C:\\tmp'

        mat = None
        mgroup = merged = None
        umaterials = []
        
        if context.active_object.type == 'MESH': mat = context.active_object.active_material
        elif context.active_object.type == 'EMPTY':     # assume group
            obs = []
            for e in context.selected_objects:
                if e.type != 'EMPTY' and e.dupli_group: continue
                grp = e.dupli_group
                subs = []
                for o in grp.objects:
                    if o.type=='MESH': subs.append( o )
                if subs:
                    m = merge_objects( subs, transform=e.matrix_world )
                    obs.append( m )
            if obs:
                merged = merge_objects( obs )
                umaterials = dot_mesh( merged, path=path, force_name='preview' )
                for o in obs: context.scene.objects.unlink(o)

        
        if not self.mesh:
            for ob in context.selected_objects:
                if ob.type == 'MESH':
                    for mat in ob.data.materials:
                        if mat and mat not in umaterials: umaterials.append( mat )

        if not merged:
            mgroup = MeshMagick.get_merge_group( context.active_object )
            if not mgroup and self.groups:
                group = get_merge_group( context.active_object )
                if group:
                    print('--------------- has merge group ---------------' )
                    merged = merge_group( group )
                else:
                    print('--------------- NO merge group ---------------' )
            elif len(context.selected_objects)>1 and context.selected_objects:
                merged = merge_objects( context.selected_objects )

            if mgroup:
                for ob in mgroup.objects:
                    nmats = dot_mesh( ob, path=path )
                    for m in nmats:
                        if m not in umaterials: umaterials.append( m )
                MeshMagick.merge( mgroup, path=path, force_name='preview' )
            elif merged:
                umaterials = dot_mesh( merged, path=path, force_name='preview' )
            else:
                umaterials = dot_mesh( context.active_object, path=path, force_name='preview' )

        if mat or umaterials:
            #CONFIG['TOUCH_TEXTURES'] = True
            #CONFIG['PATH'] = path   # TODO deprecate
            data = ''
            for umat in umaterials:
                data += generate_material( umat, path=path, copy_programs=True, touch_textures=True ) # copies shader programs to path
            f=open( os.path.join( path, 'preview.material' ), 'wb' )
            f.write( bytes(data,'utf-8') ); f.close()

        if merged: context.scene.objects.unlink( merged )

        if sys.platform.startswith('linux') or sys.platform.startswith('darwin') or sys.platform.startswith('freebsd'):
            if CONFIG['OGRE_MESHY'].endswith('.exe'):
                cmd = ['wine', CONFIG['OGRE_MESHY'], 'c:\\tmp\\preview.mesh' ]
            else:
                cmd = [CONFIG['OGRE_MESHY'], '/tmp/preview.mesh']
            print( cmd )
            #subprocess.call(cmd)
            subprocess.Popen(cmd)

        else:
            #subprocess.call([CONFIG_OGRE_MESHY, 'C:\\tmp\\preview.mesh'])
            subprocess.Popen( [CONFIG['OGRE_MESHY'], 'C:\\tmp\\preview.mesh'] )

        Report.show()
        return {'FINISHED'}



#############################

_OGRE_DOCS_ = []
def ogredoc( cls ):
    tag = cls.__name__.split('_ogredoc_')[-1]
    cls.bl_label = tag.replace('_', ' ')
    _OGRE_DOCS_.append( cls )
    return cls

############ USED BY DOCS ##########
class INFO_MT_ogre_helper(bpy.types.Menu):
    bl_label = '_overloaded_'
    def draw(self, context):
        layout = self.layout
        #row = self.layout.box().split(percentage=0.05)
        #col = row.column(align=False)
        #print(dir(col))
        #row.scale_x = 0.1
        #row.alignment = 'RIGHT'

        for line in self.mydoc.splitlines():
            if line.strip():
                for ww in wordwrap( line ): layout.label(text=ww)
        layout.separator()


class INFO_MT_ogre_docs(bpy.types.Menu):
    bl_label = "Ogre Help"
    def draw(self, context):
        layout = self.layout
        for cls in _OGRE_DOCS_:
            layout.menu( cls.__name__ )
            layout.separator()
        layout.separator()
        layout.label(text='bug reports to: bhartsho@yahoo.com')

class INFO_MT_ogre_shader_pass_attributes(bpy.types.Menu):
    bl_label = "Shader-Pass"
    def draw(self, context):
        layout = self.layout
        for cls in _OGRE_SHADER_REF_:
            layout.menu( cls.__name__ )

class INFO_MT_ogre_shader_texture_attributes(bpy.types.Menu):
    bl_label = "Shader-Texture"
    def draw(self, context):
        layout = self.layout
        for cls in _OGRE_SHADER_REF_TEX_:
            layout.menu( cls.__name__ )


@ogredoc
class _ogredoc_Installing( INFO_MT_ogre_helper ):
    mydoc = _doc_installing_

@ogredoc
class _ogredoc_FAQ( INFO_MT_ogre_helper ):
    mydoc = _faq_


@ogredoc
class _ogredoc_Animation_System( INFO_MT_ogre_helper ):
    mydoc = '''
Armature Animation System | OgreDotSkeleton
    Quick Start:
        1. select your armature and set a single keyframe on the object (loc,rot, or scl)
            . note, this step is just a hack for creating an action so you can then create an NLA track.
            . do not key in pose mode, unless you want to only export animation on the keyed bones.
        2. open the NLA, and convert the action into an NLA strip
        3. name the NLA strip(s)
        4. set the in and out frames for each strip ( the strip name becomes the Ogre track name )

    How it Works:
        The NLA strips can be blank, they are only used to define Ogre track names, and in and out frame ranges.  You are free to animate the armature with constraints (no baking required), or you can used baked animation and motion capture.  Blending that is driven by the NLA is also supported, if you don't want blending, put space between each strip.

    The OgreDotSkeleton (.skeleton) format supports multiple named tracks that can contain some or all of the bones of an armature.  This feature can be exploited by a game engine for segmenting and animation blending.  For example: lets say we want to animate the upper torso independently of the lower body while still using a single armature.  This can be done by hijacking the NLA of the armature.

    Advanced NLA Hijacking (selected-bones-animation):
        . define an action and keyframe only the bones you want to 'group', ie. key all the upper torso bones
        . import the action into the NLA
        . name the strip (this becomes the track name in Ogre)
        . adjust the start and end frames of each strip
        ( you may use multiple NLA tracks, multiple strips per-track is ok, and strips may overlap in time )

'''


@ogredoc
class _ogredoc_Physics( INFO_MT_ogre_helper ):
    mydoc = '''
Ogre Dot Scene + BGE Physics
    extended format including external collision mesh, and BGE physics settings
<node name="...">
    <entity name="..." meshFile="..." collisionFile="..." collisionPrim="..." [and all BGE physics attributes] />
</node>

collisionFile : sets path to .mesh that is used for collision (ignored if collisionPrim is set)
collisionPrim : sets optimal collision type [ cube, sphere, capsule, cylinder ]
*these collisions are static meshes, animated deforming meshes should give the user a warning that they have chosen a static mesh collision type with an object that has an armature

Blender Collision Setup:
    1. If a mesh object has a child mesh with a name starting with 'collision', then the child becomes the collision mesh for the parent mesh.

    2. If 'Collision Bounds' game option is checked, the bounds type [box, sphere, etc] is used. This will override above rule.

    3. Instances (and instances generated by optimal array modifier) will share the same collision type of the first instance, you DO NOT need to set the collision type for each instance.

'''

@ogredoc
class _ogredoc_Bugs( INFO_MT_ogre_helper ):
    mydoc = '''
Known Issues:
    . shape animation breaks when using modifiers that change the vertex count
        (Any modifier that changes the vertex count is bad with shape anim or armature anim)
    . never rename the nodes created by enabling Ogre-Material-Layers
    . never rename collision proxy meshes created by the Collision Panel
    . lighting in Tundra is not excatly the same as in Blender
Tundra Streaming:
    . only supports streaming transform of up to 10 objects selected objects
    . the 3D view must be shown at the time you open Tundra
    . the same 3D view must be visible to stream data to Tundra
    . only position and scale are updated, a bug on the Tundra side prevents rotation update
    . animation playback is broken if you rename your NLA strips after opening Tundra
'''



############ Ogre v.17 Doc ######

def _mesh_entity_helper( doc, ob, o ):
    ## extended format - BGE Physics ##
    o.setAttribute('mass', str(ob.game.mass))
    o.setAttribute('mass_radius', str(ob.game.radius))
    o.setAttribute('physics_type', ob.game.physics_type)
    o.setAttribute('actor', str(ob.game.use_actor))
    o.setAttribute('ghost', str(ob.game.use_ghost))
    o.setAttribute('velocity_min', str(ob.game.velocity_min))
    o.setAttribute('velocity_max', str(ob.game.velocity_max))

    o.setAttribute('lock_trans_x', str(ob.game.lock_location_x))
    o.setAttribute('lock_trans_y', str(ob.game.lock_location_y))
    o.setAttribute('lock_trans_z', str(ob.game.lock_location_z))

    o.setAttribute('lock_rot_x', str(ob.game.lock_rotation_x))
    o.setAttribute('lock_rot_y', str(ob.game.lock_rotation_y))
    o.setAttribute('lock_rot_z', str(ob.game.lock_rotation_z))

    o.setAttribute('anisotropic_friction', str(ob.game.use_anisotropic_friction))
    x,y,z = ob.game.friction_coefficients
    o.setAttribute('friction_x', str(x))
    o.setAttribute('friction_y', str(y))
    o.setAttribute('friction_z', str(z))

    o.setAttribute('damping_trans', str(ob.game.damping))
    o.setAttribute('damping_rot', str(ob.game.rotation_damping))

    o.setAttribute('inertia_tensor', str(ob.game.form_factor))

    mesh = ob.data
    ## custom user props ##
    for prop in mesh.items():
        propname, propvalue = prop
        if not propname.startswith('_'):
            user = doc.createElement('user_data')
            o.appendChild( user )
            user.setAttribute( 'name', propname )
            user.setAttribute( 'value', str(propvalue) )
            user.setAttribute( 'type', type(propvalue).__name__ )


#class _type(bpy.types.IDPropertyGroup):
#    name = StringProperty(name="jpeg format", description="", maxlen=64, default="")

def get_lights_by_type( T ):
    r = []
    for ob in bpy.context.scene.objects:
        if ob.type=='LAMP':
            if ob.data.type==T: r.append( ob )
    return r

class _TXML_(object):

    '''
  <component type="EC_Script" sync="1" name="myscript">
   <attribute value="" name="Script ref"/>
   <attribute value="false" name="Run on load"/>
   <attribute value="0" name="Run mode"/>
   <attribute value="" name="Script application name"/>
   <attribute value="" name="Script class name"/>
  </component>
    '''


    def create_tundra_document( self, context ):
        proto = 'local://'      # antont says file:// is also valid

        doc = RDocument()
        scn = doc.createElement('scene')
        doc.appendChild( scn )

        if 0:       # Tundra bug
            e = doc.createElement( 'entity' )
            doc.documentElement.appendChild( e )
            e.setAttribute('id', len(doc.documentElement.childNodes)+1 )

            c = doc.createElement( 'component' ); e.appendChild( c )
            c.setAttribute( 'type', 'EC_Script' )
            c.setAttribute( 'sync', '1' )
            c.setAttribute( 'name', 'myscript' )

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Script ref')
            #a.setAttribute('value', "%s%s"%(proto,TUNDRA_GEN_SCRIPT_PATH) )
            
            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Run on load')
            a.setAttribute('value', 'true' )

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Run mode')
            a.setAttribute('value', '0' )

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Script application name')
            a.setAttribute('value', 'blender2ogre' )


        ############### environment ################
        sun = hemi = None
        if get_lights_by_type('SUN'): sun = get_lights_by_type('SUN')[0]
        if get_lights_by_type('HEMI'): hemi = get_lights_by_type('HEMI')[0]
        if bpy.context.scene.world.mist_settings.use_mist or sun or hemi:
            e = doc.createElement( 'entity' )
            doc.documentElement.appendChild( e )
            e.setAttribute('id', len(doc.documentElement.childNodes)+1 )

            c = doc.createElement( 'component' ); e.appendChild( c )
            c.setAttribute( 'type', 'EC_Fog' )
            c.setAttribute( 'sync', '1' )
            c.setAttribute( 'name', 'blender-environment-fog' )

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Color')
            if bpy.context.scene.world.mist_settings.use_mist:
                A = bpy.context.scene.world.mist_settings.intensity
                R,G,B = bpy.context.scene.world.horizon_color
                a.setAttribute('value', '%s %s %s %s'%(R,G,B,A))
            else:
                a.setAttribute('value', '0.4 0.4 0.4 1.0')

            if bpy.context.scene.world.mist_settings.use_mist:
                mist = bpy.context.scene.world.mist_settings

                a = doc.createElement('attribute'); c.appendChild( a )
                a.setAttribute('name', 'Start distance')
                a.setAttribute('value', mist.start)

                a = doc.createElement('attribute'); c.appendChild( a )
                a.setAttribute('name', 'End distance')
                a.setAttribute('value', mist.start+mist.depth)

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Exponential density')
            a.setAttribute('value', 0.001)

            ##############################################
            c = doc.createElement( 'component' ); e.appendChild( c )
            c.setAttribute( 'type', 'EC_EnvironmentLight' )
            c.setAttribute( 'sync', '1' )
            c.setAttribute( 'name', 'blender-environment-light' )


            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Sunlight color')
            if sun:
                R,G,B = sun.data.color
                a.setAttribute('value', '%s %s %s 1' %(R,G,B))
            else:
                a.setAttribute('value', '0 0 0 1')

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Brightness')    # brightness of sunlight
            if sun:
                a.setAttribute('value', sun.data.energy*10)     # 10=magic
            else:
                a.setAttribute('value', '0')

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Ambient light color')
            if hemi:
                R,G,B = hemi.data.color * hemi.data.energy * 3.0
                if R>1.0: R=1.0
                if G>1.0: G=1.0
                if B>1.0: B=1.0
                a.setAttribute('value', '%s %s %s 1' %(R,G,B))
            else:
                a.setAttribute('value', '0 0 0 1')

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Sunlight direction vector')
            a.setAttribute('value', '-0.25 -1.0 -0.25')   # TODO, get the sun rotation from blender

            a = doc.createElement('attribute'); c.appendChild( a )
            a.setAttribute('name', 'Sunlight cast shadows')
            a.setAttribute('value', 'true')


        if context.scene.world.ogre_skyX:

            ################# SKYX ################
            c = doc.createElement( 'component' ); e.appendChild( c )
            c.setAttribute( 'type', 'EC_SkyX' )
            c.setAttribute( 'sync', '1' )
            c.setAttribute( 'name', 'myskyx' )

            a = doc.createElement('attribute'); a.setAttribute('name', 'Weather (volumetric clouds only)')
            den = (
                context.scene.world.ogre_skyX_cloud_density_x, 
                context.scene.world.ogre_skyX_cloud_density_y
            )
            a.setAttribute('value', '%s %s' %den)
            c.appendChild( a )

            config = (
                ('time', 'Time multiplier'), 
                ('volumetric_clouds','Volumetric clouds'), 
                ('wind','Wind direction'),
            )
            for bname, aname in config:
                a = doc.createElement('attribute')
                a.setAttribute('name', aname)
                s = str( getattr(context.scene.world, 'ogre_skyX_'+bname) )
                a.setAttribute('value', s.lower())
                c.appendChild( a )

        return doc

    ########################################
    def tundra_entity( self, doc, ob, path='/tmp', collision_proxies=[] ):
        assert not ob.subcollision
        # txml has flat hierarchy
        e = doc.createElement( 'entity' )
        doc.documentElement.appendChild( e )
        e.setAttribute('id', uid(ob) )

        c = doc.createElement('component'); e.appendChild( c )
        c.setAttribute('type', "EC_Name")
        c.setAttribute('sync', '1')
        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "name" )
        a.setAttribute('value', ob.name )
        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "description" )
        a.setAttribute('value', "" )


        ############ Tundra TRANSFORM ####################
        c = doc.createElement('component'); e.appendChild( c )
        c.setAttribute('type', "EC_Placeable")
        c.setAttribute('sync', '1')
        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "Transform" )
        x,y,z = swap(ob.matrix_world.to_translation())
        loc = '%6f,%6f,%6f' %(x,y,z)
        x,y,z = swap(ob.matrix_world.to_euler())
        x = math.degrees( x ); y = math.degrees( y ); z = math.degrees( z )
        if ob.type == 'CAMERA': x -= 90
        elif ob.type == 'LAMP': x += 90
        rot = '%6f,%6f,%6f' %(x,y,z)
        x,y,z = swap(ob.matrix_world.to_scale())
        scl = '%6f,%6f,%6f' %(abs(x),abs(y),abs(z))		# Tundra2 clamps any negative to zero
        a.setAttribute('value', "%s,%s,%s" %(loc,rot,scl) )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "Show bounding box" )
        if ob.show_bounds or ob.type != 'MESH': a.setAttribute('value', "true" )
        else: a.setAttribute('value', "false" )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "Visible" )
        a.setAttribute('value', 'true')

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "Selection layer" )
        a.setAttribute('value', 1)

        #<attribute value="1" name="Selection layer"/>
        #<attribute value="" name="Parent entity ref"/>
        #<attribute value="" name="Parent bone name"/>

        if ob.type != 'MESH':
            c = doc.createElement('component'); e.appendChild( c )
            c.setAttribute('type', 'EC_TransformGizmo')
            c.setAttribute('sync', '1')

            c = doc.createElement('component'); e.appendChild( c )
            c.setAttribute('type', 'EC_Name')
            c.setAttribute('sync', '1')
            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', "name" )
            a.setAttribute('value', ob.name)


        if ob.type == 'CAMERA':
            c = doc.createElement('component'); e.appendChild( c )
            c.setAttribute('type', 'EC_Camera')
            c.setAttribute('sync', '1')

            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', "Up vector" )
            a.setAttribute('value', '0.0 1.0 0.0')

            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', "Near plane" )
            a.setAttribute('value', '0.01')

            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', "Far plane" )
            a.setAttribute('value', '2000')

            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', "Vertical FOV" )
            a.setAttribute('value', '45')

            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', "Aspect ratio" )
            a.setAttribute('value', '')


        ## any object can have physics ##
        #if ob.game.physics_type == 'RIGID_BODY':
        #if not ob.game.use_ghost and ob.game.physics_type in 'RIGID_BODY SENSOR'.split():
        NTF = None

        if ob.physics_mode != 'NONE' or ob.collision_mode != 'NONE':
            TundraTypes = {
                'BOX' : 0,
                'SPHERE' : 1,
                'CYLINDER' : 2,
                'CONE' : 0,    # tundra is missing
                'CAPSULE' : 3,
                'TRIANGLE_MESH' : 4,
                #'HEIGHT_FIELD': 5, #blender is missing
                'CONVEX_HULL' : 6
            }


            com = doc.createElement('component'); e.appendChild( com )
            com.setAttribute('type', 'EC_RigidBody')
            com.setAttribute('sync', '1')

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Mass')
            if ob.physics_mode == 'RIGID_BODY':
                a.setAttribute('value', ob.game.mass)
            else:
                a.setAttribute('value', '0.0')  # disables physics in Tundra?


            SHAPE = a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Shape type')
            a.setAttribute('value', TundraTypes[ ob.game.collision_bounds_type ] )

            M = ob.game.collision_margin
            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Size')
            if ob.game.collision_bounds_type in 'TRIANGLE_MESH CONVEX_HULL'.split():
                a.setAttribute('value', '%s %s %s' %(1.0+M, 1.0+M, 1.0+M) )
            else:
                #x,y,z = swap(ob.matrix_world.to_scale())
                x,y,z = swap(ob.dimensions)
                a.setAttribute('value', '%s %s %s' %(abs(x)+M,abs(y)+M,abs(z)+M) )

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Collision mesh ref')
            #if ob.game.use_collision_compound:
            if ob.collision_mode == 'DECIMATED':
                proxy = None
                for child in ob.children:
                    if child.subcollision and child.name.startswith('DECIMATED'):
                        proxy = child; break
                if proxy:
                    a.setAttribute('value', 'local://_collision_%s.mesh' %proxy.data.name)
                    if proxy not in collision_proxies: collision_proxies.append( proxy )
                else:
                    print( 'WARN: collision proxy mesh not found' )
                    assert 0

            elif ob.collision_mode == 'TERRAIN':
                NTF = save_terrain_as_NTF( path, ob )
                SHAPE.setAttribute( 'value', '5' )  # HEIGHT_FIELD

            elif ob.type == 'MESH':
                a.setAttribute('value', 'local://%s.mesh' %ob.data.name)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Friction')
            #avg = sum( ob.game.friction_coefficients ) / 3.0
            a.setAttribute('value', ob.physics_friction)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Restitution')
            a.setAttribute('value', ob.physics_bounce)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Linear damping')
            a.setAttribute('value', ob.game.damping)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Angular damping')
            a.setAttribute('value', ob.game.rotation_damping)


            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Linear factor')
            a.setAttribute('value', '1.0 1.0 1.0')

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Angular factor')
            a.setAttribute('value', '1.0 1.0 1.0')


            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Kinematic')
            a.setAttribute('value', 'false' )

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Phantom')        # this must mean no-collide
            if ob.collision_mode == 'NONE':
                a.setAttribute('value', 'true' )
            else:
                a.setAttribute('value', 'false' )

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Draw Debug')
            if ob.collision_mode == 'NONE':
                a.setAttribute('value', 'false' )
            else:
                a.setAttribute('value', 'true' )


            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Linear velocity')
            a.setAttribute('value', '0.0 0.0 0.0')

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Angular velocity')
            a.setAttribute('value', '0.0 0.0 0.0')

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Collision Layer')
            a.setAttribute('value', -1)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Collision Mask')
            a.setAttribute('value', -1)

        if NTF:     # Terrain
            xp = NTF['xpatches']
            yp = NTF['ypatches']
            depth = NTF['depth']
            com = doc.createElement('component'); e.appendChild( com )
            com.setAttribute('type', 'EC_Terrain')
            com.setAttribute('sync', '1')

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Transform')
            x,y,z = ob.dimensions
            sx,sy,sz = ob.scale
            x *= 1.0/sx
            y *= 1.0/sy
            z *= 1.0/sz
            #trans = '%s,%s,%s,' %(-xp/4, -z/2, -yp/4)
            trans = '%s,%s,%s,' %(-xp/4, -depth, -yp/4)
            # scaling in Tundra happens after translation
            nx = x/(xp*16)
            ny = y/(yp*16)
            trans += '0,0,0,%s,%s,%s' %(nx,depth, ny)
            a.setAttribute('value', trans )

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Grid Width')
            a.setAttribute('value', xp)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Grid Height')
            a.setAttribute('value', yp)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Tex. U scale')
            a.setAttribute('value', 1.0)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Tex. V scale')
            a.setAttribute('value', 1.0)

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Material')
            a.setAttribute('value', '')

            for i in range(4):
                a = doc.createElement('attribute'); com.appendChild( a )
                a.setAttribute('name', 'Texture %s' %i)
                a.setAttribute('value', '')

            a = doc.createElement('attribute'); com.appendChild( a )
            a.setAttribute('name', 'Heightmap')
            a.setAttribute('value', NTF['name'] )


        return e


    def tundra_mesh( self, e, ob, url, exported_meshes ):
        if self.EX_MESH:
            murl = os.path.join( os.path.split(url)[0], '%s.mesh'%ob.data.name )
            exists = os.path.isfile( murl )
            if not exists or (exists and self.EX_MESH_OVERWRITE):
                if ob.data.name not in exported_meshes:
                    exported_meshes.append( ob.data.name )
                    self.dot_mesh( ob, os.path.split(url)[0] )

        doc = e.document
        proto = 'local://'      # antont says file:// is also valid

        if ob.find_armature():
            c = doc.createElement('component'); e.appendChild( c )
            c.setAttribute('type', "EC_AnimationController")
            c.setAttribute('sync', '1')

        c = doc.createElement('component'); e.appendChild( c )
        c.setAttribute('type', "EC_Mesh")
        c.setAttribute('sync', '1')

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "Mesh ref" )
        a.setAttribute('value', "%s%s.mesh"%(proto,ob.data.name) )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "Mesh materials" )
        # Query object its materials and make a proper material ref string of it.
        # note: We assume blindly here that the 'submesh' indexes are correct in the material list.
        #       the most common usecase is to have one material per object for rex artists.
        #       They can now assign multiple and they will at least go to the .txml data but I cant
        #       guarantee that they are in correct submesh index slots! At least they have the refs and 
        #       can manually shift them around in the viewer.
        mymaterials = ob.data.materials
        if mymaterials is not None and len(mymaterials) > 0:
            mymatstring = ''
            # generate ; separated material list
            for mymat in mymaterials: 
                if mymat is None:
                    continue
                mymatstring += proto + material_name(mymat) + '.material;'
            mymatstring = mymatstring[:-1]  # strip ending ;
            a.setAttribute('value', mymatstring )
        else:
            # default to nothing to avoid error prints in .txml import
            a.setAttribute('value', "" ) 

        if ob.find_armature():
            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', "Skeleton ref" )
            a.setAttribute('value', "%s%s.skeleton"%(proto,ob.data.name) )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "Draw distance" )
        a.setAttribute('value', "0" )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', "Cast shadows" )	# cast shadows is per object? not per material?
        a.setAttribute('value', "false" )

    def tundra_light( self, e, ob ):
        '''
            <component type="EC_Light" sync="1">
            <attribute value="1" name="light type"/>
            <attribute value="1 1 1 1" name="diffuse color"/>
            <attribute value="1 1 1 1" name="specular color"/>
            <attribute value="true" name="cast shadows"/>
            <attribute value="29.9999828" name="light range"/>
            <attribute value="1" name="brightness"/>
            <attribute value="0" name="constant atten"/>
            <attribute value="1" name="linear atten"/>
            <attribute value="0" name="quadratic atten"/>
            <attribute value="30" name="light inner angle"/>
            <attribute value="40" name="light outer angle"/>
            </component>
        '''

        if ob.data.type not in 'POINT SPOT'.split(): return

        doc = e.document

        c = doc.createElement('component'); e.appendChild( c )
        c.setAttribute('type', "EC_Light")
        c.setAttribute('sync', '1')

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'light type' )
        if ob.data.type=='POINT': a.setAttribute('value', '0' )
        elif ob.data.type=='SPOT': a.setAttribute('value', '1' )
        #2 = directional light.  blender has no directional light?

        R,G,B = ob.data.color
        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'diffuse color' )
        if ob.data.use_diffuse: a.setAttribute('value', '%s %s %s 1' %(R,G,B) )
        else: a.setAttribute('value', '0 0 0 1' )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'specular color' )
        if ob.data.use_specular: a.setAttribute('value', '%s %s %s 1' %(R,G,B) )
        else: a.setAttribute('value', '0 0 0 1' )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'cast shadows' )
        if ob.data.type=='HEMI': a.setAttribute('value', 'false' ) # HEMI no .shadow_method
        elif ob.data.shadow_method != 'NOSHADOW': a.setAttribute('value', 'true' )
        else: a.setAttribute('value', 'false' )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'light range' )
        a.setAttribute('value', ob.data.distance*2 )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'brightness' )
        a.setAttribute('value', ob.data.energy )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'constant atten' )
        a.setAttribute('value', '0' )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'linear atten' )
        energy = ob.data.energy
        if energy <= 0.0: energy = 0.001
        a.setAttribute('value', (1.0/energy)*0.25 )

        a = doc.createElement('attribute'); c.appendChild(a)
        a.setAttribute('name', 'quadratic atten' )
        a.setAttribute('value', '0.0' )

        if ob.data.type=='SPOT':
            outer = math.degrees(ob.data.spot_size) / 2.0
            inner = outer * (1.0-ob.data.spot_blend)

            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', 'light inner angle' )
            a.setAttribute('value', '%s'%inner )

            a = doc.createElement('attribute'); c.appendChild(a)
            a.setAttribute('name', 'light outer angle' )
            a.setAttribute('value', '%s' %outer )

class _OgreCommonExport_( _TXML_ ):
    #EXPORT_TYPE = 'OGRE'   # defined in subclass
    #@classmethod
    #def poll(cls, context): return True
    @classmethod
    def poll(cls, context):
        if context.active_object and context.mode != 'EDIT_MESH': return True

    def invoke(self, context, event):
        wm = context.window_manager
        fs = wm.fileselect_add(self)        # writes to filepath
        return {'RUNNING_MODAL'}
    def execute(self, context): self.ogre_export(  self.filepath, context ); return {'FINISHED'}

    EX_SWAP_AXIS = EnumProperty( 
        items=AXIS_MODES, 
        name='swap axis',  
        description='axis swapping mode', 
        default= CONFIG['SWAP_AXIS'] 
    )
    EX_SEP_MATS = BoolProperty(
        name="Separate Materials", 
        description="exports a .material for each material (rather than putting all materials in a single .material file)", 
        default=CONFIG['SEP_MATS'])
    EX_ONLY_ANIMATED_BONES = BoolProperty(
        name="Only Animated Bones", 
        description="only exports bones that have been keyframed, useful for run-time animation blending (example: upper/lower torso split)", 
        default=CONFIG['ONLY_ANIMATED_BONES'])
    EX_SCENE = BoolProperty(name="Export Scene", description="export current scene (OgreDotScene xml)", default=CONFIG['SCENE'])
    EX_SELONLY = BoolProperty(name="Export Selected Only", description="export selected", default=CONFIG['SELONLY'])
    EX_FORCE_CAMERA = BoolProperty(name="Force Camera", description="export active camera", default=CONFIG['FORCE_CAMERA'])
    EX_FORCE_LAMPS = BoolProperty(name="Force Lamps", description="export all lamps", default=CONFIG['FORCE_LAMPS'])
    EX_MESH = BoolProperty(name="Export Meshes", description="export meshes", default=CONFIG['MESH'])
    EX_MESH_OVERWRITE = BoolProperty(name="Export Meshes (overwrite)", description="export meshes (overwrite existing files)", default=CONFIG['MESH_OVERWRITE'])
    EX_ARM_ANIM = BoolProperty(name="Armature Animation", description="export armature animations - updates the .skeleton file", default=CONFIG['ARM_ANIM'])
    EX_SHAPE_ANIM = BoolProperty(name="Shape Animation", description="export shape animations - updates the .mesh file", default=CONFIG['SHAPE_ANIM'])
    EX_TRIM_BONE_WEIGHTS = FloatProperty(
        name="Trim Weights", 
        description="ignore bone weights below this value (Ogre supports 4 bones per vertex)", 
        min=0.0, max=0.5, default=CONFIG['TRIM_BONE_WEIGHTS'] )
    EX_ARRAY = BoolProperty(name="Optimize Arrays", description="optimize array modifiers as instances (constant offset only)", default=CONFIG['ARRAY'])
    EX_MATERIALS = BoolProperty(name="Export Materials", description="exports .material script", default=CONFIG['MATERIALS'])
    EX_FORCE_IMAGE_FORMAT = EnumProperty( items=_IMAGE_FORMATS, name='Convert Images',  description='convert all textures to format', default=CONFIG['FORCE_IMAGE_FORMAT'] )
    EX_DDS_MIPS = IntProperty(name="DDS Mips", description="number of mip maps (DDS)", min=0, max=16, default=CONFIG['DDS_MIPS'])

    ## Mesh Options ##
    EX_lodLevels = IntProperty(name="LOD Levels", description="MESH number of LOD levels", min=0, max=32, default=CONFIG['lodLevels'])
    EX_lodDistance = IntProperty(name="LOD Distance", description="MESH distance increment to reduce LOD", min=0, max=2000, default=CONFIG['lodDistance'])
    EX_lodPercent = IntProperty(name="LOD Percentage", description="LOD percentage reduction", min=0, max=99, default=CONFIG['lodPercent'])
    EX_nuextremityPoints = IntProperty(name="Extremity Points", description="MESH Extremity Points", min=0, max=65536, default=CONFIG['nuextremityPoints'])
    EX_generateEdgeLists = BoolProperty(name="Edge Lists", description="MESH generate edge lists (for stencil shadows)", default=CONFIG['generateEdgeLists'])
    EX_generateTangents = BoolProperty(name="Tangents", description="MESH generate tangents", default=CONFIG['generateTangents'])
    EX_tangentSemantic = StringProperty(name="Tangent Semantic", description="MESH tangent semantic", maxlen=3, default=CONFIG['tangentSemantic'])
    EX_tangentUseParity = IntProperty(name="Tangent Parity", description="MESH tangent use parity", min=0, max=16, default=CONFIG['tangentUseParity'])
    EX_tangentSplitMirrored = BoolProperty(name="Tangent Split Mirrored", description="MESH split mirrored tangents", default=CONFIG['tangentSplitMirrored'])
    EX_tangentSplitRotated = BoolProperty(name="Tangent Split Rotated", description="MESH split rotated tangents", default=CONFIG['tangentSplitRotated'])
    EX_reorganiseBuffers = BoolProperty(name="Reorganise Buffers", description="MESH reorganise vertex buffers", default=CONFIG['reorganiseBuffers'])
    EX_optimiseAnimations = BoolProperty(name="Optimize Animations", description="MESH optimize animations", default=CONFIG['optimiseAnimations'])

    EX_COPY_SHADER_PROGRAMS = BoolProperty(
        name="copy shader programs", 
        description="when using script inheritance copy the source shader programs to the output path", 
        default=CONFIG['COPY_SHADER_PROGRAMS'])

    filepath = StringProperty(name="File Path", description="Filepath used for exporting file", maxlen=1024, default="", subtype='FILE_PATH')

    def dot_material( self, meshes, path='/tmp', mat_file_name='SceneMaterial'):
        material_files = []
        mats = []
        for ob in meshes:
            if len(ob.data.materials):
                for mat in ob.data.materials:
                    if mat not in mats: mats.append( mat )

        if not mats:
            print('WARNING: no materials, not writting .material script'); return []

        M = MISSING_MATERIAL + '\n'
        for mat in mats:
            if mat is None: continue
            Report.materials.append( material_name(mat) )
            if CONFIG['COPY_SHADER_PROGRAMS']:
                data = generate_material( mat, path=path, copy_programs=True, touch_textures=CONFIG['TOUCH_TEXTURES'] )
            else:
                data = generate_material( mat, path=path, touch_textures=CONFIG['TOUCH_TEXTURES'] )

            M += data
            if self.EX_SEP_MATS:
                url = self.dot_material_write_separate( mat, data, path )
                material_files.append( url )

        if not self.EX_SEP_MATS:
            url = os.path.join(path, '%s.material' % mat_file_name)
            f = open( url, 'wb' ); f.write( bytes(M,'utf-8') ); f.close()
            print('saved', url)
            material_files.append( url )

        return material_files

    def dot_material_write_separate( self, mat, data, path = '/tmp' ):      # thanks Pforce
        url = os.path.join(path, '%s.material' % material_name(mat))
        f = open(url, 'wb'); f.write( bytes(data,'utf-8') ); f.close()
        print('saved', url)
        return url


    def dot_mesh( self, ob, path='/tmp', force_name=None, ignore_shape_animation=False ):
        dot_mesh( ob, path, force_name, ignore_shape_animation=False )

    def ogre_export(self, url, context, force_material_update=[] ):
        global CONFIG
        for name in dir(self):
            if name.startswith('EX_'):
                CONFIG[ name[3:] ] = getattr(self,name)

        Report.reset()

        print('ogre export->', url)
        prefix = url.split('.')[0]
        path = os.path.split(url)[0]

        ## nodes (objects) ##
        objects = []        # gather because macros will change selection state
        linkedgroups = []
        for ob in bpy.context.scene.objects:
            if ob.subcollision: continue
            if self.EX_SELONLY and not ob.select:
                if ob.type == 'CAMERA' and self.EX_FORCE_CAMERA: pass
                elif ob.type == 'LAMP' and self.EX_FORCE_LAMPS: pass
                else: continue
            if ob.type == 'EMPTY' and ob.dupli_group and ob.dupli_type == 'GROUP': 
                linkedgroups.append( ob )
            else: objects.append( ob )

        ######## LINKED GROUPS - allows 3 levels of nested blender library linking ########
        temps = []
        for e in linkedgroups:
            grp = e.dupli_group
            subs = []
            for o in grp.objects:
                if o.type=='MESH': subs.append( o )     # TOP-LEVEL

                elif o.type == 'EMPTY' and o.dupli_group and o.dupli_type == 'GROUP':
                    ss = []     # LEVEL2
                    for oo in o.dupli_group.objects:
                        if oo.type=='MESH': ss.append( oo )

                        elif oo.type == 'EMPTY' and oo.dupli_group and oo.dupli_type == 'GROUP':
                            sss = []    # LEVEL3
                            for ooo in oo.dupli_group.objects:
                                if ooo.type=='MESH': sss.append( ooo )
                            if sss:
                                m = merge_objects( sss, name=oo.name, transform=oo.matrix_world )
                                subs.append( m )
                                temps.append( m )
                    if ss:
                        m = merge_objects( ss, name=o.name, transform=o.matrix_world )
                        subs.append( m )
                        temps.append( m )


            if subs:
                m = merge_objects( subs, name=e.name, transform=e.matrix_world )
                objects.append( m )
                temps.append( m )


        ## find merge groups
        mgroups = []
        mobjects = []
        for ob in objects:
            group = get_merge_group( ob )
            if group:
                for member in group.objects:
                    if member not in mobjects: mobjects.append( member )
                if group not in mgroups: mgroups.append( group )
        for rem in mobjects:
            if rem in objects: objects.remove( rem )

        for group in mgroups:
            merged = merge_group( group )
            objects.append( merged )
            temps.append( merged )

        ## gather roots because ogredotscene supports parents and children ##
        def _flatten( _c, _f ):
            if _c.parent in objects: _f.append( _c.parent )
            if _c.parent: _flatten( _c.parent, _f )
            else: _f.append( _c )

        roots = []
        meshes = []

        for ob in objects:
            flat = []
            _flatten( ob, flat )
            root = flat[-1]
            if root not in roots: roots.append( root )

            if ob.type=='MESH': meshes.append( ob )

        mesh_collision_prims = {}
        mesh_collision_files = {}

        exported_meshes = []        # don't export same data multiple times

        if self.EX_MATERIALS:
            material_file_name_base=os.path.split(url)[1].replace('.scene', '').replace('.txml', '')
            material_files = self.dot_material( 
                meshes + force_material_update, 
                path, 
                material_file_name_base
            )
        else:
            material_files = []


        if self.EXPORT_TYPE == 'REX':
            ################# TUNDRA #################
            rex = self.create_tundra_document( context )
            ##########################################
            proxies = []
            for ob in objects:
                TE = self.tundra_entity( rex, ob, path=path, collision_proxies=proxies )
                if ob.type == 'MESH' and len(ob.data.faces):
                    self.tundra_mesh( TE, ob, url, exported_meshes )
                    #meshes.append( ob )
                elif ob.type == 'LAMP':
                    self.tundra_light( TE, ob )


            for proxy in proxies:
                self.dot_mesh( 
                    proxy, 
                    path=os.path.split(url)[0], 
                    force_name='_collision_%s' %proxy.data.name
                )


            if self.EX_SCENE:
                if not url.endswith('.txml'): url += '.txml'
                data = rex.toprettyxml()
                f = open( url, 'wb' ); f.write( bytes(data,'utf-8') ); f.close()
                print('realxtend scene dumped', url)


        elif self.EXPORT_TYPE == 'OGRE':       # ogre-dot-scene
            ############# OgreDotScene ###############
            doc = self.create_ogre_document( context, material_files )
            ##########################################


            for root in roots:
                print('--------------- exporting root ->', root )
                self._node_export( 
                    root, 
                    url=url,
                    doc = doc,
                    exported_meshes = exported_meshes, 
                    meshes = meshes,
                    mesh_collision_prims = mesh_collision_prims,
                    mesh_collision_files = mesh_collision_files,
                    prefix = prefix,
                    objects=objects, 
                    xmlparent=doc._scene_nodes 
                )


            if self.EX_SCENE:
                if not url.endswith('.scene'): url += '.scene'
                data = doc.toprettyxml()
                f = open( url, 'wb' ); f.write( bytes(data,'utf-8') ); f.close()
                print('ogre scene dumped', url)

        for ob in temps: context.scene.objects.unlink( ob )
        #bpy.ops.wm.call_menu( name='MiniReport' )

        save_config()   # always save?

    def create_ogre_document(self, context, material_files=[] ):
        now = time.time()
        doc = RDocument()
        scn = doc.createElement('scene'); doc.appendChild( scn )
        scn.setAttribute('export_time', str(now))
        scn.setAttribute('formatVersion', '1.0.1')
        bscn = bpy.context.scene
        if '_previous_export_time_' in bscn.keys(): scn.setAttribute('previous_export_time', str(bscn['_previous_export_time_']))
        else: scn.setAttribute('previous_export_time', '0')
        bscn[ '_previous_export_time_' ] = now
        scn.setAttribute('exported_by', getpass.getuser())

        nodes = doc.createElement('nodes')
        doc._scene_nodes = nodes
        extern = doc.createElement('externals')
        environ = doc.createElement('environment')
        for n in (nodes,extern,environ): scn.appendChild( n )
        ############################

        ## extern files ##
        for url in material_files:
            item = doc.createElement('item'); extern.appendChild( item )
            item.setAttribute('type','material')
            a = doc.createElement('file'); item.appendChild( a )
            a.setAttribute('name', url)


        ## environ settings ##
        world = context.scene.world
        if world:   # multiple scenes - other scenes may not have a world
            _c = {'colourAmbient':world.ambient_color, 'colourBackground':world.horizon_color, 'colourDiffuse':world.horizon_color}
            for ctag in _c:
                a = doc.createElement(ctag); environ.appendChild( a )
                color = _c[ctag]
                a.setAttribute('r', '%s'%color.r)
                a.setAttribute('g', '%s'%color.g)
                a.setAttribute('b', '%s'%color.b)

        if world and world.mist_settings.use_mist:
            a = doc.createElement('fog'); environ.appendChild( a )
            a.setAttribute('linearStart', '%s'%world.mist_settings.start )
            mist_falloff = world.mist_settings.falloff
            if mist_falloff == 'QUADRATIC': a.setAttribute('mode', 'exp')	# on DTD spec (none | exp | exp2 | linear)
            elif mist_falloff == 'LINEAR': a.setAttribute('mode', 'linear')
            else: a.setAttribute('mode', 'exp2')
            #a.setAttribute('mode', world.mist_settings.falloff.lower() )	# not on DTD spec
            a.setAttribute('linearEnd', '%s' %(world.mist_settings.start+world.mist_settings.depth))
            a.setAttribute('expDensity', world.mist_settings.intensity)
            a.setAttribute('colourR', world.horizon_color.r)
            a.setAttribute('colourG', world.horizon_color.g)
            a.setAttribute('colourB', world.horizon_color.b)

        return doc

    ############# node export - recursive ###############
    def _node_export( self, ob, url='', doc=None, rex=None, exported_meshes=[], meshes=[], mesh_collision_prims={}, mesh_collision_files={}, prefix='', objects=[], xmlparent=None ):

        o = _ogre_node_helper( doc, ob, objects )
        xmlparent.appendChild(o)

        ## custom user props ##
        for prop in ob.items():
            propname, propvalue = prop
            if not propname.startswith('_'):
                user = doc.createElement('user_data')
                o.appendChild( user )
                user.setAttribute( 'name', propname )
                user.setAttribute( 'value', str(propvalue) )
                user.setAttribute( 'type', type(propvalue).__name__ )

        ## BGE subset ##
        game = doc.createElement('game')
        o.appendChild( game )
        sens = doc.createElement('sensors')
        game.appendChild( sens )
        acts = doc.createElement('actuators')
        game.appendChild( acts )
        for sen in ob.game.sensors:
            sens.appendChild( WrapSensor(sen).xml(doc) )
        for act in ob.game.actuators:
            acts.appendChild( WrapActuator(act).xml(doc) )

        if ob.type == 'MESH' and len(ob.data.faces):

            collisionFile = None
            collisionPrim = None
            if ob.data.name in mesh_collision_prims: collisionPrim = mesh_collision_prims[ ob.data.name ]
            if ob.data.name in mesh_collision_files: collisionFile = mesh_collision_files[ ob.data.name ]

            #meshes.append( ob )
            e = doc.createElement('entity') 
            o.appendChild(e); e.setAttribute('name', ob.data.name)
            prefix = ''
            #if self.EX_MESH_SUBDIR: prefix = 'meshes/'
            e.setAttribute('meshFile', '%s%s.mesh' %(prefix,ob.data.name) )

            if not collisionPrim and not collisionFile:
                if ob.game.use_collision_bounds:
                    collisionPrim = ob.game.collision_bounds_type.lower()
                    mesh_collision_prims[ ob.data.name ] = collisionPrim
                else:
                    for child in ob.children:
                        if child.subcollision and child.name.startswith('DECIMATE'):
                            collisionFile = '%s_collision_%s.mesh' %(prefix,ob.data.name)
                            break
                    if collisionFile:
                        mesh_collision_files[ ob.data.name ] = collisionFile
                        self.dot_mesh( 
                            child, 
                            path=os.path.split(url)[0], 
                            force_name='_collision_%s' %ob.data.name
                        )

            if collisionPrim: e.setAttribute('collisionPrim', collisionPrim )
            elif collisionFile: e.setAttribute('collisionFile', collisionFile )

            _mesh_entity_helper( doc, ob, e )

            if self.EX_MESH:
                murl = os.path.join( os.path.split(url)[0], '%s.mesh'%ob.data.name )
                exists = os.path.isfile( murl )
                if not exists or (exists and self.EX_MESH_OVERWRITE):
                    if ob.data.name not in exported_meshes:
                        exported_meshes.append( ob.data.name )
                        self.dot_mesh( ob, os.path.split(url)[0] )

            ## deal with Array mod ##
            vecs = [ ob.matrix_world.to_translation() ]
            for mod in ob.modifiers:
                if mod.type == 'ARRAY':
                    if mod.fit_type != 'FIXED_COUNT':
                        print( 'WARNING: unsupport array-modifier type->', mod.fit_type )
                        continue
                    if not mod.use_constant_offset:
                        print( 'WARNING: unsupport array-modifier mode, must be "constant offset" type' )
                        continue

                    else:
                        #v = ob.matrix_world.to_translation()

                        newvecs = []
                        for prev in vecs:
                            for i in range( mod.count-1 ):
                                v = prev + mod.constant_offset_displace
                                newvecs.append( v )
                                ao = _ogre_node_helper( doc, ob, objects, prefix='_array_%s_'%len(vecs+newvecs), pos=v )
                                xmlparent.appendChild(ao)

                                e = doc.createElement('entity') 
                                ao.appendChild(e); e.setAttribute('name', ob.data.name)
                                #if self.EX_MESH_SUBDIR: e.setAttribute('meshFile', 'meshes/%s.mesh' %ob.data.name)
                                #else:
                                e.setAttribute('meshFile', '%s.mesh' %ob.data.name)

                                if collisionPrim: e.setAttribute('collisionPrim', collisionPrim )
                                elif collisionFile: e.setAttribute('collisionFile', collisionFile )

                        vecs += newvecs


        elif ob.type == 'CAMERA':
            Report.cameras.append( ob.name )
            c = doc.createElement('camera')
            o.appendChild(c); c.setAttribute('name', ob.data.name)
            aspx = bpy.context.scene.render.pixel_aspect_x
            aspy = bpy.context.scene.render.pixel_aspect_y
            sx = bpy.context.scene.render.resolution_x
            sy = bpy.context.scene.render.resolution_y
            fovY = 0.0
            if (sx*aspx > sy*aspy):
                fovY = 2*math.atan(sy*aspy*16.0/(ob.data.lens*sx*aspx))
            else:
                fovY = 2*math.atan(16.0/ob.data.lens)
            # fov in degree
            fov = fovY*180.0/math.pi
            c.setAttribute('fov', '%s'%fov)
            c.setAttribute('projectionType', "perspective")
            a = doc.createElement('clipping'); c.appendChild( a )
            a.setAttribute('nearPlaneDist', '%s' %ob.data.clip_start)
            a.setAttribute('farPlaneDist', '%s' %ob.data.clip_end)


        elif ob.type == 'LAMP' and ob.data.type in 'POINT SPOT SUN'.split():

            Report.lights.append( ob.name )
            l = doc.createElement('light')
            o.appendChild(l)

            mat = get_parent_matrix(ob, objects).inverted() * ob.matrix_world

            p = doc.createElement('position')   # just to make sure we conform with the DTD
            l.appendChild(p)
            v = swap( ob.matrix_world.to_translation() )
            p.setAttribute('x', '%6f'%v.x)
            p.setAttribute('y', '%6f'%v.y)
            p.setAttribute('z', '%6f'%v.z)

            if ob.data.type == 'POINT': l.setAttribute('type', 'point')
            elif ob.data.type == 'SPOT': l.setAttribute('type', 'spot')
            elif ob.data.type == 'SUN': l.setAttribute('type', 'directional')

            l.setAttribute('name', ob.name )
            l.setAttribute('powerScale', str(ob.data.energy))

            a = doc.createElement('lightAttenuation'); l.appendChild( a )
            a.setAttribute('range', '5000' )            # is this an Ogre constant?
            a.setAttribute('constant', '1.0')        # TODO support quadratic light
            a.setAttribute('linear', '%s'%(1.0/ob.data.distance))
            a.setAttribute('quadratic', '0.0')


            if ob.data.type in ('SPOT', 'SUN'):
                vector = swap(mathutils.Euler.to_matrix(ob.rotation_euler)[2])
                a = doc.createElement('direction')
                l.appendChild(a)
                a.setAttribute('x',str(round(-vector[0],3)))
                a.setAttribute('y',str(round(-vector[1],3)))
                a.setAttribute('z',str(round(-vector[2],3)))

            if ob.data.type == 'SPOT':
                a = doc.createElement('spotLightRange')
                l.appendChild(a)
                a.setAttribute('inner',str( ob.data.spot_size*(1.0-ob.data.spot_blend) ))
                a.setAttribute('outer',str(ob.data.spot_size))
                a.setAttribute('falloff','1.0')

            if ob.data.use_diffuse:
                a = doc.createElement('colourDiffuse'); l.appendChild( a )
                a.setAttribute('r', '%s'%ob.data.color.r)
                a.setAttribute('g', '%s'%ob.data.color.g)
                a.setAttribute('b', '%s'%ob.data.color.b)

            if ob.data.use_specular:
                a = doc.createElement('colourSpecular'); l.appendChild( a )
                a.setAttribute('r', '%s'%ob.data.color.r)
                a.setAttribute('g', '%s'%ob.data.color.g)
                a.setAttribute('b', '%s'%ob.data.color.b)

            if ob.data.type != 'HEMI':  # colourShadow is extra, not part of Ogre DTD
                if ob.data.shadow_method != 'NOSHADOW': # Hemi light has no shadow_method
                    a = doc.createElement('colourShadow');l.appendChild( a )
                    a.setAttribute('r', '%s'%ob.data.color.r)
                    a.setAttribute('g', '%s'%ob.data.color.g)
                    a.setAttribute('b', '%s'%ob.data.color.b)
                    l.setAttribute('shadow','true')

        for child in ob.children:
            self._node_export( child, 
                url = url, doc = doc, rex = rex, 
                exported_meshes = exported_meshes, 
                meshes = meshes,
                mesh_collision_prims = mesh_collision_prims,
                mesh_collision_files = mesh_collision_files,
                prefix = prefix,
                objects=objects, 
                xmlparent=o
            )
        ## end _node_export




################################################################

class INFO_OT_createOgreExport(bpy.types.Operator, _OgreCommonExport_):
    '''Export Ogre Scene'''
    bl_idname = "ogre.export"
    bl_label = "Export Ogre"
    bl_options = {'REGISTER'}

    EX_SWAP_AXIS = EnumProperty( 
        items=AXIS_MODES, 
        name='swap axis',  
        description='axis swapping mode', 
        default= CONFIG['SWAP_AXIS'] 
    )
    EX_SEP_MATS = BoolProperty(
        name="Separate Materials", 
        description="exports a .material for each material (rather than putting all materials in a single .material file)", 
        default=CONFIG['SEP_MATS'])
    EX_ONLY_ANIMATED_BONES = BoolProperty(
        name="Only Animated Bones", 
        description="only exports bones that have been keyframed, useful for run-time animation blending (example: upper/lower torso split)", 
        default=CONFIG['ONLY_ANIMATED_BONES'])
    EX_SCENE = BoolProperty(name="Export Scene", description="export current scene (OgreDotScene xml)", default=CONFIG['SCENE'])
    EX_SELONLY = BoolProperty(name="Export Selected Only", description="export selected", default=CONFIG['SELONLY'])
    EX_FORCE_CAMERA = BoolProperty(name="Force Camera", description="export active camera", default=CONFIG['FORCE_CAMERA'])
    EX_FORCE_LAMPS = BoolProperty(name="Force Lamps", description="export all lamps", default=CONFIG['FORCE_LAMPS'])
    EX_MESH = BoolProperty(name="Export Meshes", description="export meshes", default=CONFIG['MESH'])
    EX_MESH_OVERWRITE = BoolProperty(name="Export Meshes (overwrite)", description="export meshes (overwrite existing files)", default=CONFIG['MESH_OVERWRITE'])
    EX_ARM_ANIM = BoolProperty(name="Armature Animation", description="export armature animations - updates the .skeleton file", default=CONFIG['ARM_ANIM'])
    EX_SHAPE_ANIM = BoolProperty(name="Shape Animation", description="export shape animations - updates the .mesh file", default=CONFIG['SHAPE_ANIM'])
    EX_TRIM_BONE_WEIGHTS = FloatProperty(
        name="Trim Weights", 
        description="ignore bone weights below this value (Ogre supports 4 bones per vertex)", 
        min=0.0, max=0.5, default=CONFIG['TRIM_BONE_WEIGHTS'] )
    EX_ARRAY = BoolProperty(name="Optimize Arrays", description="optimize array modifiers as instances (constant offset only)", default=CONFIG['ARRAY'])
    EX_MATERIALS = BoolProperty(name="Export Materials", description="exports .material script", default=CONFIG['MATERIALS'])
    EX_FORCE_IMAGE_FORMAT = EnumProperty( items=_IMAGE_FORMATS, name='Convert Images',  description='convert all textures to format', default=CONFIG['FORCE_IMAGE_FORMAT'] )
    EX_DDS_MIPS = IntProperty(name="DDS Mips", description="number of mip maps (DDS)", min=0, max=16, default=CONFIG['DDS_MIPS'])

    ## Mesh Options ##
    EX_lodLevels = IntProperty(name="LOD Levels", description="MESH number of LOD levels", min=0, max=32, default=CONFIG['lodLevels'])
    EX_lodDistance = IntProperty(name="LOD Distance", description="MESH distance increment to reduce LOD", min=0, max=2000, default=CONFIG['lodDistance'])
    EX_lodPercent = IntProperty(name="LOD Percentage", description="LOD percentage reduction", min=0, max=99, default=CONFIG['lodPercent'])
    EX_nuextremityPoints = IntProperty(name="Extremity Points", description="MESH Extremity Points", min=0, max=65536, default=CONFIG['nuextremityPoints'])
    EX_generateEdgeLists = BoolProperty(name="Edge Lists", description="MESH generate edge lists (for stencil shadows)", default=CONFIG['generateEdgeLists'])
    EX_generateTangents = BoolProperty(name="Tangents", description="MESH generate tangents", default=CONFIG['generateTangents'])
    EX_tangentSemantic = StringProperty(name="Tangent Semantic", description="MESH tangent semantic", maxlen=3, default=CONFIG['tangentSemantic'])
    EX_tangentUseParity = IntProperty(name="Tangent Parity", description="MESH tangent use parity", min=0, max=16, default=CONFIG['tangentUseParity'])
    EX_tangentSplitMirrored = BoolProperty(name="Tangent Split Mirrored", description="MESH split mirrored tangents", default=CONFIG['tangentSplitMirrored'])
    EX_tangentSplitRotated = BoolProperty(name="Tangent Split Rotated", description="MESH split rotated tangents", default=CONFIG['tangentSplitRotated'])
    EX_reorganiseBuffers = BoolProperty(name="Reorganise Buffers", description="MESH reorganise vertex buffers", default=CONFIG['reorganiseBuffers'])
    EX_optimiseAnimations = BoolProperty(name="Optimize Animations", description="MESH optimize animations", default=CONFIG['optimiseAnimations'])

    filepath= StringProperty(name="File Path", description="Filepath used for exporting Ogre .scene file", maxlen=1024, default="", subtype='FILE_PATH')
    EXPORT_TYPE = 'OGRE'



class INFO_OT_createRealxtendExport( bpy.types.Operator, _OgreCommonExport_):
    '''Export RealXtend Scene'''
    bl_idname = "ogre.export_realxtend"
    bl_label = "Export RealXtend"
    bl_options = {'REGISTER', 'UNDO'}

    EX_SWAP_AXIS = EnumProperty( 
        items=AXIS_MODES, 
        name='swap axis',  
        description='axis swapping mode', 
        default= CONFIG['SWAP_AXIS'] 
    )
    EX_SEP_MATS = BoolProperty(
        name="Separate Materials", 
        description="exports a .material for each material (rather than putting all materials in a single .material file)", 
        default=CONFIG['SEP_MATS'])
    EX_ONLY_ANIMATED_BONES = BoolProperty(
        name="Only Animated Bones", 
        description="only exports bones that have been keyframed, useful for run-time animation blending (example: upper/lower torso split)", 
        default=CONFIG['ONLY_ANIMATED_BONES'])

    EX_SCENE = BoolProperty(name="Export Scene", description="export current scene (OgreDotScene xml)", default=CONFIG['SCENE'])
    EX_SELONLY = BoolProperty(name="Export Selected Only", description="export selected", default=CONFIG['SELONLY'])
    EX_FORCE_CAMERA = BoolProperty(name="Force Camera", description="export active camera", default=CONFIG['FORCE_CAMERA'])
    EX_FORCE_LAMPS = BoolProperty(name="Force Lamps", description="export all lamps", default=CONFIG['FORCE_LAMPS'])
    EX_MESH = BoolProperty(name="Export Meshes", description="export meshes", default=CONFIG['MESH'])
    EX_MESH_OVERWRITE = BoolProperty(name="Export Meshes (overwrite)", description="export meshes (overwrite existing files)", default=CONFIG['MESH_OVERWRITE'])
    EX_ARM_ANIM = BoolProperty(name="Armature Animation", description="export armature animations - updates the .skeleton file", default=CONFIG['ARM_ANIM'])
    EX_SHAPE_ANIM = BoolProperty(name="Shape Animation", description="export shape animations - updates the .mesh file", default=CONFIG['SHAPE_ANIM'])
    EX_TRIM_BONE_WEIGHTS = FloatProperty(
        name="Trim Weights", 
        description="ignore bone weights below this value (Ogre supports 4 bones per vertex)", 
        min=0.0, max=0.5, default=CONFIG['TRIM_BONE_WEIGHTS'] )

    EX_ARRAY = BoolProperty(name="Optimize Arrays", description="optimize array modifiers as instances (constant offset only)", default=CONFIG['ARRAY'])
    EX_MATERIALS = BoolProperty(name="Export Materials", description="exports .material script", default=CONFIG['MATERIALS'])
    EX_FORCE_IMAGE_FORMAT = EnumProperty( items=_IMAGE_FORMATS, name='Convert Images',  description='convert all textures to format', default=CONFIG['FORCE_IMAGE_FORMAT'] )
    EX_DDS_MIPS = IntProperty(name="DDS Mips", description="number of mip maps (DDS)", min=0, max=16, default=CONFIG['DDS_MIPS'])

    ## Mesh Options ##
    EX_lodLevels = IntProperty(name="LOD Levels", description="MESH number of LOD levels", min=0, max=32, default=CONFIG['lodLevels'])
    EX_lodDistance = IntProperty(name="LOD Distance", description="MESH distance increment to reduce LOD", min=0, max=2000, default=CONFIG['lodDistance'])
    EX_lodPercent = IntProperty(name="LOD Percentage", description="LOD percentage reduction", min=0, max=99, default=CONFIG['lodPercent'])
    EX_nuextremityPoints = IntProperty(name="Extremity Points", description="MESH Extremity Points", min=0, max=65536, default=CONFIG['nuextremityPoints'])
    EX_generateEdgeLists = BoolProperty(name="Edge Lists", description="MESH generate edge lists (for stencil shadows)", default=CONFIG['generateEdgeLists'])
    EX_generateTangents = BoolProperty(name="Tangents", description="MESH generate tangents", default=CONFIG['generateTangents'])
    EX_tangentSemantic = StringProperty(name="Tangent Semantic", description="MESH tangent semantic", maxlen=3, default=CONFIG['tangentSemantic'])
    EX_tangentUseParity = IntProperty(name="Tangent Parity", description="MESH tangent use parity", min=0, max=16, default=CONFIG['tangentUseParity'])
    EX_tangentSplitMirrored = BoolProperty(name="Tangent Split Mirrored", description="MESH split mirrored tangents", default=CONFIG['tangentSplitMirrored'])
    EX_tangentSplitRotated = BoolProperty(name="Tangent Split Rotated", description="MESH split rotated tangents", default=CONFIG['tangentSplitRotated'])
    EX_reorganiseBuffers = BoolProperty(name="Reorganise Buffers", description="MESH reorganise vertex buffers", default=CONFIG['reorganiseBuffers'])
    EX_optimiseAnimations = BoolProperty(name="Optimize Animations", description="MESH optimize animations", default=CONFIG['optimiseAnimations'])

    filepath= StringProperty(name="File Path", description="Filepath used for exporting .txml file", maxlen=1024, default="", subtype='FILE_PATH')
    EXPORT_TYPE = 'REX'



def _ogre_node_helper( doc, ob, objects, prefix='', pos=None, rot=None, scl=None ):
    mat = get_parent_matrix(ob, objects).inverted() * ob.matrix_world   # shouldn't this be matrix_local?

    o = doc.createElement('node')
    o.setAttribute('name',prefix+ob.name)
    p = doc.createElement('position')
    q = doc.createElement('rotation')       #('quaternion')
    s = doc.createElement('scale')
    for n in (p,q,s): o.appendChild(n)

    if pos: v = swap(pos)
    else: v = swap( mat.to_translation() )
    p.setAttribute('x', '%6f'%v.x)
    p.setAttribute('y', '%6f'%v.y)
    p.setAttribute('z', '%6f'%v.z)

    if rot: v = swap(rot)
    else: v = swap( mat.to_quaternion() )
    q.setAttribute('qx', '%6f'%v.x)
    q.setAttribute('qy', '%6f'%v.y)
    q.setAttribute('qz', '%6f'%v.z)
    q.setAttribute('qw','%6f'%v.w)

    if scl:        # this should not be used
        v = swap(scl)
        x=abs(v.x); y=abs(v.y); z=abs(v.z)
        s.setAttribute('x', '%6f'%x)
        s.setAttribute('y', '%6f'%y)
        s.setAttribute('z', '%6f'%z)
    else:        # scale is different in Ogre from blender - rotation is removed
        ri = mat.to_quaternion().inverted().to_matrix()
        scale = ri.to_4x4() * mat
        v = swap( scale.to_scale() )
        x=abs(v.x); y=abs(v.y); z=abs(v.z)
        s.setAttribute('x', '%6f'%x)
        s.setAttribute('y', '%6f'%y)
        s.setAttribute('z', '%6f'%z)

    return o



############ Ogre Command Line Tools ###########
class MeshMagick(object):
    ''' Usage: MeshMagick [global_options] toolname [tool_options] infile(s) -- [outfile(s)]
    Available Tools
    ===============
    info - print information about the mesh.
    meshmerge - Merge multiple submeshes into a single mesh.
    optimise - Optimise meshes and skeletons.
    rename - Rename different elements of meshes and skeletons.
    transform - Scale, rotate or otherwise transform a mesh.
    '''

    @staticmethod
    def get_merge_group( ob ): return get_merge_group( ob, prefix='magicmerge' )

    @staticmethod
    def merge( group, path='/tmp', force_name=None ):
        print('-'*80)
        print(' mesh magick - merge ')
        exe = CONFIG['OGRETOOLS_MESH_MAGICK']
        if not os.path.isfile( exe ):
            print( 'ERROR: can not find MeshMagick.exe' )
            print( exe )
            return

        files = []
        for ob in group.objects:
            if ob.data.users == 1:    # single users only
                files.append( os.path.join( path, ob.data.name+'.mesh' ) )
                print( files[-1] )

        opts = 'meshmerge'
        if sys.platform == 'linux2': cmd = '/usr/bin/wine %s %s' %(exe, opts)
        else: cmd = '%s %s' %(exe, opts)
        if force_name: output = force_name + '.mesh'
        else: output = '_%s_.mesh' %group.name
        cmd = cmd.split() + files + ['--', os.path.join(path,output) ]
        subprocess.call( cmd )
        print(' mesh magick - complete ')
        print('-'*80)

_ogre_command_line_tools_doc = '''
Usage: OgreXMLConverter [options] sourcefile [destfile]

Available options:
    -i             = interactive mode - prompt for options
    (The next 4 options are only applicable when converting XML to Mesh)
    -l lodlevels   = number of LOD levels
    -d loddist     = distance increment to reduce LOD
    -p lodpercent  = Percentage triangle reduction amount per LOD
    -f lodnumtris  = Fixed vertex reduction per LOD
    -e             = DON'T generate edge lists (for stencil shadows)
    -r             = DON'T reorganise vertex buffers to OGRE recommended format.
    -t             = Generate tangents (for normal mapping)
    -o             = DON'T optimise out redundant tracks & keyframes
    -d3d           = Prefer D3D packed colour formats (default on Windows)
    -gl            = Prefer GL packed colour formats (default on non-Windows)
    -E endian      = Set endian mode 'big' 'little' or 'native' (default)
    -q             = Quiet mode, less output
    -log filename  = name of the log file (default: 'OgreXMLConverter.log')
    sourcefile     = name of file to convert
    destfile       = optional name of file to write to. If you don't
                       specify this OGRE works it out through the extension
                       and the XML contents if the source is XML. For example
                       test.mesh becomes test.xml, test.xml becomes test.mesh
                       if the XML document root is <mesh> etc.
'''

def OgreXMLConverter( infile, has_uvs=False ):
    print('[Ogre Tools Wrapper]', infile )
    exe = CONFIG['OGRETOOLS_XML_CONVERTER']
    if not os.path.isfile( exe ):
        print( 'WARNING: can not find OgreXMLConverter (can not convert XXX.mesh.xml to XXX.mesh' )
        return

    basicArguments = ''

    if CONFIG['lodLevels']:
        basicArguments += ' -l %s -d %s -p %s' %(CONFIG['lodLevels'], CONFIG['lodDistance'], CONFIG['lodPercent'])
        
    if CONFIG['nuextremityPoints'] > 0:
        basicArguments += ' -x %s' %CONFIG['nuextremityPoints']

    if not CONFIG['generateEdgeLists']:
        basicArguments += ' -e'

    if CONFIG['generateTangents'] and has_uvs:  # OgreXmlConverter fails to convert meshes without UVs
        basicArguments += ' -t'
        if CONFIG['tangentSemantic']:
            basicArguments += ' -td %s' %CONFIG['tangentSemantic']
        if CONFIG['tangentUseParity']:
            basicArguments += ' -ts %s' %CONFIG['tangentUseParity']
        if CONFIG['tangentSplitMirrored']:
            basicArguments += ' -tm'
        if CONFIG['tangentSplitRotated']:
            basicArguments += ' -tr'
    if not CONFIG['reorganiseBuffers']:
        basicArguments += ' -r'
    if not CONFIG['optimiseAnimations']:
        basicArguments += ' -o'

    opts = '-log _ogre_debug.txt %s' %basicArguments
    path,name = os.path.split( infile )

    cmd = '%s %s' %(exe, opts)
    print('-'*80)
    print(cmd)
    print('_'*80)
    cmd = cmd.split() + [infile]
    subprocess.call( cmd )


class Bone(object):

    def __init__(self, rbone, pbone, skeleton):
        if CONFIG['SWAP_AXIS'] == 'xyz':
            self.fixUpAxis = False
        else:
            self.fixUpAxis = True
            if CONFIG['SWAP_AXIS'] == '-xzy':      # Tundra1
                self.flipMat = mathutils.Matrix(((-1,0,0,0),(0,0,1,0),(0,1,0,0),(0,0,0,1)))
            elif CONFIG['SWAP_AXIS'] == 'xz-y':    # Tundra2
                self.flipMat = mathutils.Matrix(((1,0,0,0),(0,0,1,0),(0,1,0,0),(0,0,0,1)))
            else:
                print( 'ERROR - TODO: axis swap mode not supported with armature animation' )
                assert 0

        self.skeleton = skeleton
        self.name = pbone.name
        self.matrix = rbone.matrix_local.copy() # armature space
        #self.matrix_local = rbone.matrix.copy()    # space?

        self.bone = pbone        # safe to hold pointer to pose bone, not edit bone!
        if not pbone.bone.use_deform: print('warning: bone <%s> is non-deformabled, this is inefficient!' %self.name)
        #TODO test#if pbone.bone.use_inherit_scale: print('warning: bone <%s> is using inherit scaling, Ogre has no support for this' %self.name)
        self.parent = pbone.parent
        self.children = []

    def update(self):        # called on frame update
        pose =  self.bone.matrix.copy()
        self._inverse_total_trans_pose = pose.inverted()
        # calculate difference to parent bone
        if self.parent:
            pose = self.parent._inverse_total_trans_pose* pose
        elif self.fixUpAxis:
            pose = self.flipMat * pose
        else:
            pass

        self.pose_location =  pose.to_translation()  -  self.ogre_rest_matrix.to_translation()
        pose = self.inverse_ogre_rest_matrix * pose
        self.pose_rotation = pose.to_quaternion()
        self.pose_scale = pose.to_scale()
        for child in self.children: child.update()


    def rebuild_tree( self ):        # called first on all bones
        if self.parent:
            self.parent = self.skeleton.get_bone( self.parent.name )
            self.parent.children.append( self )

    def compute_rest( self ):    # called after rebuild_tree, recursive roots to leaves
        if self.parent:
            inverseParentMatrix = self.parent.inverse_total_trans
        elif self.fixUpAxis:
            inverseParentMatrix = self.flipMat
        else:
            inverseParentMatrix = mathutils.Matrix(((1,0,0,0),(0,1,0,0),(0,0,1,0),(0,0,0,1)))

        #self.ogre_rest_matrix = self.skeleton.object_space_transformation * self.matrix    # ALLOW ROTATION?
        self.ogre_rest_matrix = self.matrix.copy()
        # store total inverse transformation
        self.inverse_total_trans = self.ogre_rest_matrix.inverted()
        # relative to OGRE parent bone origin
        self.ogre_rest_matrix = inverseParentMatrix * self.ogre_rest_matrix
        self.inverse_ogre_rest_matrix = self.ogre_rest_matrix.inverted()

        ## recursion ##
        for child in self.children:
            child.compute_rest()

class Skeleton(object):
    def get_bone( self, name ):
        for b in self.bones:
            if b.name == name: return b

    def __init__(self, ob ):
        if ob.location.x != 0 or ob.location.y != 0 or ob.location.z != 0:
            Report.warnings.append('ERROR: Mesh (%s): is offset from Armature - zero transform is required' %ob.name)
        if ob.scale.x != 1 or ob.scale.y != 1 or ob.scale.z != 1:
            Report.warnings.append('ERROR: Mesh (%s): has been scaled - scale(1,1,1) is required' %ob.name)

        self.object = ob
        self.bones = []
        mats = {}
        self.arm = arm = ob.find_armature()
        arm.hide = False
        self._restore_layers = list(arm.layers)
        #arm.layers = [True]*20      # can not have anything hidden - REQUIRED?

        for pbone in arm.pose.bones:
            mybone = Bone( arm.data.bones[pbone.name] ,pbone, self )
            self.bones.append( mybone )

        if arm.name not in Report.armatures: Report.armatures.append( arm.name )

        ## bad idea - allowing rotation of armature, means vertices must also be rotated,
        ## also a bug with applying the rotation, the Z rotation is lost
        #x,y,z = arm.matrix_local.copy().inverted().to_euler()
        #e = mathutils.Euler( (x,z,y) )
        #self.object_space_transformation = e.to_matrix().to_4x4()
        x,y,z = arm.matrix_local.to_euler()
        if x != 0 or y != 0 or z != 0:
            Report.warnings.append('ERROR: Armature: %s is rotated - (rotation is ignored)' %arm.name)

        ## setup bones for Ogre format ##
        for b in self.bones: b.rebuild_tree()
        ## walk bones, convert them ##
        self.roots = []
        ep = 0.0001
        for b in self.bones:
            if not b.parent:
                b.compute_rest()
                loc,rot,scl = b.ogre_rest_matrix.decompose()
                if loc.x or loc.y or loc.z:
                    Report.warnings.append('ERROR: root bone has non-zero transform (location offset)')
                if rot.w > ep or rot.x > ep or rot.y > ep or rot.z < 1.0-ep:
                    Report.warnings.append('ERROR: root bone has non-zero transform (rotation offset)')
                self.roots.append( b )

    def to_xml( self ):
        _fps = float( bpy.context.scene.render.fps )

        doc = RDocument()
        root = doc.createElement('skeleton'); doc.appendChild( root )
        bones = doc.createElement('bones'); root.appendChild( bones )
        bh = doc.createElement('bonehierarchy'); root.appendChild( bh )
        for i,bone in enumerate(self.bones):
            b = doc.createElement('bone')
            b.setAttribute('name', bone.name)
            b.setAttribute('id', str(i) )
            bones.appendChild( b )
            mat = bone.ogre_rest_matrix.copy()
            if bone.parent:
                bp = doc.createElement('boneparent')
                bp.setAttribute('bone', bone.name)
                bp.setAttribute('parent', bone.parent.name)
                bh.appendChild( bp )

            pos = doc.createElement( 'position' ); b.appendChild( pos )
            x,y,z = mat.to_translation()
            pos.setAttribute('x', '%6f' %x )
            pos.setAttribute('y', '%6f' %y )
            pos.setAttribute('z', '%6f' %z )
            rot =  doc.createElement( 'rotation' )        # note "rotation", not "rotate"
            b.appendChild( rot )

            q = mat.to_quaternion()
            rot.setAttribute('angle', '%6f' %q.angle )
            axis = doc.createElement('axis'); rot.appendChild( axis )
            x,y,z = q.axis
            axis.setAttribute('x', '%6f' %x )
            axis.setAttribute('y', '%6f' %y )
            axis.setAttribute('z', '%6f' %z )

            ## Ogre bones do not have initial scaling? ##
            ## NOTE: Ogre bones by default do not pass down their scaling in animation,
            ## so in blender all bones are like 'do-not-inherit-scaling'
            if 0:
                scale = doc.createElement('scale'); b.appendChild( scale )
                x,y,z = swap( mat.to_scale() )
                scale.setAttribute('x', str(x))
                scale.setAttribute('y', str(y))
                scale.setAttribute('z', str(z))

        arm = self.arm
        if not arm.animation_data or (arm.animation_data and not arm.animation_data.nla_tracks):  # assume animated via constraints and use blender timeline.
            anims = doc.createElement('animations'); root.appendChild( anims )
            anim = doc.createElement('animation'); anims.appendChild( anim )
            tracks = doc.createElement('tracks'); anim.appendChild( tracks )
            anim.setAttribute('name', 'my_animation')
            start = bpy.context.scene.frame_start; end = bpy.context.scene.frame_end
            anim.setAttribute('length', str( (end-start)/_fps ) )

            _keyframes = {}
            _bonenames_ = []
            for bone in arm.pose.bones:
                _bonenames_.append( bone.name )
                track = doc.createElement('track')
                track.setAttribute('bone', bone.name)
                tracks.appendChild( track )
                keyframes = doc.createElement('keyframes')
                track.appendChild( keyframes )
                _keyframes[ bone.name ] = keyframes

            for frame in range( int(start), int(end), bpy.context.scene.frame_step):
                bpy.context.scene.frame_set(frame)
                for bone in self.roots: bone.update()
                print('\t\t Frame:', frame)
                for bonename in _bonenames_:
                    bone = self.get_bone( bonename )
                    _loc = bone.pose_location
                    _rot = bone.pose_rotation
                    _scl = bone.pose_scale

                    keyframe = doc.createElement('keyframe')
                    keyframe.setAttribute('time', str((frame-start)/_fps))
                    _keyframes[ bonename ].appendChild( keyframe )
                    trans = doc.createElement('translate')
                    keyframe.appendChild( trans )
                    x,y,z = _loc
                    trans.setAttribute('x', '%6f' %x)
                    trans.setAttribute('y', '%6f' %y)
                    trans.setAttribute('z', '%6f' %z)

                    rot =  doc.createElement( 'rotate' )
                    keyframe.appendChild( rot )
                    q = _rot
                    rot.setAttribute('angle', '%6f' %q.angle )
                    axis = doc.createElement('axis'); rot.appendChild( axis )
                    x,y,z = q.axis
                    axis.setAttribute('x', '%6f' %x )
                    axis.setAttribute('y', '%6f' %y )
                    axis.setAttribute('z', '%6f' %z )

                    scale = doc.createElement('scale')
                    keyframe.appendChild( scale )
                    x,y,z = _scl
                    scale.setAttribute('x', '%6f' %x)
                    scale.setAttribute('y', '%6f' %y)
                    scale.setAttribute('z', '%6f' %z)


        elif arm.animation_data:
            anims = doc.createElement('animations'); root.appendChild( anims )
            if not len( arm.animation_data.nla_tracks ):
                Report.warnings.append('you must assign an NLA strip to armature (%s) that defines the start and end frames' %arm.name)

            for nla in arm.animation_data.nla_tracks:        # NLA required, lone actions not supported
                if not len(nla.strips): print( 'skipping empty NLA track: %s' %nla.name ); continue
                print('NLA track:',  nla.name)

                for strip in nla.strips:
                    print('   strip name:', strip.name)
                    anim = doc.createElement('animation'); anims.appendChild( anim )
                    tracks = doc.createElement('tracks'); anim.appendChild( tracks )
                    Report.armature_animations.append( '%s : %s [start frame=%s  end frame=%s]' %(arm.name, nla.name, strip.frame_start, strip.frame_end) )

                    anim.setAttribute('name', strip.name)                       # USE the strip name
                    anim.setAttribute('length', str( (strip.frame_end-strip.frame_start)/_fps ) )

                    stripbones = []
                    if CONFIG['ONLY_ANIMATED_BONES']:
                        for group in strip.action.groups:        # check if the user has keyed only some of the bones (for anim blending)
                            if group.name in arm.pose.bones: stripbones.append( group.name )
                        if not stripbones:                                    # otherwise we use all bones
                            stripbones = [ bone.name for bone in arm.pose.bones ]
                    else:
                        stripbones = [ bone.name for bone in arm.pose.bones ]

                    _keyframes = {}
                    for bonename in stripbones:
                        track = doc.createElement('track')
                        track.setAttribute('bone', bonename)
                        tracks.appendChild( track )
                        keyframes = doc.createElement('keyframes')
                        track.appendChild( keyframes )
                        _keyframes[ bonename ] = keyframes

                    for frame in range( int(strip.frame_start), int(strip.frame_end), bpy.context.scene.frame_step):
                        bpy.context.scene.frame_set(frame)
                        for bone in self.roots: bone.update()
                        for bonename in stripbones:
                            bone = self.get_bone( bonename )
                            _loc = bone.pose_location
                            _rot = bone.pose_rotation
                            _scl = bone.pose_scale

                            keyframe = doc.createElement('keyframe')
                            keyframe.setAttribute('time', str((frame-strip.frame_start)/_fps))
                            _keyframes[ bonename ].appendChild( keyframe )
                            trans = doc.createElement('translate')
                            keyframe.appendChild( trans )
                            x,y,z = _loc
                            trans.setAttribute('x', '%6f' %x)
                            trans.setAttribute('y', '%6f' %y)
                            trans.setAttribute('z', '%6f' %z)

                            rot =  doc.createElement( 'rotate' )
                            keyframe.appendChild( rot )
                            q = _rot
                            rot.setAttribute('angle', '%6f' %q.angle )
                            axis = doc.createElement('axis'); rot.appendChild( axis )
                            x,y,z = q.axis
                            axis.setAttribute('x', '%6f' %x )
                            axis.setAttribute('y', '%6f' %y )
                            axis.setAttribute('z', '%6f' %z )

                            scale = doc.createElement('scale')
                            keyframe.appendChild( scale )
                            x,y,z = _scl
                            scale.setAttribute('x', '%6f' %x)
                            scale.setAttribute('y', '%6f' %y)
                            scale.setAttribute('z', '%6f' %z)

        return doc.toprettyxml()


############ selector extras ############
class INFO_MT_instances(bpy.types.Menu):
    bl_label = "Instances"
    def draw(self, context):
        layout = self.layout
        inst = gather_instances()
        for data in inst:
            ob = inst[data][0]
            op = layout.operator(INFO_MT_instance.bl_idname, text=ob.name)    # operator has no variable for button name?
            op.mystring = ob.name
        layout.separator()

class INFO_MT_instance(bpy.types.Operator):
    '''select instance group'''
    bl_idname = "ogre.select_instances"
    bl_label = "Select Instance Group"
    bl_options = {'REGISTER', 'UNDO'}                              # Options for this panel type
    mystring= StringProperty(name="MyString", description="hidden string", maxlen=1024, default="my string")
    @classmethod
    def poll(cls, context):
        return True
    def invoke(self, context, event):
        print( 'invoke select_instances op', event )
        select_instances( context, self.mystring )
        return {'FINISHED'}

class INFO_MT_groups(bpy.types.Menu):
    bl_label = "Groups"
    def draw(self, context):
        layout = self.layout
        for group in bpy.data.groups:
            op = layout.operator(INFO_MT_group.bl_idname, text=group.name)    # operator no variable for button name?
            op.mystring = group.name
        layout.separator()

class INFO_MT_group(bpy.types.Operator):
    '''select group'''
    bl_idname = "ogre.select_group"
    bl_label = "Select Group"
    bl_options = {'REGISTER'}                              # Options for this panel type
    mystring= StringProperty(name="MyString", description="hidden string", maxlen=1024, default="my string")
    @classmethod
    def poll(cls, context):
        return True
    def invoke(self, context, event):
        select_group( context, self.mystring )
        return {'FINISHED'}

#############




NVDXT_DOC = '''
Version 8.30
NVDXT
This program
   compresses images
   creates normal maps from color or alpha
   creates DuDv map
   creates cube maps
   writes out .dds file
   does batch processing
   reads .tga, .bmp, .gif, .ppm, .jpg, .tif, .cel, .dds, .png, .psd, .rgb, *.bw and .rgba
   filters MIP maps

Options:
  -profile <profile name> : Read a profile created from the Photoshop plugin
  -quick : use fast compression method
  -quality_normal : normal quality compression
  -quality_production : production quality compression
  -quality_highest : highest quality compression (this can be very slow)
  -rms_threshold <int> : quality RMS error. Above this, an extensive search is performed.
  -prescale <int> <int>: rescale image to this size first
  -rescale <nearest | hi | lo | next_lo>: rescale image to nearest, next highest or next lowest power of two
  -rel_scale <float, float> : relative scale of original image. 0.5 is half size Default 1.0, 1.0

Optional Filtering for rescaling. Default cube filter:
  -RescalePoint
  -RescaleBox
  -RescaleTriangle
  -RescaleQuadratic
  -RescaleCubic
  -RescaleCatrom
  -RescaleMitchell
  -RescaleGaussian
  -RescaleSinc
  -RescaleBessel
  -RescaleHanning
  -RescaleHamming
  -RescaleBlackman
  -RescaleKaiser
  -clamp <int, int> : maximum image size. image width and height are clamped
  -clampScale <int, int> : maximum image size. image width and height are scaled 
  -window <left, top, right, bottom> : window of original window to compress
  -nomipmap : don't generate MIP maps
  -nmips <int> : specify the number of MIP maps to generate
  -rgbe : Image is RGBE format
  -dither : add dithering
  -sharpenMethod <method>: sharpen method MIP maps
  <method> is 
        None
        Negative
        Lighter
        Darker
        ContrastMore
        ContrastLess
        Smoothen
        SharpenSoft
        SharpenMedium
        SharpenStrong
        FindEdges
        Contour
        EdgeDetect
        EdgeDetectSoft
        Emboss
        MeanRemoval
        UnSharp <radius, amount, threshold>
        XSharpen <xsharpen_strength, xsharpen_threshold>
        Custom
  -pause : wait for keyboard on error
  -flip : flip top to bottom 
  -timestamp : Update only changed files
  -list <filename> : list of files to convert
  -cubeMap : create cube map . 
            Cube faces specified with individual files with -list option
                  positive x, negative x, positive y, negative y, positive z, negative z
                  Use -output option to specify filename
            Cube faces specified in one file.  Use -file to specify input filename

  -volumeMap : create volume texture. 
            Volume slices specified with individual files with -list option
                  Use -output option to specify filename
            Volume specified in one file.  Use -file to specify input filename

  -all : all image files in current directory
  -outdir <directory>: output directory
  -deep [directory]: include all subdirectories
  -outsamedir : output directory same as input
  -overwrite : if input is .dds file, overwrite old file
  -forcewrite : write over readonly files
  -file <filename> : input file to process. Accepts wild cards
  -output <filename> : filename to write to [-outfile can also be specified]
  -append <filename_append> : append this string to output filename
  -8  <dxt1c | dxt1a | dxt3 | dxt5 | u1555 | u4444 | u565 | u8888 | u888 | u555 | L8 | A8>  : compress 8 bit images with this format
  -16 <dxt1c | dxt1a | dxt3 | dxt5 | u1555 | u4444 | u565 | u8888 | u888 | u555 | A8L8> : compress 16 bit images with this format
  -24 <dxt1c | dxt1a | dxt3 | dxt5 | u1555 | u4444 | u565 | u8888 | u888 | u555> : compress 24 bit images with this format
  -32 <dxt1c | dxt1a | dxt3 | dxt5 | u1555 | u4444 | u565 | u8888 | u888 | u555> : compress 32 bit images with this format

  -swapRB : swap rb
  -swapRG : swap rg
  -gamma <float value>: gamma correcting during filtering
  -outputScale <float, float, float, float>: scale the output by this (r,g,b,a)
  -outputBias <float, float, float, float>: bias the output by this amount (r,g,b,a)
  -outputWrap : wraps overflow values modulo the output format 
  -inputScale <float, float, float, float>: scale the inpput by this (r,g,b,a)
  -inputBias <float, float, float, float>: bias the input by this amount (r,g,b,a)
  -binaryalpha : treat alpha as 0 or 1
  -alpha_threshold <byte>: [0-255] alpha reference value 
  -alphaborder : border images with alpha = 0
  -alphaborderLeft : border images with alpha (left) = 0
  -alphaborderRight : border images with alpha (right)= 0
  -alphaborderTop : border images with alpha (top) = 0
  -alphaborderBottom : border images with alpha (bottom)= 0
  -fadeamount <int>: percentage to fade each MIP level. Default 15

  -fadecolor : fade map (color, normal or DuDv) over MIP levels
  -fadetocolor <hex color> : color to fade to
  -custom_fade <n> <n fadeamounts> : set custom fade amount.  n is number number of fade amounts. fadeamount are [0,1]
  -fadealpha : fade alpha over MIP levels
  -fadetoalpha <byte>: [0-255] alpha to fade to
  -border : border images with color
  -bordercolor <hex color> : color for border
  -force4 : force DXT1c to use always four colors
  -weight <float, float, float>: Compression weightings for R G and B
  -luminance :  convert color values to luminance for L8 formats
  -greyScale : Convert to grey scale
  -greyScaleWeights <float, float, float, float>: override greyscale conversion weights of (0.3086, 0.6094, 0.0820, 0)  
  -brightness <float, float, float, float>: per channel brightness. Default 0.0  usual range [0,1]
  -contrast <float, float, float, float>: per channel contrast. Default 1.0  usual range [0.5, 1.5]

Texture Format  Default DXT3:
  -dxt1c   : DXT1 (color only)
  -dxt1a   : DXT1 (one bit alpha)
  -dxt3    : DXT3
  -dxt5    : DXT5n
  -u1555   : uncompressed 1:5:5:5
  -u4444   : uncompressed 4:4:4:4
  -u565    : uncompressed 5:6:5
  -u8888   : uncompressed 8:8:8:8
  -u888    : uncompressed 0:8:8:8
  -u555    : uncompressed 0:5:5:5
  -p8c     : paletted 8 bit (256 colors)
  -p8a     : paletted 8 bit (256 colors with alpha)
  -p4c     : paletted 4 bit (16 colors)
  -p4a     : paletted 4 bit (16 colors with alpha)
  -a8      : 8 bit alpha channel
  -cxv8u8  : normal map format
  -v8u8    : EMBM format (8, bit two component signed)
  -v16u16  : EMBM format (16 bit, two component signed)
  -A8L8    : 8 bit alpha channel, 8 bit luminance
  -fp32x4  : fp32 four channels (A32B32G32R32F)
  -fp32    : fp32 one channel (R32F)
  -fp16x4  : fp16 four channels (A16B16G16R16F)
  -dxt5nm  : dxt5 style normal map
  -3Dc     : 3DC
  -g16r16  : 16 bit in, two component
  -g16r16f : 16 bit float, two components

Mip Map Filtering Options. Default box filter:
  -Point
  -Box
  -Triangle
  -Quadratic
  -Cubic
  -Catrom
  -Mitchell
  -Gaussian
  -Sinc
  -Bessel
  -Hanning
  -Hamming
  -Blackman
  -Kaiser

***************************
To make a normal or dudv map, specify one of
  -n4 : normal map 4 sample
  -n3x3 : normal map 3x3 filter
  -n5x5 : normal map 5x5 filter
  -n7x7 : normal map 7x7 filter
  -n9x9 : normal map 9x9 filter
  -dudv : DuDv

and source of height info:
  -alpha : alpha channel
  -rgb : average rgb
  -biased : average rgb biased
  -red : red channel
  -green : green channel
  -blue : blue channel
  -max : max of (r,g,b)
  -colorspace : mix of r,g,b

-norm : normalize mip maps (source is a normal map)

-toHeight : create a height map (source is a normal map)


Normal/DuDv Map dxt:
  -aheight : store calculated height in alpha field
  -aclear : clear alpha channel
  -awhite : set alpha channel = 1.0
  -scale <float> : scale of height map. Default 1.0
  -wrap : wrap texture around. Default off
  -minz <int> : minimum value for up vector [0-255]. Default 0

***************************
To make a depth sprite, specify:
  -depth

and source of depth info:
  -alpha  : alpha channel
  -rgb    : average rgb (default)
  -red    : red channel
  -green  : green channel
  -blue   : blue channel
  -max    : max of (r,g,b)
  -colorspace : mix of r,g,b

Depth Sprite dxt:
  -aheight : store calculated depth in alpha channel
  -aclear : store 0.0 in alpha channel
  -awhite : store 1.0 in alpha channel
  -scale <float> : scale of depth sprite (default 1.0)
  -alpha_modulate : multiplies color by alpha during filtering
  -pre_modulate : multiplies color by alpha before processing

Examples
  nvdxt -cubeMap -list cubemapfile.lst -output cubemap.dds
  nvdxt -cubeMap -file cubemapfile.tga
  nvdxt -file test.tga -dxt1c
  nvdxt -file *.tga
  nvdxt -file c:\temp\*.tga
  nvdxt -file temp\*.tga
  nvdxt -file height_field_in_alpha.tga -n3x3 -alpha -scale 10 -wrap
  nvdxt -file grey_scale_height_field.tga -n5x5 -rgb -scale 1.3
  nvdxt -file normal_map.tga -norm
  nvdxt -file image.tga -dudv -fade -fadeamount 10
  nvdxt -all -dxt3 -gamma -outdir .\dds_dir -time
  nvdxt -file *.tga -depth -max -scale 0.5

'''





try: import io_export_rogremesh.rogremesh as Rmesh
except:
    Rmesh = None
    print( 'WARNING: "io_export_rogremesh" is missing' )

if Rmesh and Rmesh.rpy.load(): _USE_RPYTHON_ = True
else:
    _USE_RPYTHON_ = False
    print( 'Rpython module is not cached, you must exit Blender to compile the module:' )
    print( 'cd io_export_rogremesh; python rogremesh.py' )


class VertexNoPos(object):
    def __init__(self, ogre_vidx, nx,ny,nz, r,g,b,ra, vert_uvs):
        self.ogre_vidx = ogre_vidx
        self.nx = nx
        self.ny = ny
        self.nz = nz
        self.r = r
        self.g = g
        self.b = b
        self.ra = ra
        self.vert_uvs = vert_uvs

    '''does not compare ogre_vidx (and position at the moment) [ no need to compare position ]'''
    def __eq__(self, o):
        if self.nx != o.nx or self.ny != o.ny or self.nz != o.nz: return False
        elif self.r != o.r or self.g != o.g or self.b != o.b or self.ra != o.ra: return False
        elif len(self.vert_uvs) != len(o.vert_uvs): return False
        elif self.vert_uvs:
            for i, uv1 in enumerate( self.vert_uvs ):
                uv2 = o.vert_uvs[ i ]
                if uv1 != uv2: return False
        return True


#######################################################################################
def dot_mesh( ob, path='/tmp', force_name=None, ignore_shape_animation=False, normals=True ):
    start = time.time()
    print('mesh to Ogre mesh XML format', ob.name)

    if not os.path.isdir( path ):
        print('creating directory', path )
        os.makedirs( path )

    Report.meshes.append( ob.data.name )
    Report.faces += len( ob.data.faces )
    Report.orig_vertices += len( ob.data.vertices )

    cleanup = False
    if ob.modifiers:
        cleanup = True
        copy = ob.copy()
        #bpy.context.scene.objects.link(copy)
        rem = []
        for mod in copy.modifiers:        # remove armature and array modifiers before collaspe
            if mod.type in 'ARMATURE ARRAY'.split(): rem.append( mod )
        for mod in rem: copy.modifiers.remove( mod )
        ## bake mesh ##
        mesh = copy.to_mesh(bpy.context.scene, True, "PREVIEW")    # collaspe
    else:
        copy = ob
        mesh = ob.data

    print('creating document...')

    name = force_name or ob.data.name
    xmlfile = os.path.join(path, '%s.mesh.xml' %name )

    if _USE_RPYTHON_ and False:
        Rmesh.save( ob, xmlfile )

    else:
        f = open( xmlfile, 'w' )
        doc = SimpleSaxWriter(f, 'UTF-8', "mesh", {})

        #//very ugly, have to replace number of vertices later
        doc.start_tag('sharedgeometry', {'vertexcount' : '__TO_BE_REPLACED_VERTEX_COUNT__'})

        print('    writing shared geometry')
        doc.start_tag('vertexbuffer', {
                'positions':'true',
                'normals':'true',
                'colours_diffuse' : str(bool( mesh.vertex_colors )),
                'texture_coords' : '%s' % len(mesh.uv_textures) if mesh.uv_textures.active else '0'
        })

        ## vertex colors ##
        vcolors = None
        vcolors_alpha = None
        if len( mesh.vertex_colors ):
            vcolors = mesh.vertex_colors[0]
            for bloc in mesh.vertex_colors:
                if bloc.name.lower().startswith('alpha'):
                    vcolors_alpha = bloc; break

        ######################################################

        materials = []
        for mat in ob.data.materials:
            if mat: materials.append( mat )
            else:
                print('WARNING: bad material data', ob)
                materials.append( '_missing_material_' )        # fixed dec22, keep proper index
        if not materials: materials.append( '_missing_material_' )
        _sm_faces_ = []
        for matidx, mat in enumerate( materials ):
            _sm_faces_.append([])


        dotextures = False
        uvcache = []    # should get a little speed boost by this cache
        if mesh.uv_textures.active:
            dotextures = True
            for layer in mesh.uv_textures:
                uvs = []; uvcache.append( uvs ) # layer contains: name, active, data
                for uvface in layer.data:
                    uvs.append( (uvface.uv1, uvface.uv2, uvface.uv3, uvface.uv4) )


        _sm_vertices_ = {}
        _remap_verts_ = []
        numverts = 0
        for F in mesh.faces:
            smooth = F.use_smooth
            #print(F, "is smooth=", smooth)
            faces = _sm_faces_[ F.material_index ]
            ## Ogre only supports triangles
            tris = []
            tris.append( (F.vertices[0], F.vertices[1], F.vertices[2]) )
            if len(F.vertices) >= 4: tris.append( (F.vertices[0], F.vertices[2], F.vertices[3]) )
            if dotextures:
                a = []; b = []
                uvtris = [ a, b ]
                for layer in uvcache:
                    uv1, uv2, uv3, uv4 = layer[ F.index ]
                    a.append( (uv1, uv2, uv3) )
                    b.append( (uv1, uv3, uv4) )
                    
                    
            
            for tidx, tri in enumerate(tris):
                face = []
                for vidx, idx in enumerate(tri):
                    v = mesh.vertices[ idx ]
                    
                    if smooth: nx,ny,nz = swap( v.normal )     # fixed june 17th 2011
                    else: nx,ny,nz = swap( F.normal )
                    
                    r = 1.0
                    g = 1.0
                    b = 1.0
                    ra = 1.0
                    if vcolors:
                        k = list(F.vertices).index(idx)
                        r,g,b = getattr( vcolors.data[ F.index ], 'color%s'%(k+1) )
                        if vcolors_alpha:
                            ra,ga,ba = getattr( vcolors_alpha.data[ F.index ], 'color%s'%(k+1) )
                        else:
                            ra = 1.0

                    ## texture maps ##
                    vert_uvs = []
                    if dotextures:
                        for layer in uvtris[ tidx ]:
                            vert_uvs.append(layer[ vidx ])
                    
                    
                    #check if we already exported that vertex with same normal, do not export in that case, (flat shading in blender seems to 
                    #work with face normals, so we copy each flat face' vertices, if this vertex with same normals was already exported,
                    #TODO: maybe not best solution, check other ways (let blender do all the work, or only support smooth shading, what about seems, smoothing groups, materials, ...)
                    vert = VertexNoPos(numverts, nx, ny, nz, r, g, b, ra, vert_uvs)
                    #print("DEBUG: %i %.9f %.9f %.9f len^2: %.9f" % (numverts, nx, ny, nz, nx*nx+ny*ny+nz*nz))
                    alreadyExported = False
                    if idx in _sm_vertices_:
                        for vert2 in _sm_vertices_[idx]:
                            #does not compare ogre_vidx (and position at the moment)
                            if vert == vert2:
                                face.append(vert2.ogre_vidx)
                                alreadyExported = True
                                #print(idx,numverts, nx,ny,nz, r,g,b,ra, vert_uvs, "already exported")
                                break
                        if not alreadyExported:
                            face.append(vert.ogre_vidx)
                            _sm_vertices_[idx].append(vert)
                            #print(numverts, nx,ny,nz, r,g,b,ra, vert_uvs, "appended")
                    else:
                        face.append(vert.ogre_vidx)
                        _sm_vertices_[idx] = [vert]
                        #print(idx, numverts, nx,ny,nz, r,g,b,ra, vert_uvs, "created")
                    
                    if alreadyExported:
                        continue
                    
                    numverts += 1
                    _remap_verts_.append( v )

                    x,y,z = swap(v.co)        # xz-y is correct!
                    
                    doc.start_tag('vertex', {})
                    doc.leaf_tag('position', {
                            'x' : '%6f' % x,
                            'y' : '%6f' % y,
                            'z' : '%6f' % z
                    })
                    
                    
                    doc.leaf_tag('normal', {
                            'x' : '%6f' % nx,
                            'y' : '%6f' % ny,
                            'z' : '%6f' % nz
                    })

                    if vcolors:
                        doc.leaf_tag('colour_diffuse', {'value' : '%6f %6f %6f %6f' % (r,g,b,ra)})

                    ## texture maps ##
                    if dotextures:
                        for uv in vert_uvs:
                            doc.leaf_tag('texcoord', {
                                    'u' : '%6f' % uv[0],
                                    'v' : '%6f' % (1.0-uv[1])
                            })
                    
                    
                    doc.end_tag('vertex')
                
                faces.append( (face[0], face[1], face[2]) )
                
        del(_sm_vertices_)
        Report.vertices += numverts
        
        doc.end_tag('vertexbuffer')
        doc.end_tag('sharedgeometry')
        print(' time: ', time.time()-start )
        print('    writing submeshes' )
        doc.start_tag('submeshes', {})
        for matidx, mat in enumerate( materials ):
            if not len(_sm_faces_[matidx]):
                Report.warnings.append( 'BAD SUBMESH: %s' %ob.name )
                continue	# fixes corrupt unused materials

            doc.start_tag('submesh', {
                    'usesharedvertices' : 'true',
                    'material' : material_name(mat),
                    #maybe better look at index of all faces, if one over 65535 set to true;
                    #problem: we know it too late, postprocessing of file needed
                    "use32bitindexes" : str(bool(numverts > 65535))
            })
            doc.start_tag('faces', {
                    'count' : str(len(_sm_faces_[matidx]))
            })
            for fidx, (v1, v2, v3) in enumerate(_sm_faces_[matidx]):
                doc.leaf_tag('face', {
                    'v1' : str(v1),
                    'v2' : str(v2),
                    'v3' : str(v3)
                })
            doc.end_tag('faces')
            doc.end_tag('submesh')
            Report.triangles += len(_sm_faces_[matidx])
        del(_sm_faces_)
        doc.end_tag('submeshes')

        
        arm = ob.find_armature()
        if arm:
            doc.leaf_tag('skeletonlink', {
                    'name' : '%s.skeleton' %(force_name or ob.data.name)
            })
            doc.start_tag('boneassignments', {})
            badverts = 0
            for vidx, v in enumerate(_remap_verts_):
                check = 0
                for vgroup in v.groups:
                    if vgroup.weight > CONFIG['TRIM_BONE_WEIGHTS']:
                        bnidx = find_bone_index(copy,arm,vgroup.group)
                        if bnidx is not None:        # allows other vertex groups, not just armature vertex groups
                            doc.leaf_tag('vertexboneassignment', {
                                    'vertexindex' : str(vidx),
                                    'boneindex' : str(bnidx),
                                    'weight' : str(vgroup.weight)
                            })
                            check += 1
                if check > 4:
                    badverts += 1
                    #print('WARNING: vertex %s is in more than 4 vertex groups (bone weights)\n(this maybe Ogre incompatible)' %vidx)
            if badverts:
                Report.warnings.append( '%s has %s vertices weighted to too many bones (Ogre limits a vertex to 4 bones)\n[try increaseing the Trim-Weights threshold option]' %(mesh.name, badverts) )
            doc.end_tag('boneassignments')

        ############################################
        ## updated June3 2011 - shape animation works ##
        if CONFIG['SHAPE_ANIM'] and ob.data.shape_keys and len(ob.data.shape_keys.key_blocks):
            print('    writing shape keys')

            doc.start_tag('poses', {})
            for sidx, skey in enumerate(ob.data.shape_keys.key_blocks):
                if sidx == 0: continue
                if len(skey.data) != len( mesh.vertices ):
                    failure = 'FAILED to save shape animation - you can not use a modifier that changes the vertex count! '
                    failure += '[ mesh : %s ]' %mesh.name
                    Report.warnings.append( failure )
                    print( failure )
                    break

                doc.start_tag('pose', {
                        'name' : skey.name,
                        # If target is 'mesh', no index needed, if target is submesh then submesh identified by 'index'
                        #'index' : str(sidx-1),
                        #'index' : '0',
                        'target' : 'mesh'
                })

                for vidx, v in enumerate(_remap_verts_):
                    pv = skey.data[ v.index ]
                    x,y,z = swap( pv.co - v.co )
                    #for i,p in enumerate( skey.data ):
                    #x,y,z = p.co - ob.data.vertices[i].co
                    #x,y,z = swap( ob.data.vertices[i].co - p.co )
                    #if x==.0 and y==.0 and z==.0: continue        # the older exporter optimized this way, is it safe?
                    doc.leaf_tag('poseoffset', {
                            'x' : '%6f' % x,
                            'y' : '%6f' % y,
                            'z' : '%6f' % z,
                            'index' : str(vidx)     # is this required?
                    })
                doc.end_tag('pose')
            doc.end_tag('poses')

            if ob.data.shape_keys.animation_data and len(ob.data.shape_keys.animation_data.nla_tracks):
                print('    writing shape animations')
                doc.start_tag('animations', {})
                _fps = float( bpy.context.scene.render.fps )
                for nla in ob.data.shape_keys.animation_data.nla_tracks:
                    for idx, strip in enumerate(nla.strips):
                        doc.start_tag('animation', {
                                'name' : strip.name,
                                'length' : str((strip.frame_end-strip.frame_start)/_fps)
                        })
                        doc.start_tag('tracks', {})
                        doc.start_tag('track', {
                                'type' : 'pose',
                                'target' : 'mesh'
                                # If target is 'mesh', no index needed, if target is submesh then submesh identified by 'index'
                                #'index' : str(idx)
                                #'index' : '0'
                        })
                        doc.start_tag('keyframes', {})
                        for frame in range( int(strip.frame_start), int(strip.frame_end), bpy.context.scene.frame_step):
                            bpy.context.scene.frame_set(frame)
                            doc.start_tag('keyframe', {
                                    'time' : str((frame-strip.frame_start)/_fps)
                            })
                            for sidx, skey in enumerate( ob.data.shape_keys.key_blocks ):
                                if sidx == 0: continue
                                doc.leaf_tag('poseref', {
                                        'poseindex' : str(sidx-1),
                                        'influence' : str(skey.value)
                                })
                            doc.end_tag('keyframe')
                        doc.end_tag('keyframes')
                        doc.end_tag('track')
                        doc.end_tag('tracks')
                        doc.end_tag('animation')
                doc.end_tag('animations')


        ########## clean up and save #############
        #bpy.context.scene.meshes.unlink(mesh)
        if cleanup:
            #bpy.context.scene.objects.unlink(copy)
            bpy.data.objects.remove(copy)
            bpy.data.meshes.remove(mesh)
            mesh.user_clear()
            copy.user_clear()
            del copy
            del mesh

        del _remap_verts_
        del uvcache

        doc.close()     # reported by Reyn
        f.close()

    #very ugly, find better way
    def replaceInplace(f,searchExp,replaceExp):
            import fileinput
            for line in fileinput.input(f, inplace=1):
                if searchExp in line:
                    line = line.replace(searchExp,replaceExp)
                sys.stdout.write(line)
            fileinput.close()   # reported by jakob
    
    replaceInplace(xmlfile, '__TO_BE_REPLACED_VERTEX_COUNT__' + '"', str(numverts) + '"' )#+ ' ' * (ls - lr))
    del(replaceInplace)
        
    OgreXMLConverter( xmlfile, has_uvs=dotextures )

    if arm and CONFIG['ARM_ANIM']:
        skel = Skeleton( ob )
        data = skel.to_xml()
        name = force_name or ob.data.name
        xmlfile = os.path.join(path, '%s.skeleton.xml' %name )
        f = open( xmlfile, 'wb' )
        f.write( bytes(data,'utf-8') )
        f.close()
        OgreXMLConverter( xmlfile )

    mats = []
    for mat in materials:
        if mat != '_missing_material_': mats.append( mat )

    print('*'*80)
    print( 'TIME: ', time.time()-start )
    return mats

## end dot_mesh ##

############### Jmonkey ################ TODO remove jmonkey
class JmonkeyPreviewOp( _OgreCommonExport_, bpy.types.Operator ):
    '''helper to open jMonkey (JME)'''
    bl_idname = 'jmonkey.preview'
    bl_label = "opens JMonkeyEngine in a non-blocking subprocess"
    bl_options = {'REGISTER'}

    filepath= StringProperty(name="File Path", description="Filepath used for exporting Jmonkey .scene file", maxlen=1024, default="/tmp/preview.txml", subtype='FILE_PATH')
    EXPORT_TYPE = 'OGRE'

    @classmethod
    def poll(cls, context):
        if context.active_object: return True

    def invoke(self, context, event):
        global TundraSingleton
        path = '/tmp/preview.scene'
        self.ogre_export( path, context )
        JmonkeyPipe( path )
        return {'FINISHED'}

def JmonkeyPipe( path ):
    root = CONFIG[ 'JMONKEY_ROOT']
    if sys.platform.startswith('win'):
        cmd = [ os.path.join( os.path.join( root, 'bin' ), 'jmonkeyplatform.exe' ) ]
    else:
        cmd = [ os.path.join( os.path.join( root, 'bin' ), 'jmonkeyplatform' ) ]
    cmd.append( '--nosplash' )
    cmd.append( '--open' )
    cmd.append( path )
    proc = subprocess.Popen(cmd)#, stdin=subprocess.PIPE)
    return proc

############### Tundra ################

class TundraPreviewOp( _OgreCommonExport_, bpy.types.Operator ):
    '''helper to open Tundra2 (realXtend)'''
    bl_idname = 'tundra.preview'
    bl_label = "opens Tundra2 in a non-blocking subprocess"
    bl_options = {'REGISTER'}
    filepath= StringProperty(name="File Path", description="Filepath used for exporting Tundra .txml file", maxlen=1024, default="/tmp/preview.txml", subtype='FILE_PATH')
    EXPORT_TYPE = 'REX'
    EX_FORCE_CAMERA = BoolProperty(name="Force Camera", description="export active camera", default=False)
    EX_FORCE_LAMPS = BoolProperty(name="Force Lamps", description="export all lamps", default=False)

    @classmethod
    def poll(cls, context):
        if context.active_object and context.mode != 'EDIT_MESH': return True

    def invoke(self, context, event):
        global TundraSingleton
        syncmats = []
        obs = []
        if TundraSingleton:
            actob = context.active_object
            obs = TundraSingleton.deselect_previously_updated(context)
            for ob in obs:
                if ob.type=='MESH':
                    syncmats.append( ob )
                    if ob.name == actob.name:
                        export_mesh( ob, path='/tmp/rex' )

        if not os.path.isdir( '/tmp/rex' ): os.makedirs( '/tmp/rex' )
        path = '/tmp/rex/preview.txml'
        self.ogre_export( path, context, force_material_update=syncmats )
        if not TundraSingleton:
            TundraSingleton = TundraPipe( context )
        elif self.EX_SCENE:
            TundraSingleton.load( context, path )

        for ob in obs: ob.select = True     # restore selection
        return {'FINISHED'}

TundraSingleton = None

class Tundra_StartPhysicsOp(bpy.types.Operator):
    '''TundraSingleton helper'''
    bl_idname = 'tundra.start_physics'
    bl_label = "start physics"
    bl_options = {'REGISTER'}
    @classmethod
    def poll(cls, context):
        if TundraSingleton: return True
    def invoke(self, context, event):
        TundraSingleton.start()
        return {'FINISHED'}

class Tundra_StopPhysicsOp(bpy.types.Operator):
    '''TundraSingleton helper'''
    bl_idname = 'tundra.stop_physics'
    bl_label = "stop physics"
    bl_options = {'REGISTER'}
    @classmethod
    def poll(cls, context):
        if TundraSingleton: return True
    def invoke(self, context, event):
        TundraSingleton.stop()
        return {'FINISHED'}

class Tundra_PhysicsDebugOp(bpy.types.Operator):
    '''TundraSingleton helper'''
    bl_idname = 'tundra.toggle_physics_debug'
    bl_label = "stop physics"
    bl_options = {'REGISTER'}
    @classmethod
    def poll(cls, context):
        if TundraSingleton: return True
    def invoke(self, context, event):
        TundraSingleton.toggle_physics_debug()
        return {'FINISHED'}

class Tundra_ExitOp(bpy.types.Operator):
    '''TundraSingleton helper'''
    bl_idname = 'tundra.exit'
    bl_label = "quit tundra"
    bl_options = {'REGISTER'}
    @classmethod
    def poll(cls, context):
        if TundraSingleton: return True
    def invoke(self, context, event):
        TundraSingleton.exit()
        return {'FINISHED'}

#######################
class Server(object):
    def stream( self, o ):
        b = pickle.dumps( o, protocol=2 ) #protocol2 is python2 compatible
        #print( 'streaming bytes', len(b) )
        n = len( b ); d = STREAM_BUFFER_SIZE - n -4
        if n > STREAM_BUFFER_SIZE:
            print( 'ERROR: STREAM OVERFLOW:', n )
            return
        padding = b'#' * d
        if n < 10: header = '000%s' %n
        elif n < 100: header = '00%s' %n
        elif n < 1000: header = '0%s' %n
        else: header = '%s' %n
        header = bytes( header, 'utf-8' )
        assert len(header) == 4
        w = header + b + padding
        assert len(w) == STREAM_BUFFER_SIZE
        self.buffer.insert(0, w )
        return w

    def sync( self ):   # 153 bytes per object + n bytes for animation names and weights
        p = STREAM_PROTO
        i = 0; msg = []
        for ob in bpy.context.selected_objects:
            if ob.type not in ('MESH','LAMP'): continue
            loc, rot, scale = ob.matrix_world.decompose()
            loc = swap(loc).to_tuple()
            x,y,z = swap( rot.to_euler() )
            rot = (x,y,z)
            x,y,z = swap( scale )
            scale = ( abs(x), abs(y), abs(z) )
            d = { p['ID']:uid(ob), p['POSITION']:loc, p['ROTATION']:rot, p['SCALE']:scale, p['TYPE']:p[ob.type] }
            msg.append( d )
            arm = ob.find_armature()
            if arm and arm.animation_data and arm.animation_data.nla_tracks:
                anim = None
                d[ p['ANIMATIONS'] ] = state = {}    # animation-name : weight
                for nla in arm.animation_data.nla_tracks:
                    for strip in nla.strips:
                        if strip.active: state[ strip.name ] = strip.influence
            else: pass      # armature without proper NLA setup

            if ob.type == 'LAMP':
                d[ p['ENERGY'] ] = ob.data.energy
                d[ p['DISTANCE'] ] = ob.data.distance

            if i >= 10: break    # max is 13 objects to stay under 2048 bytes
        return msg

    def __init__(self):
        import socket
        self.buffer = []    # cmd buffer
        self.socket = sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)   # UDP
        host='localhost'; port = 9420
        sock.connect((host, port))
        print('SERVER: socket connected', sock)
        self._handle = None
        self.setup_callback( bpy.context )
        import threading
        self.ready = threading._allocate_lock()
        self.ID = threading._start_new_thread( 
            self.loop, (None,) 
        )
        print( 'SERVER: thread started')

    def loop(self, none):
        self.active = True
        prev = time.time()
        while self.active:
            if not self.ready.locked(): time.sleep(0.001)    # not threadsafe
            else:    # threadsafe start
                if not bpy.context.active_object: continue
                now = time.time()
                if now - prev > 0.033:            # don't flood Tundra
                    prev = now
                    sel = bpy.context.active_object
                    msg = self.sync()
                    self.ready.release()          # thread release
                    self.stream( msg )            # releases GIL?
                    if self.buffer:
                        bin = self.buffer.pop()
                        try:
                            self.socket.sendall( bin )
                        except:
                            print('SERVER: send data error')
                            time.sleep(0.5)
                            pass
                    else: print( 'SERVER: empty buffer' )
                else:
                    self.ready.release()
        print('SERVER: thread exit')

    def threadsafe( self, reg ):
        if not TundraSingleton: return
        if not self.ready.locked():
            self.ready.acquire()
            time.sleep(0.0001)
            while self.ready.locked():    # must block to be safe
                time.sleep(0.0001)            # wait for unlock
        else: pass #time.sleep(0.033) dont block

    _handle = None
    def setup_callback( self, context ):        # TODO replace with a proper frame update callback
        print('SERVER: setup frame update callback')
        if self._handle: return self._handle
        for area in bpy.context.window.screen.areas:
            if area.type == 'VIEW_3D':
                for reg in area.regions:
                    if reg.type == 'WINDOW':
                        # PRE_VIEW, POST_VIEW, POST_PIXEL
                        self._handle = reg.callback_add(self.threadsafe, (reg,), 'PRE_VIEW' )
                        self._area = area
                        self._region = reg
                        break
        if not self._handle:
            print('SERVER: FAILED to setup frame update callback')


def _create_stream_proto():
    proto = {}
    tags = 'ID NAME POSITION ROTATION SCALE DATA SELECTED TYPE MESH LAMP CAMERA ANIMATIONS DISTANCE ENERGY'.split()
    for i,tag in enumerate( tags ): proto[ tag ] = chr(i)		# up to 256
    return proto
STREAM_PROTO = _create_stream_proto()
STREAM_BUFFER_SIZE = 2048
TUNDRA_SCRIPT = '''
# this file was generated by blender2ogre #
import tundra, socket, select, pickle
STREAM_BUFFER_SIZE = 2048
globals().update( %s )
E = None    # this is just for debugging from the pyconsole
class Client(object):
    def __init__(self):
        self.socket = sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        host='localhost'; port = 9420
        sock.bind((host, port))
        self._animated = {}    # entity ID : { anim-name : weight }

    def update(self, delay):
        global E
        sock = self.socket
        poll = select.select( [ sock ], [], [], 0.01 )
        if not poll[0]: return True
        data = sock.recv( STREAM_BUFFER_SIZE )
        assert len(data) == STREAM_BUFFER_SIZE
        if not data:
            print( 'blender crashed?' )
            return
        header = data[ : 4]
        s = data[ 4 : int(header)+4 ]
        objects = pickle.loads( s )
        scn = tundra.Scene().MainCameraScene()	# replaces GetDefaultScene()
        for ob in objects:
            e = scn.GetEntityRaw( ob[ID] )
            if not e: continue
            x,y,z = ob[POSITION]
            e.placeable.SetPosition( x,y,z )
            x,y,z = ob[SCALE]
            e.placeable.SetScale( x,y,z )
            #e.placeable.SetOrientation( ob[ROTATION] )
            if ob[TYPE] == LAMP:
                e.light.range = ob[ DISTANCE ]
                e.light.brightness = ob[ ENERGY ]
                #e.light.diffColor = !! not wrapped !!
                #e.light.specColor = !! not wrapped !!
            if ANIMATIONS in ob:
                self.update_animation( e, ob )
            if not E: E = e

    def update_animation( self, e, ob ):
        if ob[ID] not in self._animated:
            self._animated[ ob[ID] ] = {}
        state = self._animated[ ob[ID] ]
        ac = e.animationcontroller
        for aname in ob[ ANIMATIONS ]:
            if aname not in state:      # save weight of new animation
                state[ aname ] = ob[ANIMATIONS][aname]  # weight
        for aname in state:
            if aname not in ob[ANIMATIONS] and ac.IsAnimationActive( aname ):
                ac.StopAnim( aname, '0.0' )
            elif aname in ob[ANIMATIONS]:
                weight = ob[ANIMATIONS][aname]
                if ac.HasAnimationFinished( aname ):
                    ac.PlayLoopedAnim( aname, '1.0', 'false' )      # PlayAnim(...) TODO single playback
                    ok = ac.SetAnimationWeight( aname, weight )
                    state[ aname ] = weight

                if weight != state[ aname ]:
                    ok = ac.SetAnimationWeight( aname, weight )
                    state[ aname ] = weight

client = Client()
tundra.Frame().connect( 'Updated(float)', client.update )
print('blender2ogre plugin ok')
''' %STREAM_PROTO

class TundraPipe(object):
    CONFIG_PATH = '/tmp/rex/plugins.xml'
    TUNDRA_SCRIPT_PATH = '/tmp/rex/b2ogre_plugin.py'
    CONFIG_XML = '''<?xml version="1.0"?>
<Tundra>
  <!-- plugins.xml is hardcoded to be the default configuration file to load if another file is not specified on the command line with the "config filename.xml" parameter. -->
  <plugin path="OgreRenderingModule" />
  <plugin path="EnvironmentModule" />           <!-- EnvironmentModule depends on OgreRenderingModule -->
  <plugin path="PhysicsModule" />               <!-- PhysicsModule depends on OgreRenderingModule and EnvironmentModule -->
  <plugin path="TundraProtocolModule" />        <!-- TundraProtocolModule depends on OgreRenderingModule -->
  <plugin path="JavascriptModule" />            <!-- JavascriptModule depends on TundraProtocolModule --> 
  <plugin path="AssetModule" />                 <!-- AssetModule depends on TundraProtocolModule -->
  <plugin path="AvatarModule" />                <!-- AvatarModule depends on AssetModule and OgreRenderingModule -->
  <plugin path="ECEditorModule" />              <!-- ECEditorModule depends on OgreRenderingModule, TundraProtocolModule, OgreRenderingModule and AssetModule -->
  <plugin path="SkyXHydrax" />                  <!-- SkyXHydrax depends on OgreRenderingModule -->
  <plugin path="OgreAssetEditorModule" />       <!-- OgreAssetEditorModule depends on OgreRenderingModule -->
  <plugin path="DebugStatsModule" />            <!-- DebugStatsModule depends on OgreRenderingModule, EnvironmentModule and AssetModule -->
  <plugin path="SceneWidgetComponents" />       <!-- SceneWidgetComponents depends on OgreRenderingModule and TundraProtocolModule -->
  <plugin path="PythonScriptModule" />

  <!-- TODO: Currently the above <plugin> items are loaded in the order they are specified, but below, the jsplugin items are loaded in an undefined order. Use the order specified here as the load order. -->
  <!-- NOTE: The startup .js scripts are specified only by base name of the file. Don's specify a path here. Place the startup .js scripts to /bin/jsmodules/startup/. -->
  <!-- Important: The file names specified here are case sensitive even on Windows! -->
  <jsplugin path="cameraapplication.js" />
  <jsplugin path="FirstPersonMouseLook.js" />
  <jsplugin path="MenuBar.js" />
  
  <!-- Python plugins -->
  <!-- <pyplugin path="lib/apitests.py" /> -->          <!-- Runs framework api tests -->
  <pyplugin path="%s" />         <!-- shows qt py console. could enable by default when add to menu etc. for controls, now just shows directly when is enabled here -->
  
  <option name="--accept_unknown_http_sources" />
  <option name="--accept_unknown_local_sources" />
  <option name="--fpslimit" value="60" />
  <!--  AssetAPI's file system watcher currently disabled because LocalAssetProvider implements
        the same functionality for LocalAssetStorages and HTTPAssetProviders do not yet support live-update. -->
  <option name="--nofilewatcher" />

</Tundra>''' %TUNDRA_SCRIPT_PATH

    def __init__(self, context, debug=False):
        self._physics_debug = True
        self._objects = []
        self.proc = None
        exe = None
        if 'Tundra.exe' in os.listdir( CONFIG['TUNDRA_ROOT'] ):
            exe = os.path.join( CONFIG['TUNDRA_ROOT'], 'Tundra.exe' )
        elif 'Tundra' in os.listdir( CONFIG['TUNDRA_ROOT'] ):
            exe = os.path.join( CONFIG['TUNDRA_ROOT'], 'Tundra' )

        cmd = []
        if not exe:
            print('ERROR: failed to find Tundra executable')
            assert 0
        elif sys.platform.startswith('win'):
            cmd.append(exe)
        else:
            if exe.endswith('.exe'): cmd.append('wine')     # assume user has Wine
            cmd.append( exe )
        if debug:
            cmd.append('--loglevel')
            cmd.append('debug')

        if CONFIG['TUNDRA_STREAMING']:
            cmd.append( '--config' )
            cmd.append( self.CONFIG_PATH )
            with open( self.CONFIG_PATH, 'wb' ) as fp: fp.write( bytes(self.CONFIG_XML,'utf-8') )
            with open( self.TUNDRA_SCRIPT_PATH, 'wb' ) as fp: fp.write( bytes(TUNDRA_SCRIPT,'utf-8') )
            self.server = Server()

        #cmd += ['--file', '/tmp/rex/preview.txml']     # tundra2.1.2 bug loading from --file ignores entity ID's
        cmd.append( '--storage' )
        cmd.append( '/tmp/rex' )
        self.proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)

        self.physics = True
        if self.proc:
            time.sleep(0.1)
            self.load( context, '/tmp/rex/preview.txml' )
            self.stop()

    def deselect_previously_updated(self, context):
        r = []
        for ob in context.selected_objects:
            if ob.name in self._objects: ob.select = False; r.append( ob )
        return r

    def load( self, context, url, clear=False ):
        self._objects += [ob.name for ob in context.selected_objects]
        if clear:
            self.proc.stdin.write( b'loadscene(/tmp/rex/preview.txml,true,true)\n')
        else:
            self.proc.stdin.write( b'loadscene(/tmp/rex/preview.txml,false,true)\n')
        try:
            self.proc.stdin.flush()
        except:
            global TundraSingleton
            TundraSingleton = None

    def start( self ):
        self.physics = True
        self.proc.stdin.write( b'startphysics\n' )
        try: self.proc.stdin.flush()
        except:
            global TundraSingleton
            TundraSingleton = None

    def stop( self ):
        self.physics = False
        self.proc.stdin.write( b'stopphysics\n' )
        try: self.proc.stdin.flush()
        except:
            global TundraSingleton
            TundraSingleton = None

    def toggle_physics_debug( self ):
        self._physics_debug = not self._physics_debug
        self.proc.stdin.write( b'physicsdebug\n' )
        try: self.proc.stdin.flush()
        except:
            global TundraSingleton
            TundraSingleton = None

    def exit(self):
        self.proc.stdin.write( b'exit\n' )
        self.proc.stdin.flush()
        global TundraSingleton
        TundraSingleton = None






###########################################
class MENU_preview_material_text(bpy.types.Menu):
    bl_label = 'preview'
    @classmethod
    def poll(self,context):
        if context.active_object and context.active_object.active_material: return True
    def draw(self, context):
        layout = self.layout
        mat = context.active_object.active_material
        if mat:
            #CONFIG['TOUCH_TEXTURES'] = False
            preview = generate_material( mat )
            for line in preview.splitlines():
                if line.strip():
                    for ww in wordwrap( line ): layout.label(text=ww)

@UI
class INFO_HT_myheader(bpy.types.Header):
    bl_space_type = 'INFO'
    def draw(self, context):
        layout = self.layout
        wm = context.window_manager
        window = context.window
        scene = context.scene
        rd = scene.render
        ob = context.active_object
        screen = context.screen

        #layout.separator()

        if _USE_JMONKEY_:
            row = layout.row(align=True)
            op = row.operator( 'jmonkey.preview', text='', icon='MONKEY' )

        if _USE_TUNDRA_:
            row = layout.row(align=True)
            op = row.operator( 'tundra.preview', text='', icon='WORLD' )
            if TundraSingleton:
                op = row.operator( 'tundra.preview', text='', icon='META_CUBE' )
                op.EX_SCENE = False
                if not TundraSingleton.physics:
                    op = row.operator( 'tundra.start_physics', text='', icon='PLAY' )
                else:
                    op = row.operator( 'tundra.stop_physics', text='', icon='PAUSE' )
                op = row.operator( 'tundra.toggle_physics_debug', text='', icon='MOD_PHYSICS' )
                op = row.operator( 'tundra.exit', text='', icon='CANCEL' )

        op = layout.operator( 'ogremeshy.preview', text='', icon='PLUGIN' ); op.mesh = True


        row = layout.row(align=True)
        sub = row.row(align=True)
        sub.menu("INFO_MT_file")
        sub.menu("INFO_MT_add")
        if rd.use_game_engine: sub.menu("INFO_MT_game")
        else: sub.menu("INFO_MT_render")

        row = layout.row(align=False); row.scale_x = 1.25
        row.menu("INFO_MT_instances", icon='NODETREE', text='')
        row.menu("INFO_MT_groups", icon='GROUP', text='')


        layout.template_header()
        if not context.area.show_menus:
            if window.screen.show_fullscreen: layout.operator("screen.back_to_previous", icon='SCREEN_BACK', text="Back to Previous")
            else: layout.template_ID(context.window, "screen", new="screen.new", unlink="screen.delete")
            layout.template_ID(context.screen, "scene", new="scene.new", unlink="scene.delete")

            layout.separator()
            layout.template_running_jobs()
            layout.template_reports_banner()
            layout.separator()
            if rd.has_multiple_engines: layout.prop(rd, "engine", text="")

            layout.label(text=scene.statistics())
            layout.menu( "INFO_MT_help" )

        else:
            layout.template_ID(context.window, "screen", new="screen.new", unlink="screen.delete")

            if ob:
                row = layout.row(align=True)
                row.prop( ob, 'name', text='' )
                row.prop( ob, 'draw_type', text='' )
                row.prop( ob, 'show_x_ray', text='' )
                row = layout.row()
                row.scale_y = 0.75; row.scale_x = 0.9
                row.prop( ob, 'layers', text='' )

            layout.separator()
            row = layout.row(align=True); row.scale_x = 1.1
            row.prop(scene.game_settings, 'material_mode', text='')
            row.prop(scene, 'camera', text='')

            layout.menu( 'MENU_preview_material_text', icon='TEXT', text='' )

            layout.menu( "INFO_MT_ogre_docs" )
            layout.operator("wm.window_fullscreen_toggle", icon='FULLSCREEN_ENTER', text="")
            if OgreToggleInterfaceOp.TOGGLE: layout.operator('ogre.toggle_interface', text='Ogre', icon='CHECKBOX_DEHLT')
            else: layout.operator('ogre.toggle_interface', text='Ogre', icon='CHECKBOX_HLT')


def export_menu_func_ogre(self, context):
    path,name = os.path.split( context.blend_data.filepath )
    op = self.layout.operator(INFO_OT_createOgreExport.bl_idname, text="Ogre3D (.scene and .mesh)")
    op.filepath = os.path.join( path, name.split('.')[0]+'.scene' )

def export_menu_func_realxtend(self, context):
    path,name = os.path.split( context.blend_data.filepath )
    op = self.layout.operator(INFO_OT_createRealxtendExport.bl_idname, text="RealXtend (.txml and .mesh)")
    op.filepath = os.path.join( path, name.split('.')[0]+'.txml' )


try: _header_ = bpy.types.INFO_HT_header
except:
    print('---blender2ogre addon enable---')

class OgreToggleInterfaceOp(bpy.types.Operator):
    '''Toggle Ogre UI'''
    bl_idname = "ogre.toggle_interface"
    bl_label = "Ogre UI"
    bl_options = {'REGISTER'}
    TOGGLE = True  #restore_minimal_interface()
    BLENDER_DEFAULT_HEADER = _header_
    @classmethod
    def poll(cls, context): return True
    def invoke(self, context, event):
        #global _header_
        if OgreToggleInterfaceOp.TOGGLE: #_header_:
            print( 'ogre.toggle_interface ENABLE' )
            bpy.utils.register_module(__name__)
            #_header_ = bpy.types.INFO_HT_header
            try: bpy.utils.unregister_class(_header_)
            except: pass
            bpy.utils.unregister_class( INFO_HT_microheader )   # moved to custom header
            OgreToggleInterfaceOp.TOGGLE = False
        else:
            print( 'ogre.toggle_interface DISABLE' )
            #bpy.utils.unregister_module(__name__); # this is not safe, can segfault blender, why?
            hide_user_interface()
            bpy.utils.register_class(_header_)
            restore_minimal_interface()
            OgreToggleInterfaceOp.TOGGLE = True

        return {'FINISHED'}



class INFO_HT_microheader(bpy.types.Header):
    bl_space_type = 'INFO'
    def draw(self, context):
        layout = self.layout
        try:
            if OgreToggleInterfaceOp.TOGGLE:
                layout.operator('ogre.toggle_interface', text='Ogre', icon='CHECKBOX_DEHLT')
            else:
                layout.operator('ogre.toggle_interface', text='Ogre', icon='CHECKBOX_HLT')
        except: pass    # STILL REQUIRED?

def get_minimal_interface_classes():
    return INFO_OT_createOgreExport, INFO_OT_createRealxtendExport, OgreToggleInterfaceOp, MiniReport, INFO_HT_microheader

_USE_TUNDRA_ = False
_USE_JMONKEY_ = False

def restore_minimal_interface():
    #if not hasattr( bpy.ops.ogre..   #always true
    for cls in get_minimal_interface_classes():
        try: bpy.utils.register_class( cls )
        except: pass
    return False

    try:
        bpy.utils.register_class( INFO_HT_microheader )
        for op in get_minimal_interface_classes(): bpy.utils.register_class( op )
        return False
    except:
        print( 'b2ogre minimal UI already setup' )
        return True




MyShaders = None
def register():
    print( '-'*80)
    print(VERSION)
    global MyShaders, _header_, _USE_TUNDRA_, _USE_JMONKEY_
    #bpy.utils.register_module(__name__)    ## do not load all the ogre panels by default
    #_header_ = bpy.types.INFO_HT_header
    #bpy.utils.unregister_class(_header_)
    restore_minimal_interface()

    ## only test for Tundra2 once - do not do this every panel redraw ##
    if os.path.isdir( CONFIG['TUNDRA_ROOT'] ): _USE_TUNDRA_ = True
    else: _USE_TUNDRA_ = False
    #if os.path.isdir( CONFIG['JMONKEY_ROOT'] ): _USE_JMONKEY_ = True
    #else: _USE_JMONKEY_ = False

    bpy.types.INFO_MT_file_export.append(export_menu_func_ogre)
    bpy.types.INFO_MT_file_export.append(export_menu_func_realxtend)

    if os.path.isdir( CONFIG['USER_MATERIALS'] ):
        scripts,progs = update_parent_material_path( CONFIG['USER_MATERIALS'] )
        for prog in progs:
            print('ogre shader program', prog.name)

    else: print( 'WARNING: invalid my-shaders path' )

    print( '-'*80)

def unregister():
    header = _header_
    print('unreg-> ogre exporter')
    bpy.utils.unregister_module(__name__)
    if header: bpy.utils.register_class(header)
    bpy.types.INFO_MT_file_export.remove(export_menu_func_ogre)
    bpy.types.INFO_MT_file_export.remove(export_menu_func_realxtend)


########### Tundra SkyX - TODO move to tundra.py #############

bpy.types.World.ogre_skyX = BoolProperty(
    name="enable sky", description="ogre sky",
    default=False
)

bpy.types.World.ogre_skyX_time = FloatProperty(
    name="Time Multiplier",
    description="change speed of day/night cycle", 
    default=0.3, min=0.0, max=5.0
)

bpy.types.World.ogre_skyX_wind = FloatProperty(
    name="Wind Direction",
    description="change direction of wind", 
    default=33.0, min=0.0, max=360.0
)

bpy.types.World.ogre_skyX_volumetric_clouds = BoolProperty(
    name="volumetric clouds", description="toggle ogre volumetric clouds",
    default=True
)
bpy.types.World.ogre_skyX_cloud_density_x = FloatProperty(
    name="Cloud Density X",
    description="change density of volumetric clouds on X", 
    default=0.1, min=0.0, max=5.0
)
bpy.types.World.ogre_skyX_cloud_density_y = FloatProperty(
    name="Cloud Density Y",
    description="change density of volumetric clouds on Y", 
    default=1.0, min=0.0, max=5.0
)

@UI
class OgreSkyPanel(bpy.types.Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "world"
    bl_label = "Ogre Sky Settings"
    @classmethod
    def poll(cls, context): return True
    def draw(self, context):
        layout = self.layout
        box = layout.box()
        box.prop( context.world, 'ogre_skyX' )
        if context.world.ogre_skyX:
            box.prop( context.world, 'ogre_skyX_time' )
            box.prop( context.world, 'ogre_skyX_wind' )
            box.prop( context.world, 'ogre_skyX_volumetric_clouds' )
            if context.world.ogre_skyX_volumetric_clouds:
                box.prop( context.world, 'ogre_skyX_cloud_density_x' )
                box.prop( context.world, 'ogre_skyX_cloud_density_y' )


class OgreProgram(object):
    '''
    parses .program scripts
    saves bytes to copy later

    self.name = name of program reference
    self.source = name of shader program (.cg, .glsl)

    '''
    def save( self, path ):
        print('saving program to', path)
        f = open( os.path.join(path,self.source), 'wb' )
        f.write(self.source_bytes )
        f.close()
        for name in self.includes:
            f = open( os.path.join(path,name), 'wb' )
            f.write( self.includes[name] )
            f.close()

    ######################################
    PROGRAMS = {}
    SOURCES = {}

    def reload(self):   # only one directory is allowed to hold shader programs
        if self.source not in os.listdir( CONFIG['SHADER_PROGRAMS'] ):
            print( 'ERROR: ogre material %s is missing source: %s' %(self.name,self.source) )
            print( CONFIG['SHADER_PROGRAMS'] )
            return False
        url = os.path.join(  CONFIG['SHADER_PROGRAMS'], self.source )
        print('shader source:', url)
        self.source_bytes = open( url, 'rb' ).read()#.decode('utf-8')
        print('shader source num bytes:', len(self.source_bytes))
        data = self.source_bytes.decode('utf-8')

        for line in data.splitlines():  # only cg shaders use the include macro?
            if line.startswith('#include') and line.count('"')==2:
                name = line.split()[-1].replace('"','').strip()
                print('shader includes:', name)
                url = os.path.join(  CONFIG['SHADER_PROGRAMS'], name )
                self.includes[ name ] = open( url, 'rb' ).read()
        return True

    def __init__(self, name='', data=''):
        self.name=name
        self.data = data.strip()
        self.source = None
        self.includes = {} # cg files may use #include something.cg

        if self.name in OgreProgram.PROGRAMS:
            print('---copy ogreprogram---', self.name)
            other = OgreProgram.PROGRAMS
            self.source = other.source
            self.data = other.data
            self.entry_point = other.entry_point
            self.profiles = other.profiles

        if data: self.parse( self.data )
        if self.name: OgreProgram.PROGRAMS[ self.name ] = self

    def parse( self, txt ):
        self.data = txt
        print('--parsing ogre shader program--' )
        for line in self.data.splitlines():
            print(line)
            line = line.split('//')[0]
            line = line.strip()
            if line.startswith('vertex_program') or line.startswith('fragment_program'):
                a, self.name, self.type = line.split()

            elif line.startswith('source'): self.source = line.split()[-1]
            elif line.startswith('entry_point'): self.entry_point = line.split()[-1]
            elif line.startswith('profiles'): self.profiles = line.split()[1:]

class OgreMaterialScript(object):
    def get_programs(self):
        progs = []
        for name in list(self.vertex_programs.keys()) + list(self.fragment_programs.keys()):
            p = get_shader_program( name )  # OgreProgram.PROGRAMS
            if p: progs.append( p )
        return progs

    def __init__(self, txt, url):
        self.url = url
        self.data = txt.strip()
        self.parent = None
        self.vertex_programs = {}
        self.fragment_programs = {}
        self.texture_units = {}
        self.texture_units_order = []
        self.passes = []

        line = self.data.splitlines()[0]
        assert line.startswith('material')
        if ':' in line:
            line, self.parent = line.split(':')
        self.name = line.split()[-1]
        print( 'new ogre material: %s' %self.name )

        brace = 0
        self.techniques = techs = []
        prog = None  # pick up program params
        tex = None  # pick up texture_unit options, require "texture" ?
        for line in self.data.splitlines():
            #print( line )
            rawline = line
            line = line.split('//')[0]
            line = line.strip()
            if not line: continue

            if line == '{': brace += 1
            elif line == '}': brace -= 1; prog = None; tex = None

            if line.startswith( 'technique' ):
                tech = {'passes':[]}; techs.append( tech )
                if len(line.split()) > 1: tech['technique-name'] = line.split()[-1]
            elif techs:
                if line.startswith('pass'):
                    P = {'texture_units':[], 'vprogram':None, 'fprogram':None, 'body':[]}
                    tech['passes'].append( P )
                    self.passes.append( P )

                elif tech['passes']:
                    P = tech['passes'][-1]
                    P['body'].append( rawline )

                    if line == '{' or line == '}': continue

                    if line.startswith('vertex_program_ref'):
                        prog = P['vprogram'] = {'name':line.split()[-1], 'params':{}}
                        self.vertex_programs[ prog['name'] ] = prog
                    elif line.startswith('fragment_program_ref'):
                        prog = P['fprogram'] = {'name':line.split()[-1], 'params':{}}
                        self.fragment_programs[ prog['name'] ] = prog

                    elif line.startswith('texture_unit'):
                        prog = None
                        tex = {'name':line.split()[-1], 'params':{}}
                        if tex['name'] == 'texture_unit': # ignore unnamed texture units
                            print('WARNING: material %s contains unnamed texture_units' %self.name)
                            print('---unnamed texture units will be ignored---')
                        else:
                            P['texture_units'].append( tex )
                            self.texture_units[ tex['name'] ] = tex
                            self.texture_units_order.append( tex['name'] )

                    elif prog:
                        p = line.split()[0]
                        if p=='param_named':
                            items = line.split()
                            if len(items) == 4: p, o, t, v = items
                            elif len(items) == 3:
                                p, o, v = items
                                t = 'class'
                            elif len(items) > 4:
                                o = items[1]; t = items[2]
                                v = items[3:]

                            opt = { 'name': o, 'type':t, 'raw_value':v }
                            prog['params'][ o ] = opt
                            if t=='float': opt['value'] = float(v)
                            elif t in 'float2 float3 float4'.split(): opt['value'] = [ float(a) for a in v ]
                            else: print('unknown type:', t)

                    elif tex:   # (not used)
                        tex['params'][ line.split()[0] ] = line.split()[ 1 : ]

        for P in self.passes:
            lines = P['body']
            while lines and ''.join(lines).count('{')!=''.join(lines).count('}'):
                if lines[-1].strip() == '}': lines.pop()
                else: break
            P['body'] = '\n'.join( lines )
            assert P['body'].count('{') == P['body'].count('}')     # if this fails, the parser choked

        #print( self.techniques )
        self.hidden_texture_units = rem = []
        for tex in self.texture_units.values():
            if 'texture' not in tex['params']:
                rem.append( tex )
        for tex in rem:
            print('WARNING: not using texture_unit because it lacks a "texture" parameter', tex['name'])
            self.texture_units.pop( tex['name'] )

        if len(self.techniques)>1:
            print('WARNING: user material %s has more than one technique' %self.url)


    def as_abstract_passes( self ):
        r = []
        for i,P in enumerate(self.passes):
            head = 'abstract pass %s/PASS%s' %(self.name,i)
            r.append( head + '\n' + P['body'] )
        return r


class MaterialScripts(object):
    ALL_MATERIALS = {}
    ENUM_ITEMS = []

    def __init__(self, url):
        self.url = url
        self.data = ''
        data = open( url, 'rb' ).read()
        try:
            self.data = data.decode('utf-8')
        except:
            self.data = data.decode('latin-1')

        self.materials = {}
        ## chop up .material file, find all material defs ####
        mats = []
        mat = []
        skip = False    # for now - programs must be defined in .program files, not in the .material
        for line in self.data.splitlines():
            if not line.strip(): continue
            a = line.split()[0]             #NOTE ".split()" strips white space
            if a == 'material':
                mat = []; mats.append( mat )
                mat.append( line )
            elif a in ('vertex_program', 'fragment_program', 'abstract'):
                skip = True
            elif mat and not skip:
                mat.append( line )
            elif skip and line=='}':
                skip = False

        ##########################
        for mat in mats:
            omat = OgreMaterialScript( '\n'.join( mat ), url )
            if omat.name in self.ALL_MATERIALS:
                print( 'WARNING: material %s redefined' %omat.name )
                #print( '--OLD MATERIAL--')
                #print( self.ALL_MATERIALS[ omat.name ].data )
                #print( '--NEW MATERIAL--')
                #print( omat.data )
            self.materials[ omat.name ] = omat
            self.ALL_MATERIALS[ omat.name ] = omat
            if omat.vertex_programs or omat.fragment_programs:  # ignore materials without programs
                self.ENUM_ITEMS.append( (omat.name, omat.name, url) )

    @classmethod    # only call after parsing all material scripts
    def reset_rna(self, callback=None):
        bpy.types.Material.ogre_parent_material = EnumProperty(
            name="Script Inheritence", 
            description='ogre parent material class',
            items=self.ENUM_ITEMS,
            #update=callback
        )


############################################################################
def is_image_postprocessed( image ):
    if CONFIG['FORCE_IMAGE_FORMAT'] != 'NONE' or image.use_resize_half or image.use_resize_absolute or image.use_color_quantize or image.use_convert_format:
        return True
    else:
        return False

class _image_processing_( object ):
    def _reformat( self, name, image ):
        if image.use_resize_half or image.use_resize_absolute:
            name = 'R.%s' %name
        if image.use_color_quantize:
            name = 'Q.%s' %name
        if image.use_convert_format:
            name = 'F.%s' %name
            if image.convert_format != 'NONE':
                name = '%s.%s' %(name[:name.rindex('.')], image.convert_format)
        elif CONFIG['FORCE_IMAGE_FORMAT'] != 'NONE':
            name = '%s.%s' %(name[:name.rindex('.')], CONFIG['FORCE_IMAGE_FORMAT'])
        return name

    def image_magick( self, texture, infile ):
        print('IMAGE MAGICK', infile )
        exe = CONFIG['IMAGE_MAGICK_CONVERT']
        if not os.path.isfile( exe ):
            Report.warnings.append( 'ImageMagick not installed!' )
            print( 'ERROR: can not find Image Magick - convert', exe ); return
        cmd = [ exe, infile ]
        ## enforce max size ##
        x,y = texture.image.size
        ax = texture.image.resize_x
        ay = texture.image.resize_y

        if texture.image.use_convert_format and texture.image.convert_format == 'jpg':
            cmd.append( '-quality' )
            cmd.append( '%s' %texture.image.jpeg_quality )

        if texture.image.use_resize_half:
            cmd.append( '-resize' )
            cmd.append( '%sx%s' %(x/2, y/2) )
        elif texture.image.use_resize_absolute and (x>ax or y>ay):
            cmd.append( '-resize' )
            cmd.append( '%sx%s' %(ax,ay) )

        elif x > CONFIG['MAX_TEXTURE_SIZE'] or y > CONFIG['MAX_TEXTURE_SIZE']:
            cmd.append( '-resize' )
            cmd.append( str(CONFIG['MAX_TEXTURE_SIZE']) )

        if texture.image.use_color_quantize:
            if texture.image.use_color_quantize_dither:
                cmd.append( '+dither' )
            cmd.append( '-colors' )
            cmd.append( str(texture.image.color_quantize) )

        path,name = os.path.split( infile )
        outfile = os.path.join( path, self._reformat(name,texture.image) )
        cmd.append( outfile )
        print( 'IMAGE MAGICK', cmd )
        subprocess.call( cmd )

    def DDS_converter(self, texture, infile ):
        print('[NVIDIA DDS Wrapper]', infile )
        exe = CONFIG['NVIDIATOOLS_EXE']
        if not os.path.isfile( exe ):
            Report.warnings.append( 'Nvidia DDS tools not installed!' )
            print( 'ERROR: can not find nvdxt.exe', exe ); return
        opts = '-quality_production -nmips %s -rescale nearest' %CONFIG['DDS_MIPS']
        path,name = os.path.split( infile )
        outfile = os.path.join( path, self._reformat(name,texture.image) )
        opts = opts.split() + ['-file', infile, '-output', '_tmp_.dds']
        if sys.platform.startswith('linux') or sys.platform.startswith('darwin') or sys.platform.startswith('freebsd'):
            subprocess.call( ['/usr/bin/wine', exe]+opts )
        else: subprocess.call( [exe]+opts )         ## TODO support OSX
        data = open( '_tmp_.dds', 'rb' ).read()
        f = open( outfile, 'wb' )
        f.write(data)
        f.close()



class OgreMaterialGenerator( _image_processing_ ):
    def __init__(self, material, path='/tmp', touch_textures=False ):
        self.material = material    # top level material
        self.path = path                # copy textures to path
        self.passes = []
        self.touch_textures = touch_textures
        if material.node_tree:
            nodes = bpyShaders.get_subnodes( self.material.node_tree, type='MATERIAL_EXT' )
            for node in nodes:
                if node.material:
                    self.passes.append( node.material )

    def get_active_programs(self):
        r = []
        for mat in self.passes:
            if mat.use_ogre_parent_material and mat.ogre_parent_material:
                usermat = get_ogre_user_material( mat.ogre_parent_material )
                for prog in usermat.get_programs(): r.append( prog )
        return r

    def get_header(self):
        r = []
        for mat in self.passes:
            if mat.use_ogre_parent_material and mat.ogre_parent_material:
                usermat = get_ogre_user_material( mat.ogre_parent_material )
                r.append( '//user material: %s' %usermat.name )
                for prog in usermat.get_programs():
                    r.append( prog.data )
                r.append( '// abstract passes //' )
                r += usermat.as_abstract_passes()
        return '\n'.join( r )


    def get_passes(self):
        r = []
        r.append( self.generate_pass(self.material) )
        for mat in self.passes:
            if mat.use_in_ogre_material_pass:             # submaterials
                r.append( self.generate_pass(mat) )
        return r


    def generate_pass( self, mat, pass_name=None ):
        usermat = texnodes = None
        if mat.use_ogre_parent_material and mat.ogre_parent_material:
            usermat = get_ogre_user_material( mat.ogre_parent_material )
            texnodes = bpyShaders.get_texture_subnodes( self.material, mat )

        M = ''
        if not pass_name: pass_name = 'b2ogre_%s'%time.time()
        if usermat:
            M += indent(2, 'pass %s : %s/PASS0' %(pass_name,usermat.name), '{' )
        else:
            M += indent(2, 'pass %s'%pass_name, '{' )

        color = mat.diffuse_color
        alpha = 1.0
        if mat.use_transparency: alpha = mat.alpha

        slots = get_image_textures( mat )        # returns texture_slot objects (CLASSIC MATERIAL)
        usealpha = False #mat.ogre_depth_write
        for slot in slots:
            #if slot.use_map_alpha and slot.texture.use_alpha: usealpha = True; break
            if slot.texture.use_alpha: usealpha = True; break

        ## force material alpha to 1.0 if textures use_alpha?
        #if usealpha: alpha = 1.0    # let the alpha of the texture control material alpha?

        if mat.use_fixed_pipeline:
            f = mat.ambient
            if mat.use_vertex_color_paint:
                M += indent(3, 'ambient vertexcolour' )
            else:        # fall back to basic material
                M += indent(3, 'ambient %s %s %s %s' %(color.r*f, color.g*f, color.b*f, alpha) )

            f = mat.diffuse_intensity
            if mat.use_vertex_color_paint:
                M += indent(3, 'diffuse vertexcolour' )
            else:        # fall back to basic material 
                M += indent(3, 'diffuse %s %s %s %s' %(color.r*f, color.g*f, color.b*f, alpha) )

            f = mat.specular_intensity
            s = mat.specular_color
            M += indent(3, 'specular %s %s %s %s %s' %(s.r*f, s.g*f, s.b*f, alpha, mat.specular_hardness/4.0) )

            f = mat.emit
            if mat.use_shadeless:     # requested by Borris
                M += indent(3, 'emissive %s %s %s 1.0' %(color.r, color.g, color.b) )
            elif mat.use_vertex_color_light:
                M += indent(3, 'emissive vertexcolour' )
            else:
                M += indent(3, 'emissive %s %s %s %s' %(color.r*f, color.g*f, color.b*f, alpha) )

        if mat.offset_z:
            M += indent(3, 'depth_bias %s'%mat.offset_z )

        for name in dir(mat):   #mat.items() - items returns custom props not pyRNA:
            if name.startswith('ogre_') and name != 'ogre_parent_material':
                var = getattr(mat,name)
                op = name.replace('ogre_', '')
                val = var
                if type(var) == bool:
                    if var: val = 'on'
                    else: val = 'off'
                M += indent( 3, '%s %s' %(op,val) )


        if texnodes and usermat.texture_units:
            for i,name in enumerate(usermat.texture_units_order):
                if i<len(texnodes):
                    node = texnodes[i]
                    if node.texture:
                        geo = bpyShaders.get_connected_input_nodes( self.material, node )[0]
                        M += self.generate_texture_unit( node.texture, name=name, uv_layer=geo.uv_layer )

        #if mat.use_fixed_pipeline:
        elif slots:
            for slot in slots:
                M += self.generate_texture_unit( slot.texture, slot=slot )

        M += indent(2, '}' )    # end pass

        return M

    def generate_texture_unit(self, texture, slot=None, name=None, uv_layer=None):
        if not hasattr(texture, 'image'):
            print('WARNING: texture must be of type IMAGE->', texture)
            return ''
        if not texture.image:
            print('WARNING: texture has no image assigned->', texture)
            return ''
        #if slot: print(dir(slot))
        if slot and not slot.use: return ''

        path = self.path    #CONFIG['PATH']

        M = ''; _alphahack = None
        if not name: name = 'b2ogre_%s' %time.time()
        M += indent(3, 'texture_unit %s' %name, '{' )

        if texture.library: # support library linked textures
            libpath = os.path.split( bpy.path.abspath(texture.library.filepath) )[0]
            iurl = bpy.path.abspath( texture.image.filepath, libpath )
        else:
            iurl = bpy.path.abspath( texture.image.filepath )

        postname = texname = os.path.split(iurl)[-1]
        destpath = path

        if texture.image.packed_file:
            orig = texture.image.filepath
            iurl = os.path.join(path, texname)
            if '.' not in iurl:
                print('WARNING: packed image is of unknown type - assuming PNG format')
                iurl += '.png'
                texname = postname = os.path.split(iurl)[-1]

            if not os.path.isfile( iurl ):
                if self.touch_textures:
                    print('MESSAGE: unpacking image: ', iurl)
                    texture.image.filepath = iurl
                    texture.image.save()
                    texture.image.filepath = orig
            else:
                print('MESSAGE: packed image already in temp, not updating', iurl)

        if is_image_postprocessed( texture.image ):
            postname = self._reformat( texname, texture.image )
            print('MESSAGE: image postproc',postname)

        M += indent(4, 'texture %s' %postname )    

        exmode = texture.extension
        if exmode in TextureUnit.tex_address_mode:
            M += indent(4, 'tex_address_mode %s' %TextureUnit.tex_address_mode[exmode] )


        # TODO - hijack nodes for better control?
        if slot:        # classic blender material slot options
            if exmode == 'CLIP': M += indent(4, 'tex_border_colour %s %s %s' %(slot.color.r, slot.color.g, slot.color.b) )    
            M += indent(4, 'scale %s %s' %(1.0/slot.scale.x, 1.0/slot.scale.y) )
            if slot.texture_coords == 'REFLECTION':
                if slot.mapping == 'SPHERE':
                    M += indent(4, 'env_map spherical' )
                elif slot.mapping == 'FLAT':
                    M += indent(4, 'env_map planar' )
                else: print('WARNING: <%s> has a non-UV mapping type (%s) and not picked a proper projection type of: Sphere or Flat' %(texture.name, slot.mapping))

            ox,oy,oz = slot.offset
            if ox or oy:
                M += indent(4, 'scroll %s %s' %(ox,oy) )
            if oz:
                M += indent(4, 'rotate %s' %oz )

            #if slot.use_map_emission:    # problem, user will want to use emission sometimes
            if slot.use_from_dupli:    # hijacked again - june7th
                M += indent(4, 'rotate_anim %s' %slot.emission_color_factor )
            if slot.use_map_scatter:    # hijacked from volume shaders
                M += indent(4, 'scroll_anim %s %s ' %(slot.density_factor, slot.emission_factor) )

            if slot.uv_layer:
                idx = find_uv_layer_index( slot.uv_layer, self.material )
                M += indent(4, 'tex_coord_set %s' %idx)

            rgba = False
            if texture.image.depth == 32: rgba = True
            btype = slot.blend_type     # TODO - fix this hack if/when slots support pyRNA
            ex = False; texop = None
            if btype in TextureUnit.colour_op:
                if btype=='MIX' and slot.use_map_alpha and not slot.use_stencil:
                    if slot.diffuse_color_factor >= 1.0: texop = 'alpha_blend'
                    else:
                        texop = TextureUnit.colour_op[ btype ]
                        ex = True
                elif btype=='MIX' and slot.use_map_alpha and slot.use_stencil:
                    texop = 'blend_current_alpha'; ex=True
                elif btype=='MIX' and not slot.use_map_alpha and slot.use_stencil:
                    texop = 'blend_texture_alpha'; ex=True
                else:
                    texop = TextureUnit.colour_op[ btype ]
            elif btype in TextureUnit.colour_op_ex:
                    texop = TextureUnit.colour_op_ex[ btype ]
                    ex = True

            if texop and ex:
                if texop == 'blend_manual':
                    factor = 1.0 - slot.diffuse_color_factor
                    M += indent(4, 'colour_op_ex %s src_texture src_current %s' %(texop, factor) )
                else:
                    M += indent(4, 'colour_op_ex %s src_texture src_current' %texop )
            elif texop:
                    M += indent(4, 'colour_op %s' %texop )

        else:
            if uv_layer:
                idx = find_uv_layer_index( uv_layer )
                M += indent(4, 'tex_coord_set %s' %idx)


        M += indent(3, '}' )
        ###############

        if self.touch_textures:
            ## copy texture only if newer ##
            if not os.path.isfile( iurl ): Report.warnings.append( 'missing texture: %s' %iurl )
            else:
                desturl = os.path.join( destpath, texname )
                if not os.path.isfile( desturl ) or os.stat( desturl ).st_mtime < os.stat( iurl ).st_mtime:
                    f = open( desturl, 'wb' )
                    f.write( open(iurl,'rb').read() )
                    f.close()
                if is_image_postprocessed( texture.image ):     # TODO allow imagemagick to operate first, then DDS-convert
                    if CONFIG['FORCE_IMAGE_FORMAT'] == 'dds': self.DDS_converter( texture, desturl )
                    else: self.image_magick( texture, desturl )

        return M



class TextureUnit(object):
    colour_op = {
        'MIX'       :   'replace',
        'ADD'     :   'add',
        'MULTIPLY' : 'modulate',
        #'alpha_blend' : '',
    }
    colour_op_ex = {
        'MIX'       :    'blend_manual',
        'SCREEN': 'modulate_x2',
        'LIGHTEN': 'modulate_x4',
        'SUBTRACT': 'subtract',
        'OVERLAY':    'add_signed',
        'DIFFERENCE': 'dotproduct',        # best match?
        'VALUE': 'blend_diffuse_colour',
    }

    tex_address_mode = {
        'REPEAT': 'wrap',
        'EXTEND': 'clamp',
        'CLIP'       : 'border',
        'CHECKER' : 'mirror'
    }







#####################################################################################
###################################### Public API #####################################
def material_name( mat ):
    if type(mat) is str: return mat
    elif not mat.library: return mat.name
    else: return mat.name + mat.library.filepath.replace('/','_')


def export_mesh( ob, path='/tmp', force_name=None, ignore_shape_animation=False, normals=True ):
    ''' returns materials used by the mesh '''
    return dot_mesh( ob, path, force_name, ignore_shape_animation, normals )


def generate_material( mat, path='/tmp', copy_programs=False, touch_textures=False ):
    ''' returns generated material string '''
    safename = material_name(mat)     # supports blender library linking
    M = '// blender material: %s\n' %(mat.name)
    M += 'material %s \n{\n'        %safename
    if mat.use_shadows: M += indent(1, 'receive_shadows on')
    else: M += indent(1, 'receive_shadows off')
    M += indent(1, 'technique b2ogre_%s'%time.time(), '{' )    # technique GLSL, CG
    w = OgreMaterialGenerator( mat, path=path, touch_textures=touch_textures )
    if copy_programs:
        progs = w.get_active_programs()
        for prog in progs: prog.save( path )

    header = w.get_header()
    passes = w.get_passes()
    M += '\n'.join( passes )
    M += indent(1, '}' )    # end technique
    M += '}\n'    # end material
    return header + '\n' + M

def get_ogre_user_material( name ):
    #print('get_ogre_user_material(%s)'%name)
    if name in MaterialScripts.ALL_MATERIALS:
        return MaterialScripts.ALL_MATERIALS[ name ]

def get_shader_program( name ):
    if name in OgreProgram.PROGRAMS:
        return OgreProgram.PROGRAMS[ name ]
    else:
        print('WARNING: no shader program named: %s' %name)

def get_shader_programs():
    return OgreProgram.PROGRAMS.values()


def parse_material_and_program_scripts( path, scripts, progs, missing ):   # recursive
    for name in os.listdir(path):
        url = os.path.join(path,name)
        if os.path.isdir( url ):
            parse_material_and_program_scripts( url, scripts, progs, missing )

        elif os.path.isfile( url ):
            if name.endswith( '.material' ):
                print( '<found material>', url )
                scripts.append( MaterialScripts( url ) )

            if name.endswith('.program'):
                print( '<found program>', url )
                data = open( url, 'rb' ).read().decode('utf-8')

                chk = []; chunks = [ chk ]
                for line in data.splitlines():
                    line = line.split('//')[0]
                    if line.startswith('}'):
                        chk.append( line )
                        chk = []; chunks.append( chk )
                    elif line.strip():
                        chk.append( line )

                for chk in chunks:
                    if not chk: continue
                    p = OgreProgram( data='\n'.join(chk) )
                    if p.source:
                        ok = p.reload()
                        if not ok: missing.append( p )
                        else: progs.append( p )


## updates RNA ##
def update_parent_material_path( path ):
    print( '>>SEARCHING FOR OGRE MATERIALS: %s' %path )
    scripts = []
    progs = []
    missing = []
    parse_material_and_program_scripts( path, scripts, progs, missing )

    if missing:
        print('WARNING: missing shader programs:')
        for p in missing: print(p.name)
    if missing and not progs:
        print('WARNING: no shader programs were found - set "SHADER_PROGRAMS" to your path')

    MaterialScripts.reset_rna( callback=bpyShaders.on_change_parent_material )
    return scripts, progs




def get_subcollision_meshes():
    ''' returns all collision meshes found in the scene '''
    r = []
    for ob in bpy.context.scene.objects:
        if ob.type=='MESH' and ob.subcollision: r.append( ob )
    return r

def get_objects_with_subcollision():
    ''' returns objects that have active sub-collisions '''
    r = []
    for ob in bpy.context.scene.objects:
        if ob.type=='MESH' and ob.collision_mode not in ('NONE', 'PRIMITIVE'):
            r.append( ob )
    return r

def get_subcollisions(ob):
    prefix = '%s.' %ob.collision_mode
    r = []
    for child in ob.children:
        if child.subcollision and child.name.startswith( prefix ):
            r.append( child )
    return r



class bpyShaders(bpy.types.Operator):
    '''operator: enables material nodes (workaround for not having IDPointers in pyRNA)'''  
    bl_idname = "ogre.force_setup_material_passes"  
    bl_label = "force bpyShaders"
    bl_options = {'REGISTER'}

    @classmethod
    def poll(cls, context):
        if context.active_object and context.active_object.active_material: return True
    def invoke(self, context, event):
        mat = context.active_object.active_material
        mat.use_material_passes = True
        bpyShaders.create_material_passes( mat )
        return {'FINISHED'}


    ## setup from MaterialScripts.reset_rna( callback=bpyShaders.on_change_parent_material )
    @staticmethod
    def on_change_parent_material(mat,context):
        print(mat,context)
        print('callback', mat.ogre_parent_material)

    @staticmethod
    def get_subnodes(mat, type='TEXTURE'):
        d = {}
        for node in mat.nodes:
            if node.type==type: d[node.name] = node
        keys = list(d.keys())
        keys.sort()
        r = []
        for key in keys: r.append( d[key] )
        return r


    @staticmethod
    def get_texture_subnodes( parent, submaterial=None ):
        if not submaterial: submaterial = parent.active_node_material
        d = {}
        for link in parent.node_tree.links:
            if link.from_node and link.from_node.type=='TEXTURE':
                if link.to_node and link.to_node.type == 'MATERIAL_EXT':
                    if link.to_node.material:
                        if link.to_node.material.name == submaterial.name:
                            node = link.from_node
                            d[node.name] = node
        keys = list(d.keys())           # this breaks if the user renames the node - TODO improve me
        keys.sort()
        r = []
        for key in keys: r.append( d[key] )
        return r

    @staticmethod
    def get_connected_input_nodes( material, node ):
        r = []
        for link in material.node_tree.links:
            if link.to_node and link.to_node.name == node.name:
                r.append( link.from_node )
        return r

    @staticmethod
    def get_or_create_material_passes( mat, n=8 ):
        if not mat.node_tree:
            print('CREATING MATERIAL PASSES', n)
            bpyShaders.create_material_passes( mat, n )

        d = {}      # funky, blender259 had this in order, now blender260 has random order
        for node in mat.node_tree.nodes:
            if node.type == 'MATERIAL_EXT' and node.name.startswith('GEN.'):
                d[node.name] = node
        keys = list(d.keys())
        keys.sort()
        r = []
        for key in keys: r.append( d[key] )
        return r

    @staticmethod
    def get_or_create_texture_nodes( mat, n=6 ):    # currently not used
        #print('bpyShaders.get_or_create_texture_nodes( %s, %s )' %(mat,n))
        assert mat.node_tree    # must call create_material_passes first
        m = []
        for node in mat.node_tree.nodes:
            if node.type == 'MATERIAL_EXT' and node.name.startswith('GEN.'):
                m.append( node )
        if not m:
            m = bpyShaders.get_or_create_material_passes(mat)
        print(m)
        r = []
        for link in mat.node_tree.links:
            print(link, link.to_node, link.from_node)
            if link.to_node and link.to_node.name.startswith('GEN.') and link.from_node.type=='TEXTURE':
                r.append( link.from_node )
        if not r:
            print('--missing texture nodes--')
            r = bpyShaders.create_texture_nodes( mat, n )
        return r


    @staticmethod
    def create_material_passes( mat, n=8, textures=True ):
        #print('bpyShaders.create_material_passes( %s, %s )' %(mat,n))
        mat.use_nodes = True
        tree = mat.node_tree	# valid pointer now

        nodes = bpyShaders.get_subnodes( tree, 'MATERIAL' )  # assign base material
        if nodes and not nodes[0].material:
            nodes[0].material = mat

        r = []
        x = 680
        for i in range( n ):
            node = tree.nodes.new( type='MATERIAL_EXT' ) #[‘OUTPUT’, ‘MATERIAL’, ‘RGB’, ‘VALUE’, ‘MIX_RGB’, ‘VALTORGB’, ‘RGBTOBW’, ‘TEXTURE’, ‘NORMAL’, ‘GEOMETRY’, ‘MAPPING’, ‘CURVE_VEC’, ‘CURVE_RGB’, ‘CAMERA’, ‘MATH’, ‘VECT_MATH’, ‘SQUEEZE’, ‘MATERIAL_EXT’, ‘INVERT’, ‘SEPRGB’, ‘COMBRGB’, ‘HUE_SAT’, ‘SCRIPT’, ‘GROUP’]
            node.name = 'GEN.%s' %i
            node.location.x = x; node.location.y = 640
            r.append( node )
            x += 220
        #mat.use_nodes = False  # TODO set user material to default output
        if textures:
            texnodes = bpyShaders.create_texture_nodes( mat )
            print( texnodes )
        return r

    @staticmethod
    def create_texture_nodes( mat, n=6, geoms=True ):
        #print('bpyShaders.create_texture_nodes( %s )' %mat)
        assert mat.node_tree    # must call create_material_passes first
        mats = bpyShaders.get_or_create_material_passes( mat )
        r = {}; x = 400
        for i,m in enumerate(mats):
            r['material'] = m; r['textures'] = []; r['geoms'] = []
            inputs = []     # other inputs mess up material preview #
            for tag in ['Mirror', 'Ambient', 'Emit', 'SpecTra', 'Ray Mirror', 'Translucency']:
                inputs.append( m.inputs[ tag ] )
            for j in range(n):
                tex = mat.node_tree.nodes.new( type='TEXTURE' )
                tex.name = 'TEX.%s.%s' %(j, m.name)
                tex.location.x = x - (j*16)
                tex.location.y = -(j*230)
                input = inputs[j]; output = tex.outputs['Color']
                link = mat.node_tree.links.new( input, output )
                r['textures'].append( tex )

                if geoms:
                    geo = mat.node_tree.nodes.new( type='GEOMETRY' )
                    link = mat.node_tree.links.new( tex.inputs['Vector'], geo.outputs['UV'] )
                    geo.location.x = x - (j*16) - 250
                    geo.location.y = -(j*250) - 1500
                    r['geoms'].append( geo )
            x += 220
        return r


@UI
class PANEL_node_editor_ui( bpy.types.Panel ):
    bl_space_type = 'NODE_EDITOR'
    bl_region_type = 'UI'
    bl_label = "Ogre Material"
    @classmethod
    def poll(self,context):
        if context.space_data.id: return True
    def draw(self, context):
        layout = self.layout
        topmat = context.space_data.id             # the top level node_tree
        mat = topmat.active_node_material        # the currently selected sub-material
        if not mat or topmat.name == mat.name:
            self.bl_label = topmat.name
            if not topmat.use_material_passes:
                layout.operator(
                    'ogre.force_setup_material_passes', 
                    text="Ogre Material Layers", 
                    icon='SCENE_DATA' 
                )
            ogre_material_panel( layout, topmat, show_programs=False )
        elif mat:
            self.bl_label = mat.name
            ogre_material_panel( layout, mat, topmat, show_programs=False )


@UI
class PANEL_node_editor_ui_extra( bpy.types.Panel ):
    bl_space_type = 'NODE_EDITOR'
    bl_region_type = 'UI'
    bl_label = "Ogre Material Advanced"
    bl_options = {'DEFAULT_CLOSED'}
    @classmethod
    def poll(self,context):
        if context.space_data.id: return True
    def draw(self, context):
        layout = self.layout
        topmat = context.space_data.id             # the top level node_tree
        mat = topmat.active_node_material        # the currently selected sub-material
        if mat:
            self.bl_label = mat.name + ' (advanced)'
            ogre_material_panel_extra( layout, mat )
        else:
            self.bl_label = topmat.name + ' (advanced)'
            ogre_material_panel_extra( layout, topmat )




def ogre_material_panel_extra( parent, mat ):
    box = parent.box()
    header = box.row()

    if mat.use_fixed_pipeline:
        header.prop( mat, 'use_fixed_pipeline', text='Fixed Pipeline', icon='LAMP_SUN' )

        row = box.row()
        row.prop(mat, "use_vertex_color_paint", text="Vertex Colors")
        row.prop(mat, "use_shadeless")
        if mat.use_shadeless and not mat.use_vertex_color_paint:
            row = box.row()
            row.prop(mat, "diffuse_color", text='')
        elif not mat.use_shadeless:
            if not mat.use_vertex_color_paint:
                row = box.row()
                row.prop(mat, "diffuse_color", text='')
                row.prop(mat, "diffuse_intensity", text='intensity')
            row = box.row()
            row.prop(mat, "specular_color", text='')
            row.prop(mat, "specular_intensity", text='intensity')
            row = box.row()
            row.prop(mat, "specular_hardness")
            row = box.row()
            row.prop(mat, "ambient")
            #row = box.row()
            row.prop(mat, "emit")
        box.prop(mat, 'use_ogre_advanced_options', text='---guru options---' )

    else:
        header.prop( mat, 'use_fixed_pipeline', text='', icon='LAMP_SUN' )
        header.prop(mat, 'use_ogre_advanced_options', text='---guru options---' )

    if mat.use_ogre_advanced_options:
        box.prop(mat, 'offset_z')
        box.prop(mat, "use_shadows")
        box.prop(mat, 'ogre_depth_write' )

        for tag in 'ogre_colour_write ogre_lighting ogre_normalise_normals ogre_light_clip_planes ogre_light_scissor ogre_alpha_to_coverage ogre_depth_check'.split():
            box.prop(mat, tag)

        for tag in 'ogre_polygon_mode ogre_shading ogre_cull_hardware ogre_transparent_sorting ogre_illumination_stage ogre_depth_func ogre_scene_blend_op'.split():
            box.prop(mat, tag)


def ogre_material_panel( layout, mat, parent=None, show_programs=True ):
    box = layout.box()
    header = box.row()
    header.prop(mat, 'ogre_scene_blend', text='')
    if mat.ogre_scene_blend and 'alpha' in mat.ogre_scene_blend:
        row = box.row()
        if mat.use_transparency:
            row.prop(mat, "use_transparency", text='')
            row.prop(mat, "alpha")
        else:
            row.prop(mat, "use_transparency", text='Transparent')
    if not parent: return   # only allow on pass1 and higher

    header.prop(mat, 'use_ogre_parent_material', icon='FILE_SCRIPT', text='')

    if mat.use_ogre_parent_material:
        row = box.row()
        row.prop(mat, 'ogre_parent_material', text='')

        s = get_ogre_user_material( mat.ogre_parent_material )  # gets by name
        if s and (s.vertex_programs or s.fragment_programs):
            progs = s.get_programs()

            split = box.row()

            texnodes = None
            if parent:
                texnodes = bpyShaders.get_texture_subnodes( parent, submaterial=mat )
            elif mat.node_tree:
                texnodes = bpyShaders.get_texture_subnodes( mat )   # assume toplevel

            if not progs:
                bx = split.box()
                bx.label( text='(missing shader programs)', icon='ERROR' )
            elif s.texture_units and texnodes:
                bx = split.box()
                for i,name in enumerate(s.texture_units_order):
                    if i<len(texnodes):
                        row = bx.row()
                        #row.label( text=name )
                        tex = texnodes[i]
                        row.prop( tex, 'texture', text=name )
                        if parent:
                            inputs = bpyShaders.get_connected_input_nodes( parent, tex )
                            if inputs:
                                geo = inputs[0]
                                assert geo.type == 'GEOMETRY'
                                row.prop( geo, 'uv_layer', text='UV' )
                    else:
                        print('WARNING: no slot for texture unit:', name)

            if show_programs and (s.vertex_programs or s.fragment_programs):
                bx = box.box()
                for name in s.vertex_programs:
                    bx.label( text=name )
                for name in s.fragment_programs:
                    bx.label( text=name )


###########################################################################
if __name__ == "__main__":  # allows directly running by "blender --python blender2ogre.py"
    register()

