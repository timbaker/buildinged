include(../../tiled.pri)
include(../libtiled/libtiled.pri)
include(../qtsingleapplication/qtsingleapplication.pri)
include(../qtlockedfile/qtlockedfile.pri)

TEMPLATE = app
TARGET = BuildingEd
isEmpty(INSTALL_ONLY_BUILD) {
    target.path = $${PREFIX}/bin
    INSTALLS += target
}
win32 {
    DESTDIR = ../..
} else {
    DESTDIR = ../../bin
}

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}
contains(QT_CONFIG, opengl): QT += opengl

DEFINES += QT_NO_CAST_FROM_ASCII \
    QT_NO_CAST_TO_ASCII
DEFINES += ZOMBOID BUILDINGED_SA

macx {
    QMAKE_LIBDIR_FLAGS += -L$$OUT_PWD/../../bin/BuildingEd.app/Contents/Frameworks
    LIBS += -framework Foundation
} else:win32 {
    LIBS += -L$$OUT_PWD/../../lib
} else {
    QMAKE_LIBDIR_FLAGS += -L$$OUT_PWD/../../lib
}

# Make sure the executable can find libtiled
!win32:!macx {
    QMAKE_RPATHDIR += \$\$ORIGIN/../lib

    # It is not possible to use ORIGIN in QMAKE_RPATHDIR, so a bit manually
    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$$join(QMAKE_RPATHDIR, ":")\'
    QMAKE_RPATHDIR =
}

#MOC_DIR = .moc
#UI_DIR = .uic
#RCC_DIR = .rcc
#OBJECTS_DIR = .obj

SOURCES += colorbutton.cpp \
    filesystemwatcher.cpp \
    mapcomposite.cpp \
    mapmanager.cpp \
    resizedialog.cpp \
    threads.cpp \
    tilemetainfodialog.cpp \
    tilemetainfomgr.cpp \
    tilesetmanager.cpp \
    zoomable.cpp \
    zprogress.cpp \
    bmpblender.cpp \
    preferences.cpp \
    addtilesetsdialog.cpp \
    mapimagemanager.cpp \
    resizehelper.cpp \
    textureunpacker.cpp \
    tmxmapwriter.cpp \
    main.cpp \
    commandlineparser.cpp \
    languagemanager.cpp \
    tiledapplication.cpp \
    tiledeffile.cpp \
    BuildingEditor/roofhiding.cpp

HEADERS += colorbutton.h \
    filesystemwatcher.h \
    mapcomposite.h \
    mapmanager.h \
    resizedialog.h \
    threads.h \
    tilemetainfodialog.h \
    tilemetainfomgr.h \
    tilesetmanager.h \
    zoomable.h \
    zprogress.h \
    bmpblender.h \
    preferences.h \
    addtilesetsdialog.h \
    mapimagemanager.h \
    resizehelper.h \
    textureunpacker.h \
    tmxmapwriter.h \
    commandlineparser.h \
    languagemanager.h \
    tiledapplication.h \
    tiledeffile.h \
    BuildingEditor/roofhiding.h

