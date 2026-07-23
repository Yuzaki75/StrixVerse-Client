@echo off
call C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC\Auxiliary\Build\vcvarsall.bat x64
C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe .. -G NMake Makefiles
