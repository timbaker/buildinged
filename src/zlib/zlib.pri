INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
macx {
    LIBS += -L$$top_builddir/bin/BuildingEd.app/Contents/Frameworks -lzlib1
}
!macx {
    LIBS += -L$$top_builddir/lib -lzlib1
}