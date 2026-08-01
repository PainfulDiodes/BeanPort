#!/usr/bin/env bash

# Remove BeanPort build artefacts.
# Usage: ./clean.sh [board]
# Removes build/<board> if given, otherwise the whole build/ directory.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [ -n "$1" ]; then
    rm -rf "$SCRIPT_DIR/build/$1"
else
    rm -rf "$SCRIPT_DIR/build"
fi
