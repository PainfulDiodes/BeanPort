#!/usr/bin/env bash

# Deploy a built .uf2 to a connected Pico via picotool.
# Usage: ./deploy.sh [path-to-uf2]
# path defaults to build/src/blink.uf2
#
# Loads the firmware and reboots into it. Forces the device into BOOTSEL
# mode first if it's currently running application code, so there's no
# need to hold the BOOTSEL button manually.
#
# Requires: picotool

set -e

UF2=${1:-build/src/blink.uf2}

picotool load "$UF2" -f -x
