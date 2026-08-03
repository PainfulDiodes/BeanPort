#!/usr/bin/env bash

# Initialise BeanPort's submodules (pico-sdk and all of its own submodules).

set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

git -C "$REPO_ROOT" submodule update --init --recursive
