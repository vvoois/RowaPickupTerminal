@echo off
REM Build script for portable RowaPickupSlim C++ application
REM This creates a fully portable executable with no external runtime dependencies

setlocal enabledelayedexpansion

echo ========================================
echo RowaPickupSlim Portable Build
echo ========================================
echo.

REM Get Visual Studio installation path
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
  set "VSINSTALLDIR=%%i"
)

if not defined VSINSTALLDIR (
  echo ERROR: Visual Studio not found!
  exit /b 1
)

echo Using Visual Studio: !VSINSTALLDIR!
echo.

REM Build Release x64 - Portable (Static Runtime)
echo Building Release x64 Configuration...
call "!VSINSTALLDIR!\VC\Auxiliary\Build\vcvars64.bat"

cd /d "I:\VSprojects\VCPP\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim"

msbuild RowaPickupSlim.vcxproj ^
  /p:Configuration=Release ^
  /p:Platform=x64 ^
  /p:UseEnv=true ^
  /m

if errorlevel 1 (
  echo ERROR: Build failed!
  exit /b 1
)

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Output: bin\Release\x64\RowaPickupSlim.exe
echo.
echo Portable Application Checklist:
echo [?] C++ Runtime: Static (/MT)
echo [?] PugiXML: Static compiled
echo [?] No .NET dependencies
echo [?] No Visual C++ Redistributable needed
echo.
echo Ready for distribution!
pause
