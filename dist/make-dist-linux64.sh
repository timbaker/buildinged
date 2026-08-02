SRC=$(pwd)/..
BUILD=$SRC/../build-buildinged-docker
APPIMAGE=$SRC/../linuxdeploy-buildinged/BuildingEd-x86_64.AppImage
DESTROOT=$SRC/../dist-tiled-docker
DEST=$DESTROOT/BuildingEd

mkdir -p $DESTROOT
mkdir -p $DEST
cp -a $APPIMAGE $DEST
cp -ar $BUILD/share/ $DEST

cp -a $SRC/dist/BuildingEd-x86_64.AppImage.sh $DEST
chmod +x $DEST/BuildingEd-x86_64.AppImage.sh

cp -a $SRC/LICENSE.APACHE $DEST
cp -a $SRC/LICENSE.BSD $DEST
cp -a $SRC/LICENSE.GPL $DEST
cp -a $SRC/LICENSE.QT6 $DEST
