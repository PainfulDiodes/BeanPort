#!/usr/bin/env bash

# Remove BeanPort build artefacts.
# Usage: ./scripts/clean.sh [board]
# Removes build/<board> if given, otherwise the whole build/ directory.

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [ -n "$1" ]; then
    rm -rf "$REPO_ROOT/build/$1"
else
    rm -rf "$REPO_ROOT/build"
fi
