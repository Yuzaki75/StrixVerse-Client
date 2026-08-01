@echo off
echo Before calling vcvarsall
call C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvarsall.bat x64
echo After calling vcvarsall
C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe /noautoresponse build/debugStrixVerseClient.vcxproj /p:Configuration=Debug /p:Platform=x64
echo After MSBuild
