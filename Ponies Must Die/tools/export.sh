#!/bin/sh

if [ $# -ne 2 ]; then
    echo "Usage: export.sh in.blend out"
    echo "out must be a directory"
    exit 1
fi

DIR=$(dirname $0)
FILE_IN=$1
DIR_OUT=$2

if [ ! -d $DIR_OUT ]; then
    echo "The output path must be a directory"
    exit 2
fi

for i in blender blender-2.60
do
    which $i > /dev/null
    if [ $? -eq 0 ]; then
	BLENDER=$i
    fi
done

if [ -z $BLENDER ]; then
    echo "Error: blender not found"
    exit 3
fi

$BLENDER -b $FILE_IN -P $DIR/io_export_ogreDotScene.py -P $DIR/export.py -- $DIR_OUT/
