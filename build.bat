@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

IF NOT EXIST build mkdir build
pushd build

REM Zi -- full debug info
REM W4 -- warning level 4
REM WX -- Treat warnings as errors
REM EHa- -- Turn off c++ exception handling
REM wd4100 -- wd is "ignore warning", 4100 is "unreferenced format parameter"
REM -W4 -EHa- -wd4100
set CompilerFlags= -Zi -I ../include/ 
REM set CompilerFlags= -MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -I ../include/
set LinkerFlags= /LIBPATH:../lib/x64/ SDL2.lib SDL2_image.lib SDL2_ttf.lib

cl %CompilerFlags% ../main.cpp /link %LinkerFlags%

popd