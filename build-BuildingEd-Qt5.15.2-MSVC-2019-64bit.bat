call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
mkdir C:\Programming\BuildingEd\dist64
cd C:\Programming\BuildingEd\dist64
"C:\Programming\QtSDK2015\5.15.2\msvc2019_64\bin\qmake.exe" C:\Programming\BuildingEd\BuildingEd\tiled.pro -r -spec win32-msvc "CONFIG+=release"
"C:\Programming\QtSDK2015\Tools\QtCreator\bin\jom\jom.exe"
"C:\Programming\TclTk\8.5.x\32bit-mingw\bin\tclsh85.exe" C:\Programming\BuildingEd\BuildingEd\dist.tcl 64bit
PAUSE
