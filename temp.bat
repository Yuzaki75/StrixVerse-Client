@echo off
call C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvarsall.bat x64
C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe /noautoresponse build/debugStrixVerseClient.vcxproj /p:Configuration=Debug /p:Platform=x64
