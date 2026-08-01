#!/usr/bin/env bash

# Initialise BeanPort's submodules.
#
# pico-sdk is pulled selectively - only lib/tinyusb, not
# mbedtls/lwip/btstack/cyw43-driver - to avoid pulling in Wi-Fi/Bluetooth
# stacks not needed until the Wi-Fi console variant is built.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

git -C "$SCRIPT_DIR" submodule update --init pico-sdk
git -C "$SCRIPT_DIR/pico-sdk" submodule update --init lib/tinyusb
