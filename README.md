# BeanPort

Raspberry Pi Pico firmware providing a USB terminal bridge for Z80, and potentially other 8-bit retro homebrew computers — a low-cost replacement for the FTDI UM245R USB-to-parallel-FIFO module, which can be hard to source.

## Status

Early prototyping stage, but past the toolchain-only milestone: real Z80 data has been verified flowing end-to-end - BeanBoard GPIO output -> SN74LVC245A level shifter -> Pico GPIO input -> USB. The current `beanport` firmware echoes typed characters back over USB (uppercased, to prove it's actually round-tripping through the Pico) and, on each keystroke, reads GP0-GP7 (D0-D7 from the level shifter) and prints the byte as a hex pair - confirmed against known values sent by a BeanBoard test program (`0x55`, then `0x44`).

Still exploratory, not the final bridge protocol: capture is a free-running poll (not synced to WR#/IORQ#), no address decoding/port discrimination yet, no write path back to the Z80, no PIO.

## Overview

The Pico acts as a transparent byte pipe between a host terminal (via USB CDC — driverless on macOS, Linux, and Windows) and an 8-bit system's bus. Primary target: a native interface (STATUS/DATA ports) for the BeanZee/BeanDeck homebrew computer.

## Hardware

- Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), including wireless (`W`) variants for the future Wi-Fi option
- SN74LVC245A octal buffer/level shifter (5V Z80 bus <-> 3.3V Pico)

## Building

Requirements (macOS via Homebrew):

```sh
brew install cmake
brew install --cask gcc-arm-embedded
brew install picotool
```

Clone with submodules:

```sh
git clone https://github.com/PainfulDiodes/beanport.git
./beanport/init-submodules.sh
```

`pico-sdk` is vendored as a submodule, along with all of its own submodules (TinyUSB, mbedtls, lwip, btstack, cyw43-driver).

Build:

```sh
./build.sh [board]
```

`board` defaults to `pico` (other options: `pico_w`, `pico2`, `pico2_w`). Each board gets its own build directory (`build/<board>/`). Output binaries land in `build/<board>/bin/`.

To remove build artefacts:

```sh
./clean.sh [board]
```

Removes `build/<board>` if given, otherwise the whole `build/` directory.

## Deploying

```sh
./deploy.sh [board]
```

`board` defaults to `pico`; looks for `build/<board>/bin/beanport.uf2`.

Uses `picotool load -f -x` — forces the device into BOOTSEL mode if it's currently running application code (no need to hold the physical button), loads the firmware, and reboots into it. This only works if the running firmware exposes USB (e.g. via TinyUSB/`pico_stdio_usb`); otherwise put the Pico in BOOTSEL mode manually first (hold BOOTSEL while plugging in).

## License

MIT — see [LICENSE](LICENSE).

## Links

- [BeanZee](https://github.com/PainfulDiodes/BeanZee)
- [BeanBoard](https://github.com/PainfulDiodes/BeanBoard)
- [BeanBoardSPI](https://github.com/PainfulDiodes/BeanBoardSPI)
- [Blog](https://painfuldiodes.wordpress.com)
