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

    EMBEDDED_PYTHON_VER='3.13.3'
    EMBEDDED_PYTHON_ZIP='python-'"$EMBEDDED_PYTHON_VER"'-embed-amd64.zip'
    EMBEDDED_PYTHON_URL='https://www.python.org/ftp/python/'"$EMBEDDED_PYTHON_VER"'/'"$EMBEDDED_PYTHON_ZIP"
    wget -O "$PRIVATE_DIR"/"$EMBEDDED_PYTHON_ZIP" "$EMBEDDED_PYTHON_URL"
    unzip "$PRIVATE_DIR"/"$EMBEDDED_PYTHON_ZIP" -d "$OUTPUT"

    mv "$OUTPUT"/python313._pth "$OUTPUT"/python313.pth
    mkdir -p "$OUTPUT"/DLLS

    GETPIP_URL='https://bootstrap.pypa.io/get-pip.py'
    wget -O "$PRIVATE_DIR"/get-pip.py "$GETPIP_URL"
    "$OUTPUT"/python.exe "$PRIVATE_DIR"/get-pip.py
    
    "$OUTPUT"/python.exe -m pip install --upgrade pip
    "$OUTPUT"/python.exe -m pip install build distlib setuptools pybind11
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
