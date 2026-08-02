SRC=$(pwd)/..
BUILD=$SRC/../Qt_6_11_1_for_macOS_Release/bin
DESTROOT=$SRC/../../ProjectZomboid
DEST=$DESTROOT/BuildingEd

mkdir -p $DESTROOT
mkdir -p $DEST

cp -a $BUILD/BuildingEd.app $DEST

cp -a $SRC/LICENSE.APACHE $DEST
cp -a $SRC/LICENSE.BSD $DEST
cp -a $SRC/LICENSE.GPL $DEST
cp -a $SRC/LICENSE.QT6 $DEST

