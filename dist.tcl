if {[llength [info commands console]]} {
    console show
    update
}

set QT_DIR C:/Programming/QtSDK2015/5.7/msvc2013_64
set BIN C:/Programming/BuildingEd/dist64
set SRC C:/Programming/BuildingEd/BuildingEd
set DEST {C:\Users\Tim\Desktop\ProjectZomboid\Tools\BuildingEd}
set SUFFIX "-64bit"
set SUFFIX2 ""
set REDIST vcredist_x64.exe

if {$argc > 0} {
    switch -- [lindex $argv 0] {
        32bit {
            puts "dist.tcl: 32-bit"
            append DEST "32"
            set BIN C:/Programming/BuildingEd/dist32
            set QT_DIR C:/Programming/QtSDK2015/5.7/msvc2013
            set SUFFIX "-32bit"
            set SUFFIX2 "32"
            set REDIST vcredist_x86.exe
        }
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
copyFile {C:\Programming\TileZed} $DEST $REDIST
copyFile $BIN $DEST BuildingEd.exe
copyFile $BIN $DEST tiled.dll
copyFile $BIN $DEST zlib1.dll

copyFile $SRC $DEST LICENSE.APACHE LICENSE.APACHE.txt
copyFile $SRC $DEST LICENSE.BSD LICENSE.BSD.txt
copyFile $SRC $DEST LICENSE.GPL LICENSE.GPL.txt
copyFile $SRC $DEST LICENSE.QT5 LICENSE.QT5.txt

createFile $DEST qt.conf {[Paths]
Plugins = plugins
Translations = translations
}

copyFile $SRC $DEST Tilesets.txt

puts ---Translations---
set qt_trs {qt_cs.qm qt_de.qm qt_es.qm qt_fr.qm qt_he.qm qt_ja.qm qt_pt.qm qt_ru.qm qt_zh_CN.qm qt_zh_TW.qm}
foreach name $qt_trs {
    copyFile $QT_TRANSLATIONS_DIR $DEST/translations $name
}

puts ---BuildingEd---
foreach name {BuildingFurniture.txt BuildingTemplates.txt BuildingTiles.txt TMXConfig.txt} {
    copyFile $SRC/src/tiled/BuildingEditor $DEST $name
}
copyDir $SRC/src/tiled/BuildingEditor $DEST/docs manual BuildingEd

puts "---Qt DLLS---"
copyFile $QT_BINARY_DIR $DEST Qt5Core.dll
copyFile $QT_BINARY_DIR $DEST Qt5Gui.dll
copyFile $QT_BINARY_DIR $DEST Qt5Network.dll
copyFile $QT_BINARY_DIR $DEST Qt5OpenGL.dll
copyFile $QT_BINARY_DIR $DEST Qt5Widgets.dll
copyFile $QT_BINARY_DIR $DEST Qt5Xml.dll
if {[file exists $QT_BINARY_DIR/icudt54.dll]} {
copyFile $QT_BINARY_DIR $DEST icudt54.dll
copyFile $QT_BINARY_DIR $DEST icuin54.dll
copyFile $QT_BINARY_DIR $DEST icuuc54.dll
}

copyFile $QT_PLUGINS_DIR $DEST/plugins imageformats/qgif.dll
copyFile $QT_PLUGINS_DIR $DEST/plugins imageformats/qjpeg.dll
copyFile $QT_PLUGINS_DIR $DEST/plugins imageformats/qtiff.dll

copyFile $QT_PLUGINS_DIR $DEST/plugins platforms/qwindows.dll

