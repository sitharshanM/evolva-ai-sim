@echo off
echo ============================================================
echo   Digital Life Simulator Build Script
echo ============================================================
echo.

rem Add w64devkit to PATH if installed locally
if exist "C:\w64devkit\w64devkit\bin" (
    set "PATH=C:\w64devkit\w64devkit\bin;%PATH%"
)

set CMAKE_EXE=cmake

where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found CMake in PATH
) else (
    echo [ERROR] CMake not found!
    pause
    exit /b 1
)

if not exist build mkdir build

echo.
echo [1/3] Configuring...
echo.

where ninja >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found Ninja compiler environment, using Ninja generator...
    %CMAKE_EXE% -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
) else (
    %CMAKE_EXE% -S . -B build -G "Visual Studio 17 2022" -A x64
    if %ERRORLEVEL% NEQ 0 (
        echo Trying VS 2019...
        %CMAKE_EXE% -S . -B build -G "Visual Studio 16 2019" -A x64
    )
)

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [2/3] Building Release...
%CMAKE_EXE% --build build --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Build successful!
echo Executable: build\bin\DigitalLife.exe
echo.
if exist build\bin\DigitalLife.exe (
    build\bin\DigitalLife.exe
) else if exist build\bin\Release\DigitalLife.exe (
    build\bin\Release\DigitalLife.exe
)

