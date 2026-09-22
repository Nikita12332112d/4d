@echo off
setlocal enabledelayedexpansion

set SRC=src\main.cpp src\glcore.cpp src\gfx.cpp src\text.cpp src\scene.cpp
if not exist build mkdir build

where cl >nul 2>nul
if %errorlevel%==0 goto msvc

set "PF86=%ProgramFiles(x86)%"
set "PF64=%ProgramFiles%"

for %%y in (2022 2019 2017) do (
    for %%v in (BuildTools Community Professional Enterprise) do (
        set "VC=!PF86!\Microsoft Visual Studio\%%y\%%v\VC\Auxiliary\Build\vcvars64.bat"
        if exist "!VC!" goto found
        set "VC=!PF64!\Microsoft Visual Studio\%%y\%%v\VC\Auxiliary\Build\vcvars64.bat"
        if exist "!VC!" goto found
    )
)

where g++ >nul 2>nul
if %errorlevel%==0 goto gcc

if exist C:\msys64\mingw64\bin\g++.exe (
    set "PATH=C:\msys64\mingw64\bin;%PATH%"
    goto gcc
)

echo Compiler not found. Install Visual Studio Build Tools or MinGW.
exit /b 1

:found
call "!VC!" >nul 2>nul

:msvc
cl /nologo /EHsc /O2 /W4 /std:c++17 /Fo:build\ /Fe:spacetime4d-gl.exe %SRC% /link /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup opengl32.lib gdi32.lib user32.lib
if errorlevel 1 exit /b 1
echo Built spacetime4d-gl.exe
exit /b 0

:gcc
g++ -O2 -std=c++17 -Wall -Wextra -mwindows -o spacetime4d-gl.exe %SRC% -lopengl32 -lgdi32 -luser32
if errorlevel 1 exit /b 1
echo Built spacetime4d-gl.exe
exit /b 0
