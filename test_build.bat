@echo off
echo Before vcvarsall
call "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvarsall.bat" x64
echo After vcvarsall
"C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" /noautoresponse build/debug\StrixVerseClient.vcxproj /p:Configuration=Debug /p:Platform=x64
echo After MSBuild
