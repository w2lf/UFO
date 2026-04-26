@echo off
cd /d "c:\Users\w2lf\Documents\Unreal Projects\UFO"
echo Building UFO C++ project...
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" UFO.sln /p:Configuration=Development /p:Platform=Win64 /verbosity:normal /m:1
echo Build process completed.
pause
