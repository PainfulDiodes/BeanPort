# BeanPort

Raspberry Pi Pico firmware providing a USB terminal bridge for Z80, and potentially other 8-bit retro homebrew computers — a low-cost replacement for the FTDI UM245R USB-to-parallel-FIFO module, which can be hard to source.

## Status

Early prototyping stage, with the core read path proven end-to-end against real hardware: hardware address decoding (a 74LS138 producing a port-block enable, EN#) gates a PIO state machine that captures genuine Z80 I/O write cycles - qualified by R/W - and forwards each byte straight over USB. Verified with real keyboard input: typing on the BeanBoard's keyboard sends each character through Marvin's console, out a Z80 port write, through the address-decode/level-shifter/PIO capture chain, and out the Pico's USB as the same raw byte.

Still exploratory, not the final bridge protocol: only a single test port is decoded so far (no STATUS/DATA register scheme yet), no write path back to the Z80 (read direction switching is Pico-GPIO-input only for now), and R/W to the Pico is un-inverted Z80 RD# rather than the ACIA-convention polarity.

## Overview

The Pico acts as a transparent byte pipe between a host terminal (via USB CDC — driverless on macOS, Linux, and Windows) and an 8-bit system's bus. Primary target: a native interface (STATUS/DATA ports) for the BeanZee/BeanDeck homebrew computer.

## Future Directions

- **UM245R-compatible mode** — a drop-in replacement for boards built around the UM245R socket
- **Wi-Fi console** — on Pico W / Pico 2 W hardware
- **RC2014 bus card** — packaging as a card for the RC2014 backplane

A UART (TX/RX, optional CTS/RTS) passthrough mode is also worth adding opportunistically — it reuses the same USB/level-shifting infrastructure at near-zero marginal cost — but isn't a standalone justification the way the others are: commodity USB-UART adapters (CH340/CP2102/PL2303) are cheap and plentiful, unlike the FT245 parallel niche this project actually exists to address. It may instead be more valuable as a way to give a homebrew SBC like BeanZee its own UART alongside USB, rather than as a UM245R-style replacement in its own right.

## Hardware

- Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), including wireless (`W`) variants for the future Wi-Fi option
- SN74LVC245A octal buffer/level shifter (5V Z80 bus <-> 3.3V Pico) - one for data, one for control/address signals
- 74LS138 3-to-8 line decoder, for Z80 port-bank address decoding

Breadboard schematic (KiCad): [kicad/beanport.pdf](kicad/beanport.pdf)

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
./beanport/scripts/init-submodules.sh
```

`pico-sdk` is vendored as a submodule, along with all of its own submodules (TinyUSB, mbedtls, lwip, btstack, cyw43-driver).

Build:

```sh
./scripts/build.sh [board]
```

`board` defaults to `pico` (other options: `pico_w`, `pico2`, `pico2_w`). Each board gets its own build directory (`build/<board>/`). Output binaries land in `build/<board>/bin/`.

To remove build artefacts:

```sh
./scripts/clean.sh [board]
```

Removes `build/<board>` if given, otherwise the whole `build/` directory.

## Deploying

```sh
./scripts/deploy.sh [board]
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
