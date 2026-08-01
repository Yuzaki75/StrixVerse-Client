@echo off
call "C:/PROGRA~1/MICROS~1/18/Community/VC/Auxiliary/Build/vcvarsall.bat" x64
msbuild build/debug\StrixVerseClient.vcxproj /p:Configuration=Debug /p:Platform=x64
