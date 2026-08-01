#!/usr/bin/env bash

# Remove BeanPort build artefacts.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

rm -rf "$SCRIPT_DIR/build"
