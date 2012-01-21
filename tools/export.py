import bpy
import sys
bpy.ops.ogre.export(filepath=sys.argv[-1], EX_SELONLY=False, EX_SCENE=False, EX_SEP_MATS=False)
