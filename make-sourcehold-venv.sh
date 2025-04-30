#!/bin/sh

set -e

PYTHON_EXE="$1"
BUILD_ROOT="$2"
SOURCE_ROOT="$3"
PRIVATE_DIR="$4"
OUTPUT="$5"

echo '-------- START BUILDING SOURCEHOLD --------'

rm -rf "$OUTPUT"
mkdir -p "$OUTPUT"

"$PYTHON_EXE" -m venv "$PRIVATE_DIR"/venv
"$PRIVATE_DIR"/venv/bin/python3 -m pip -v install --upgrade pip
"$PRIVATE_DIR"/venv/bin/python3 -m pip -v install build
"$PRIVATE_DIR"/venv/bin/python3 -m build "$SOURCE_ROOT"/sourcehold-maps -v -w -o "$PRIVATE_DIR"

CFLAGS="-I/mingw64/include" "$PRIVATE_DIR"/venv/bin/python3 -m pip install --upgrade --target "$OUTPUT" Pillow --no-binary :all: -C parallel="$(nproc)" -C platform-guessing=disable
"$PRIVATE_DIR"/venv/bin/python3 -m pip install --upgrade --target "$OUTPUT" "$PRIVATE_DIR"/*.whl

echo '-------- END BUILDING SOURCEHOLD --------'
