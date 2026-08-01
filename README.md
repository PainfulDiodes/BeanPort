# BeanPort

Raspberry Pi Pico firmware providing a USB terminal bridge for Z80, and potentially other 8-bit retro homebrew computers — a low-cost replacement for the FTDI UM245R USB-to-parallel-FIFO module, which can be hard to source.

## Status

Early prototyping stage. The build toolchain is proven end-to-end (CMake + pico-sdk + an LED-blink smoke test, currently the `beanport` target, builds, flashes, and runs on both a plain Pico and a Pico 2 W), but the actual USB terminal bridge firmware (TinyUSB CDC, native register-mapped bus interface) hasn't been written yet. Level shifting (SN74LVC245A) between a host Z80 system and the Pico has been breadboarded and proven separately.

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

`pico-sdk` is vendored as a submodule, pulled selectively (only `lib/tinyusb`, not `mbedtls`/`lwip`/`btstack`/`cyw43-driver`) since those aren't needed until a future Wi-Fi variant.

Build:

```sh
./build.sh [board]
```

`board` defaults to `pico` (other options: `pico_w`, `pico2`, `pico2_w`). Each board gets its own build directory (`build/<board>/`). Output binaries land in `build/<board>/bin/`.

`pico_w`/`pico2_w` builds additionally require the `cyw43-driver` submodule, which isn't pulled by default - see `init-submodules.sh`.

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
