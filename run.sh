#!/usr/bin/env bash
# build-run.sh ── compile main.cpp → exe/main.exe, then run it with an argument.
set -euo pipefail

# ── configuration (edit if your file/folders differ) ────────────────────────
SRC="stage3.cpp"                    # your C++ source
EXE_DIR="exe"                     # where the .exe should live
EXE_NAME="$(basename "$SRC" .cpp).exe"
CXXFLAGS="-std=c++17 -O2"         # add -Wall -Wextra if you like
CXX="g++"                         # full path if g++ isn't on PATH
# ────────────────────────────────────────────────────────────────────────────

INPUT_FILE="${1:-scene.txt}"      # default argument if none given

mkdir -p "$EXE_DIR"

echo
echo "Compiling with:"
echo "  $CXX $CXXFLAGS \"$SRC\" -o \"$EXE_DIR/$EXE_NAME\""
$CXX $CXXFLAGS "$SRC" -o "$EXE_DIR/$EXE_NAME"

echo
echo "Running with:"
echo "  \"$EXE_DIR/$EXE_NAME\" \"$INPUT_FILE\""
"$EXE_DIR/$EXE_NAME" "$INPUT_FILE"
