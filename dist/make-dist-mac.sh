SRC=$(pwd)/..
BUILD=$SRC/../Qt_6_11_1_for_macOS-Release/bin/
DESTROOT=$SRC/../../ProjectZomboid
DEST=$DESTROOT/BuildingEd

mkdir $DESTROOT
mkdir $DEST

cp -ra $BUILD/BuildingEd.app $DEST

cp -a $SRC/LICENSE.APACHE.txt $DEST
cp -a $SRC/LICENSE.BSD.txt $DEST
cp -a $SRC/LICENSE.GPL.txt $DEST
cp -a $SRC/LICENSE.QT6 $DEST

