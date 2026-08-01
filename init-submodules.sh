#!/usr/bin/env bash

# Initialise BeanPort's submodules (pico-sdk and all of its own submodules).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

git -C "$SCRIPT_DIR" submodule update --init --recursive
