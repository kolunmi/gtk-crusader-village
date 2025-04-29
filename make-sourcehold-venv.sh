#!/bin/sh

set -e

PYTHON_EXE="$1"
BUILD_ROOT="$2"
SOURCE_ROOT="$3"
PRIVATE_DIR="$4"
OUTPUT="$5"

echo '-------- START BUILDING SOURCEHOLD --------'

# SOURCEHOLD_VERSION="$("$PYTHON" "$MESON_SOURCE_ROOT"/sourcehold-maps/setup.py -V 2>/dev/null)"
"$PYTHON_EXE" -m venv "$OUTPUT"
"$OUTPUT"/bin/python -m pip -v install --upgrade pip
"$OUTPUT"/bin/python -m pip -v install build
"$OUTPUT"/bin/python -m build "$SOURCE_ROOT"/sourcehold-maps -v -w -o "$PRIVATE_DIR"
"$OUTPUT"/bin/python -m pip -v install "$PRIVATE_DIR"/*.whl

echo '-------- END BUILDING SOURCEHOLD --------'
