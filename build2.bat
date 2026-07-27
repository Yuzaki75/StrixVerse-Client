@echo off
echo Before calling vcvarsall.bat > pre.log
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
echo After calling vcvarsall.bat > post.log
msbuild build\debug\StrixVerseClient.vcxproj /p:Configuration=Debug /p:Platform=x64 >> build.log 2>&1
