#!/bin/sh

set -e
set -x

HOST_SYSTEM="$1"
PYTHON_EXE="$2"
BUILD_ROOT="$3"
SOURCE_ROOT="$4"
PRIVATE_DIR="$5"
OUTPUT="$6"

echo '-------- START BUILDING SOURCEHOLD --------'

rm -rf "$OUTPUT"

if [ "$HOST_SYSTEM" = 'windows' ]; then

    PYTHON_INSTALL_VERSION='3.13.3'
    PYTHON_INSTALL_BUILD_DATE='20250409'
    PYTHON_INSTALL_TAR='cpython-'"$PYTHON_INSTALL_VERSION"'+'"$PYTHON_INSTALL_BUILD_DATE"'-x86_64-pc-windows-msvc-install_only.tar.gz'
    PYTHON_INSTALL_URL='https://github.com/astral-sh/python-build-standalone/releases/download/'"$PYTHON_INSTALL_BUILD_DATE"'/'"$PYTHON_INSTALL_TAR"
    wget -O "$PRIVATE_DIR"/"$PYTHON_INSTALL_TAR" "$PYTHON_INSTALL_URL"
    tar -C "$PRIVATE_DIR" -xzvf "$PRIVATE_DIR"/"$PYTHON_INSTALL_TAR"
    mv "$PRIVATE_DIR"/python "$OUTPUT"

    "$OUTPUT"/python.exe -m pip install --upgrade pip
    "$OUTPUT"/python.exe -m pip install build distlib setuptools
    "$OUTPUT"/python.exe -m build "$SOURCE_ROOT"/sourcehold-maps -w -o "$PRIVATE_DIR"
    "$OUTPUT"/python.exe -m pip install --upgrade "$PRIVATE_DIR"/*.whl
    
else

    mkdir -p "$OUTPUT"
    
    "$PYTHON_EXE" -m venv "$PRIVATE_DIR"/venv
    "$PRIVATE_DIR"/venv/bin/python -m pip install --upgrade pip
    "$PRIVATE_DIR"/venv/bin/python -m pip install build distlib setuptools
    "$PRIVATE_DIR"/venv/bin/python -m build "$SOURCE_ROOT"/sourcehold-maps -w -o "$PRIVATE_DIR"
    "$PRIVATE_DIR"/venv/bin/python -m pip --python "$PYTHON_EXE" install --upgrade --target "$OUTPUT" "$PRIVATE_DIR"/*.whl

fi

echo '-------- END BUILDING SOURCEHOLD --------'
