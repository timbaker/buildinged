call "D:\VisualStudioCommunity2013\VC\vcvarsall.bat" x86
mkdir C:\Programming\BuildingEd\dist32
cd C:\Programming\BuildingEd\dist32
"C:\Programming\QtSDK2015\5.7\msvc2013\bin\qmake.exe" C:\Programming\BuildingEd\BuildingEd\tiled.pro -r -spec win32-msvc2013 "CONFIG+=release"
"C:\Programming\QtSDK2015\Tools\QtCreator\bin\jom.exe"
"C:\Programming\TclTk\8.5.x\32bit-mingw\bin\tclsh85.exe" C:\Programming\BuildingEd\BuildingEd\dist.tcl 32bit
PAUSE
