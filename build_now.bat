@echo off
setlocal enabledelayedexpansion
cd /d "c:\Users\w2lf\Documents\Unreal Projects\UFO"
echo.
echo ============================================
echo UFO PROJECT BUILD - WITH FIX VERIFICATION
echo ============================================
echo.
echo Starting build with MSBuild...
echo.

set MSBUILD_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

if not exist %MSBUILD_PATH% (
    echo ERROR: MSBuild not found at %MSBUILD_PATH%
    echo Please ensure Visual Studio 2022 Community is installed with C++ workload
    pause
    exit /b 1
)

echo Using: %MSBUILD_PATH%
echo Solution: UFO.sln
echo Configuration: Development
echo Platform: Win64
echo.

%MSBUILD_PATH% UFO.sln /p:Configuration=Development /p:Platform=Win64 /p:VCProjectVersion=17.0 /verbosity:minimal /m:1

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo BUILD SUCCEEDED
    echo ============================================
    echo.
    echo Next steps:
    echo 1. Reopen Unreal Editor
    echo 2. Open your level
    echo 3. Set GameMode Override to AUFOGameMode
    echo 4. Press Play and check Output Log
    echo 5. Look for: "UFOPawn::BeginPlay called"
    echo.
    echo If you see the UFO pawn in the viewport, the fix is COMPLETE.
    echo.
) else (
    echo.
    echo ============================================
    echo BUILD FAILED
    echo ============================================
    echo.
    echo Error code: %ERRORLEVEL%
    echo Check error messages above for details
    echo.
)

pause
exit /b %ERRORLEVEL%