HEADERS += BuildingEditor/buildingeditorwindow.h \
    BuildingEditor/simplefile.h \
    BuildingEditor/buildingtools.h \
    BuildingEditor/buildingdocument.h \
    BuildingEditor/building.h \
    BuildingEditor/buildingfloor.h \
    BuildingEditor/buildingundoredo.h \
    BuildingEditor/mixedtilesetview.h \
    BuildingEditor/newbuildingdialog.h \
    BuildingEditor/buildingpreferencesdialog.h \
    BuildingEditor/buildingobjects.h \
    BuildingEditor/buildingtemplates.h \
    BuildingEditor/buildingtemplatesdialog.h \
    BuildingEditor/choosebuildingtiledialog.h \
    BuildingEditor/roomsdialog.h \
    BuildingEditor/buildingtilesdialog.h \
    BuildingEditor/buildingtiles.h \
    BuildingEditor/templatefrombuildingdialog.h \
    BuildingEditor/buildingwriter.h \
    BuildingEditor/buildingreader.h \
    BuildingEditor/resizebuildingdialog.h \
    BuildingEditor/furnitureview.h \
    BuildingEditor/furnituregroups.h \
    BuildingEditor/buildingpreferences.h \
    BuildingEditor/buildingtmx.h \
    BuildingEditor/tilecategoryview.h \
    BuildingEditor/listofstringsdialog.h \
    BuildingEditor/horizontallinedelegate.h \
    BuildingEditor/buildingfloorsdialog.h \
    BuildingEditor/buildingtiletools.h \
    BuildingEditor/buildingmap.h \
    BuildingEditor/buildingfurnituredock.h \
    BuildingEditor/buildingtilesetdock.h \
    BuildingEditor/buildinglayersdock.h \
    BuildingEditor/buildingorthoview.h \
    BuildingEditor/buildingisoview.h \
    BuildingEditor/choosetemplatesdialog.h \
    BuildingEditor/buildingtileentryview.h \
    BuildingEditor/buildingpropertiesdialog.h \
    BuildingEditor/buildingdocumentmgr.h \
    BuildingEditor/categorydock.h \
    BuildingEditor/imode.h \
    BuildingEditor/objecteditmode.h \
    BuildingEditor/tileeditmode.h \
    BuildingEditor/editmodestatusbar.h \
    BuildingEditor/objecteditmode_p.h \
    BuildingEditor/tileeditmode_p.h \
    BuildingEditor/embeddedmainwindow.h \
    BuildingEditor/singleton.h \
    BuildingEditor/fancytabwidget.h \
    BuildingEditor/utils/stylehelper.h \
    BuildingEditor/utils/styledbar.h \
    BuildingEditor/utils/hostosinfo.h \
    BuildingEditor/welcomemode.h \
    BuildingEditor/buildingroomdef.h

SOURCES += BuildingEditor/simplefile.cpp \
    BuildingEditor/buildingtools.cpp \
    BuildingEditor/buildingdocument.cpp \
    BuildingEditor/building.cpp \
    BuildingEditor/buildingfloor.cpp \
    BuildingEditor/buildingundoredo.cpp \
    BuildingEditor/mixedtilesetview.cpp \
    BuildingEditor/newbuildingdialog.cpp \
    BuildingEditor/buildingpreferencesdialog.cpp \
    BuildingEditor/buildingobjects.cpp \
    BuildingEditor/buildingtemplates.cpp \
    BuildingEditor/buildingtemplatesdialog.cpp \
    BuildingEditor/choosebuildingtiledialog.cpp \
    BuildingEditor/roomsdialog.cpp \
    BuildingEditor/buildingtilesdialog.cpp \
    BuildingEditor/buildingtiles.cpp \
    BuildingEditor/templatefrombuildingdialog.cpp \
    BuildingEditor/buildingwriter.cpp \
    BuildingEditor/buildingreader.cpp \
    BuildingEditor/resizebuildingdialog.cpp \
    BuildingEditor/furnitureview.cpp \
    BuildingEditor/furnituregroups.cpp \
    BuildingEditor/buildingpreferences.cpp \
    BuildingEditor/buildingtmx.cpp \
    BuildingEditor/tilecategoryview.cpp \
    BuildingEditor/listofstringsdialog.cpp \
    BuildingEditor/horizontallinedelegate.cpp \
    BuildingEditor/buildingfloorsdialog.cpp \
    BuildingEditor/buildingtiletools.cpp \
    BuildingEditor/buildingmap.cpp \
    BuildingEditor/buildingfurnituredock.cpp \
    BuildingEditor/buildingtilesetdock.cpp \
    BuildingEditor/buildinglayersdock.cpp \
    BuildingEditor/buildingeditorwindow.cpp \
    BuildingEditor/buildingorthoview.cpp \
    BuildingEditor/buildingisoview.cpp \
    BuildingEditor/choosetemplatesdialog.cpp \
    BuildingEditor/buildingtileentryview.cpp \
    BuildingEditor/buildingpropertiesdialog.cpp \
    BuildingEditor/buildingdocumentmgr.cpp \
    BuildingEditor/categorydock.cpp \
    BuildingEditor/imode.cpp \
    BuildingEditor/objecteditmode.cpp \
    BuildingEditor/tileeditmode.cpp \
    BuildingEditor/editmodestatusbar.cpp \
    BuildingEditor/embeddedmainwindow.cpp \
    BuildingEditor/fancytabwidget.cpp \
    BuildingEditor/utils/stylehelper.cpp \
    BuildingEditor/utils/styledbar.cpp \
    BuildingEditor/welcomemode.cpp \
    BuildingEditor/buildingroomdef.cpp

