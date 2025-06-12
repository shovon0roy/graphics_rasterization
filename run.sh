#!/usr/bin/env bash
# build-run.sh ── compile stage{1..4}.cpp   → exe/stage{1..4}.exe
#                 then run the full 4-stage pipeline for every test-case dir.
set -euo pipefail

# ── configuration ───────────────────────────────────────────────────────────
SRC_DIR="."               # where stage?.cpp live
EXE_DIR="exe"             # compiled binaries go here
CXX="g++"                 # change if g++ isn't on PATH
CXXFLAGS="-std=c++17 -O2" # add -Wall -Wextra -pedantic if you wish
STAGES=(stage1 stage2 stage3 stage4)
# ────────────────────────────────────────────────────────────────────────────

mkdir -p "$EXE_DIR"

# ---------------------------------------------------------------------------
# 1. Build all four stages
# ---------------------------------------------------------------------------
for S in "${STAGES[@]}"; do
  SRC="$SRC_DIR/$S.cpp"
  EXE="$EXE_DIR/$S.exe"
  echo
  echo "Compiling $SRC → $EXE"
  "$CXX" $CXXFLAGS "$SRC" -o "$EXE"
done

# ---------------------------------------------------------------------------
# 2. Run pipeline for every test-case directory supplied
#    Usage: ./build-run.sh  path/to/case1  path/to/case2 …
#    If no dir given, run in current dir (must contain scene.txt+config.txt).
# ---------------------------------------------------------------------------
if [ "$#" -eq 0 ]; then
  set -- "."                # default to current directory
fi

echo
for CASE_DIR in "$@"; do
  echo "─── Processing test case: $CASE_DIR ───"
  SCENE_FILE="$CASE_DIR/scene.txt"
  CONFIG_FILE="$CASE_DIR/config.txt"

  # Basic sanity checks
  if [ ! -f "$SCENE_FILE" ] || [ ! -f "$CONFIG_FILE" ]; then
    echo "  ⚠️  Skipping: scene.txt or config.txt missing"
    continue
  fi

  # Create / clean output folder inside the case dir
  OUT_DIR="$CASE_DIR/output"
  rm -rf "$OUT_DIR"
  mkdir  -p "$OUT_DIR"

  # Stage 1: world → stage1.txt  (+camera/proj json if your code writes it)
  echo "  ➤ Stage 1"
  "$EXE_DIR/stage1.exe" "$CASE_DIR" "$SCENE_FILE"        # writes $CASE_DIR/output/stage1.txt

  # Stage 2: view  → stage2.txt
  echo "  ➤ Stage 2"
  "$EXE_DIR/stage2.exe" "$CASE_DIR"                     # reads files inside $CASE_DIR/output

  # Stage 3: proj  → stage3.txt
  echo "  ➤ Stage 3"
  "$EXE_DIR/stage3.exe" "$CASE_DIR"                     # ditto

  # Stage 4: raster → z_buffer.txt + out.bmp (needs config.txt)
  echo "  ➤ Stage 4"
  "$EXE_DIR/stage4.exe" "$CASE_DIR" "$CONFIG_FILE"       # reads stage3 & config, writes bmp/zb

  echo "  ✔  Done — results in $OUT_DIR"
  echo
done
