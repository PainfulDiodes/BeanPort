#!/usr/bin/env bash

# Deploy a built .uf2 to a connected Pico via picotool.
# Usage: ./deploy.sh [board]
# board defaults to pico; looks for build/<board>/bin/beanport.uf2
#
# Loads the firmware and reboots into it. Forces the device into BOOTSEL
# mode first if it's currently running application code, so there's no
# need to hold the BOOTSEL button manually.
#
# Requires: picotool

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BOARD=${1:-pico}
UF2="$SCRIPT_DIR/build/$BOARD/bin/beanport.uf2"

picotool load "$UF2" -f -x