FORMS += BuildingEditor/buildingeditorwindow.ui \
    BuildingEditor/newbuildingdialog.ui \
    BuildingEditor/buildingpreferencesdialog.ui \
    BuildingEditor/buildingtemplatesdialog.ui \
    BuildingEditor/choosebuildingtiledialog.ui \
    BuildingEditor/roomsdialog.ui \
    BuildingEditor/buildingtilesdialog.ui \
    BuildingEditor/templatefrombuildingdialog.ui \
    BuildingEditor/resizebuildingdialog.ui \
    BuildingEditor/listofstringsdialog.ui \
    BuildingEditor/buildingfloorsdialog.ui \
    BuildingEditor/buildingtilesetdock.ui \
    BuildingEditor/buildinglayersdock.ui \
    BuildingEditor/choosetemplatesdialog.ui \
    BuildingEditor/buildingpropertiesdialog.ui \
    BuildingEditor/welcomemode.ui \
    addtilesetsdialog.ui

macx {
    OBJECTIVE_SOURCES += macsupport.mm
}

FORMS += resizedialog.ui \
    tilemetainfodialog.ui

RESOURCES += tiled.qrc BuildingEditor/buildingeditor.qrc
macx {
    TARGET = BuildingEd
    QMAKE_INFO_PLIST = Info.plist
    ICON = images/tiled-icon-mac.icns
}
win32 {
    RC_FILE = BuildingEd.rc
}
win32:INCLUDEPATH += .
contains(CONFIG, static) {
    DEFINES += STATIC_BUILD
    QTPLUGIN += qgif \
        qjpeg \
        qtiff
}

# In the "qmake" build step, add "INSTALL_ONLY_BUILD=1" under "Additional arguments"
# Add a new "Make" build step, with "install" under "Make arguments"

isEmpty(INSTALL_ONLY_BUILD) {
    win32:CONFIG_PREFIX = $${target.path}
    unix:CONFIG_PREFIX = $${target.path}/../share/tilezed/config
    macx:CONFIG_PREFIX = $${target.path}/TileZed.app/Contents/Config

    win32:DOCS_PREFIX = $${target.path}/docs
    unix:DOCS_PREFIX = $${target.path}/../share/tilezed/docs
    macx:DOCS_PREFIX = $${target.path}/TileZed.app/Contents/Docs

    win32:LUA_PREFIX = $${target.path}/lua
    unix:LUA_PREFIX = $${target.path}/../share/tilezed/lua
    macx:LUA_PREFIX = $${target.path}/TileZed.app/Contents/Lua
} else {
    win32:CONFIG_PREFIX = $${top_builddir}
    unix:CONFIG_PREFIX = $${top_builddir}/share/tilezed/config
    macx:CONFIG_PREFIX = $${top_builddir}/bin/TileZed.app/Contents/Config

    win32:DOCS_PREFIX = $${top_builddir}/docs
    unix:DOCS_PREFIX = $${top_builddir}/share/tilezed/docs
    macx:DOCS_PREFIX = $${top_builddir}/TileZed.app/Contents/Docs

    win32:LUA_PREFIX = $${top_builddir}/lua
    unix:LUA_PREFIX = $${top_builddir}/share/tilezed/lua
    macx:LUA_PREFIX = $${top_builddir}/TileZed.app/Contents/Lua
}

configTxtFiles.path = $${CONFIG_PREFIX}
configTxtFiles.files = \
    $${top_srcdir}/LuaTools.txt \
    $${top_srcdir}/TileProperties.txt \
    $${top_srcdir}/Tilesets.txt
INSTALLS += configTxtFiles

buildingEdTxt.path = $${CONFIG_PREFIX}
buildingEdTxt.files = \
    BuildingEditor/BuildingFurniture.txt \
    BuildingEditor/BuildingTemplates.txt \
    BuildingEditor/BuildingTiles.txt \
    BuildingEditor/TMXConfig.txt
INSTALLS += buildingEdTxt

tiledDocs.path = $${DOCS_PREFIX}
tiledDocs.files = \
    $${top_srcdir}/docs/TileProperties \
    $${top_srcdir}/docs/TileZed
INSTALLS += tiledDocs

buildingEdDocs.path = $${DOCS_PREFIX}/BuildingEd
buildingEdDocs.files = \
    BuildingEditor/manual/*
INSTALLS += buildingEdDocs

luaScripts.path = $${LUA_PREFIX}
luaScripts.files = $${top_srcdir}/lua/*
INSTALLS += luaScripts
