#!/bin/bash
SRC=$(pwd)/..
BUILD=$(realpath $SRC/../Qt_6_11_1_for_macOS_Release/bin)
DESTROOT=$(realpath $SRC/../../ProjectZomboid)
DEST=$DESTROOT/BuildingEd

mkdir -p $DESTROOT
mkdir -p $DEST

rm -rf $DEST/BuildingEd.app
cp -a $BUILD/BuildingEd.app $DEST

# Add all necessary Qt libraries
~/Qt/6.11.1/macOS/bin/macdeployqt $DEST/BuildingEd.app

# Remove all symlinks, they won't work on Steam
find $DEST/BuildingEd.app/Contents -type l -delete

mv $DEST/BuildingEd.app/Contents/Frameworks/libtiled.1.0.0.dylib $DEST/BuildingEd.app/Contents/Frameworks/libtiled.1.dylib
mv $DEST/BuildingEd.app/Contents/Frameworks/libzlib1.1.0.0.dylib $DEST/BuildingEd.app/Contents/Frameworks/libzlib1.1.dylib

cp -a $SRC/LICENSE.APACHE $DEST
cp -a $SRC/LICENSE.BSD $DEST
cp -a $SRC/LICENSE.GPL $DEST
cp -a $SRC/LICENSE.QT6 $DEST

