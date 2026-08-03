#!/usr/bin/env bash

# Build BeanPort firmware.
# Usage: ./scripts/build.sh [board]
# board defaults to pico (other options: pico_w, pico2, pico2_w)
#
# Each board gets its own build directory (build/<board>/)
#
# Requires: cmake, arm-none-eabi-gcc (brew install --cask gcc-arm-embedded),
# picotool, and the pico-sdk submodule initialised (see init-submodules.sh)

set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOARD=${1:-pico}
BUILD_DIR="$REPO_ROOT/build/$BOARD"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake -DPICO_BOARD=$BOARD "$REPO_ROOT"
cmake --build .
