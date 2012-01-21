import bpy
import sys, os, zipfile

target = sys.argv[-1]
tmpDir = target+"tmp"

os.mkdir(tmpDir)
filename, ext = os.path.splitext(os.path.basename(target))
bpy.ops.ogre.export(filepath=os.path.join(tmpDir,filename), EX_SELONLY=False, EX_SCENE=False, EX_SEP_MATS=False)

outFile = zipfile.ZipFile(target, "w")

for f in os.listdir(tmpDir):
	if f.split('.')[-1] != 'xml':
		outFile.write(os.path.join(tmpDir,f), f if f != '.material' else 'materials.material')
	os.remove(os.path.join(tmpDir,f))
os.rmdir(tmpDir)
