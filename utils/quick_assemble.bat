@echo off
REM Build and run gate_computer_toolset

REM Delete old executable if it exists
if exist gct.exe (
    taskkill /F /IM gct.exe 2>nul
    timeout /t 1 /nobreak >nul
    del /F gct.exe
)

REM Compile
echo Compiling gate_computer_toolset.cpp...
g++ -std=c++17 -Wall -o gct gate_computer_toolset.cpp

REM Check if compilation succeeded
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo Compilation successful!
echo.

REM Run with automated input
(
echo 1
echo script.s
echo.
echo.
echo 0
) | gct.exe
