::@echo off
:: > Setup required Environment
:: -------------------------------------
set COMPILER_DIR=C:\raylib\w64devkit\bin
set PATH=%PATH%;%COMPILER_DIR%
cd %~dp0
:: .
:: > Compile simple .rc file
:: ----------------------------
cd ..\..\src
cmd /c windres project_name.rc -o project_name.rc.data
:: .
:: > Generating project
:: --------------------------
cmd /c mingw32-make -f Makefile ^
PROJECT_NAME=project_name ^
PROJECT_VERSION=1.0 ^
PROJECT_DESCRIPTION="ProjectDescription" ^
PROJECT_INTERNAL_NAME=project_name ^
PROJECT_PLATFORM=PLATFORM_DESKTOP ^
PROJECT_SOURCE_FILES="project_name.c" ^
BUILD_MODE="RELEASE"
:: > Return to scripts directory
:: -----------------------------
cd ..\projects\scripts