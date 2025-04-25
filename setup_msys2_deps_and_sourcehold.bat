@echo off
setlocal enabledelayedexpansion

REM ===========================================================
REM == Helper Script to Install MSYS2 & Sourcehold Deps      ==
REM ==                                                       ==
REM == Assumes MSYS2 is installed at C:\msys64               ==
REM == Assumes this script is run from the project root dir  ==
REM == Installs C/GTK deps via pacman                        ==
REM == Clones & builds 'sourcehold-maps' from source via pip ==
REM ===========================================================

REM == Configuration ==
set MSYS2_PATH=C:\msys64
set "PROJECT_DIR=%~dp0"
REM Get the actual directory name of the project
for %%* in ("%PROJECT_DIR%.") do set PROJECT_NAME=%%~nx*
REM Convert project dir path to MSYS2 format for use inside the shell
set "MSYS2_PROJECT_DIR=%PROJECT_DIR:\=/%"
set "MSYS2_PROJECT_DIR=%MSYS2_PROJECT_DIR:C:=/c%"
REM Remove trailing slash if present for cd later
if "%MSYS2_PROJECT_DIR:~-1%"=="/" set "MSYS2_PROJECT_DIR=%MSYS2_PROJECT_DIR:~0,-1%"

REM Check if MSYS2 exists
if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    echo ERROR: MSYS2 not found at %MSYS2_PATH%.
    echo Please install MSYS2 from https://www.msys2.org/ and ensure it's in the default location.
    pause
    goto :eof
)

echo Starting MSYS2 MINGW64 shell to install dependencies...
echo.
echo A new MSYS2 window will open and attempt to:
echo 1. Update package databases (pacman -Syu)
echo 2. Install required C/GTK/Meson packages via pacman (inc. python-pip)
echo 3. Clone the 'sourcehold-maps' repository into the parent directory
echo 4. Install 'sourcehold-maps' using pip from source (using MSYS2's Python)
echo.
echo Please monitor the MSYS2 window for any confirmation prompts or errors.
echo Using '--noconfirm' for pacman to minimize prompts.
echo.
pause

REM Construct the command string to run inside MSYS2
set COMMAND_STRING="echo '--- Updating MSYS2 packages (pacman -Syu)... ---' && \
pacman -Syu --noconfirm --needed && \
echo '--- Installing Build & Python Dependencies... ---' && \
pacman -S --noconfirm --needed git mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-python mingw-w64-x86_64-pkgconf mingw-w64-x86_64-gtk4 mingw-w64-x86_64-libadwaita mingw-w64-x86_64-json-glib mingw-w64-x86_64-gettext mingw-w64-x86_64-desktop-file-utils mingw-w64-x86_64-appstream mingw-w64-x86_64-python-pip && \
echo '--- Cloning sourcehold-maps repository (into parent directory)... ---' && \
cd .. && \
if [ -d sourcehold-maps ]; then echo 'sourcehold-maps directory already exists, skipping clone.'; else git clone https://github.com/sourcehold/sourcehold-maps.git || exit 1; fi && \
echo '--- Installing sourcehold-maps via pip (using MSYS2 Python)... ---' && \
cd sourcehold-maps && \
/mingw64/bin/pip3 install . || echo 'WARNING: pip install for sourcehold-maps failed. Module might not work.' && \
echo '--- Returning to project directory (%PROJECT_NAME%)... ---' && \
cd '%MSYS2_PROJECT_DIR%' && \
echo. && \
echo '--- Dependency installation commands finished. Check for errors above. ---' && \
echo '--- You can now build the project in this shell: ---' && \
echo '   meson setup build --prefix=/mingw64 --buildtype=release' && \
echo '   ninja -C build install' && \
echo '--- IMPORTANT: Remember to configure the Python path in the application preferences! ---' && \
echo '--- Since this script installed sourcehold-maps to the MSYS2 Python, use this path: ---' && \
echo '---   C:\msys64\mingw64\bin\python.exe ---' && \
echo '--- (Or the equivalent MSYS2 path: /mingw64/bin/python.exe) ---' && \
echo '--- Keeping shell open. ---' && \
echo. && \
exec bash -i"

REM Set environment variable for MSYS2 to start in the project directory
set CHERE_INVOKING=true

REM Launch msys2_shell.cmd to execute the command string
REM Using start to open a new window
start "MSYS2 Dependency Installation" "%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -here -c %COMMAND_STRING%

echo.
echo MSYS2 shell launched. Please follow prompts or check for errors in that window.
echo After it finishes, use that MSYS2 MINGW64 shell to build the project.
echo.
echo == IMPORTANT ==
echo Remember to configure the Python path in the GTK Crusader Village application preferences
echo to point to the MSYS2 Python installation used by this script:
echo   C:\msys64\mingw64\bin\python.exe
echo =============

endlocal