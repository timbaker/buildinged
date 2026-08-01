SRC=$(pwd)/..
BUILD=$SRC/../build-buildinged-docker
APPIMAGE=$SRC/../linuxdeploy-tiled/BuildingEd-x86_64.AppImage
DESTROOT=$SRC/../dist-buildinged-docker
DEST=$DESTROOT/BuildingEd

mkdir $DESTROOT
mkdir $DEST
cp -a $APPIMAGE $DEST
cp -ar $BUILD/share/ $DEST

cp -a $SRC/dist/BuildingEd-x86_64.AppImage.sh $DEST
chmod +x $DEST/BuildingEd-x86_64.AppImage.sh

