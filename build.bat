@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

IF NOT EXIST build mkdir build
pushd build

REM Zi -- full debug info
REM I -- include folder
REM SUBSYSTEM:CONSOLE -- sends output to the console window (default is SUBSYSTEM:WINDOWS)

set CompilerFlags= -Zi -I ../include/
set LinkerFlags= /SUBSYSTEM:CONSOLE /LIBPATH:../lib/x64/ SDL2.lib SDL2_image.lib SDL2_ttf.lib

cl %CompilerFlags% ../main.cpp /link %LinkerFlags%

popd