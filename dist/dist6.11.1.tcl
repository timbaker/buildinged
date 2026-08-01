if {[llength [info commands console]]} {
    console show
    update
}

set QT_DIR C:/Programming/QtSDK2015/6.11.1/msvc2022_64
set BIN C:/Programming/buildinged-dist6.11.1
set SRC C:/Programming/buildinged
set DEST {C:\Programming\ProjectZomboid\Tools\BuildingEd}
set SUFFIX "-64bit"
set SUFFIX2 ""
# C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\v143
set REDIST vc_redist.x64.2015-2022.exe

if {$argc > 0} {
    switch -- [lindex $argv 0] {
        64bit {
            puts "dist.tcl: 64-bit"
        }
        default {
            error "unknown arguments to dist.tcl: $argv"
        }
    }
}

set QT_BINARY_DIR $QT_DIR/bin
set QT_PLUGINS_DIR $QT_DIR/plugins
set QT_TRANSLATIONS_DIR $QT_DIR/translations

proc copyFile {SOURCE DEST name {name2 ""}} {
    if {$name2 == ""} { set name2 $name }
    set src [file join $SOURCE $name]
    set dst [file join $DEST $name2]
    if {![file exists $src]} {
        error "no such file \"$src\""
    }
    set relative $name
    foreach var {BIN SRC QT_BINARY_DIR QT_PLUGINS_DIR QT_TRANSLATIONS_DIR} {
        if {[string match [set ::$var]* $src]} {
            set relative [string range $src [string length [set ::$var]] end]
        }
    }
    if {![file exists $dst] || ([file mtime $src] > [file mtime $dst]) || ([file size $src] != [file size $dst])} {
        file mkdir [file dirname $dst]
        if {[file extension $name2] == ".txt"} {
            set chan [open $src r]
            set text [read $chan]
            close $chan
            set chan [open $dst w]
            fconfigure $chan -translation crlf
            puts -nonewline $chan $text
            close $chan
            puts "copied $relative (crlf)"
        } else {
            file copy -force $src $dst
            puts "copied $relative"
        }
    } else {
        puts "skipped $relative"
    }
    return
}

proc copyDir {SOURCE DEST name {name2 ""}} {
    if {$name2 == ""} { set name2 $name }
    set src [file join $SOURCE $name]
    set dst [file join $DEST $name2]
    if {![file exists $src]} {
        error "no such directory \"$src\""
    }
    foreach f [glob -nocomplain -tails -dir $src *] {
        if {$f == "." || $f == ".."} continue
        if {[file isdirectory $src/$f]} {
            copyDir $src $dst $f
        } else {
            copyFile $src $dst $f
        }
    }
}

proc createFile {DEST name contents} {
    set chan [open $DEST/$name w]
    fconfigure $chan -translation crlf
    puts -nonewline $chan $contents
    close $chan
    puts "created $DEST/$name"
}

puts ---Toplevel---
copyFile {C:\Programming\TileZed} $DEST $REDIST vc_redist.x64.exe
copyFile $BIN $DEST BuildingEd.exe
copyFile $BIN $DEST tiled.dll
copyFile $BIN $DEST zlib1.dll

copyFile $SRC $DEST LICENSE.APACHE LICENSE.APACHE.txt
copyFile $SRC $DEST LICENSE.BSD LICENSE.BSD.txt
copyFile $SRC $DEST LICENSE.GPL LICENSE.GPL.txt
copyFile $SRC $DEST LICENSE.QT5 LICENSE.QT6.txt

createFile $DEST qt.conf {[Paths]
Plugins = plugins
Translations = translations
}

copyFile $SRC $DEST Tilesets.txt

puts ---Translations---
set qt_trs {
    qt_ar.qm
    qt_bg.qm
    qt_ca.qm
    qt_cs.qm
    qt_da.qm
    qt_de.qm
    qt_en.qm
    qt_es.qm
    qt_fa.qm
    qt_fi.qm
    qt_fr.qm
    qt_gd.qm
    qt_gl.qm
    qt_he.qm
}
foreach name $qt_trs {
    copyFile $QT_TRANSLATIONS_DIR $DEST/translations $name
}

puts ---BuildingEd---
foreach name {BuildingFurniture.txt BuildingTemplates.txt BuildingTiles.txt TMXConfig.txt} {
    copyFile $SRC/src/tiled/BuildingEditor $DEST $name
}
copyDir $SRC/src/tiled/BuildingEditor $DEST/docs manual BuildingEd

puts "---Qt DLLS---"
copyFile $QT_BINARY_DIR $DEST Qt6Core.dll
copyFile $QT_BINARY_DIR $DEST Qt6Core5Compat.dll
copyFile $QT_BINARY_DIR $DEST Qt6Gui.dll
copyFile $QT_BINARY_DIR $DEST Qt6Network.dll
copyFile $QT_BINARY_DIR $DEST Qt6OpenGL.dll
copyFile $QT_BINARY_DIR $DEST Qt6OpenGLWidgets.dll
copyFile $QT_BINARY_DIR $DEST Qt6Svg.dll
copyFile $QT_BINARY_DIR $DEST Qt6Widgets.dll
copyFile $QT_BINARY_DIR $DEST Qt6Xml.dll

copyFile $QT_PLUGINS_DIR $DEST/plugins imageformats/qgif.dll
copyFile $QT_PLUGINS_DIR $DEST/plugins imageformats/qjpeg.dll
copyFile $QT_PLUGINS_DIR $DEST/plugins imageformats/qsvg.dll
copyFile $QT_PLUGINS_DIR $DEST/plugins imageformats/qtiff.dll

copyFile $QT_PLUGINS_DIR $DEST/plugins platforms/qwindows.dll

copyFile $QT_PLUGINS_DIR $DEST/plugins styles/qmodernwindowsstyle.dll

