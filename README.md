# BeanPort

Raspberry Pi Pico firmware providing a USB terminal bridge for Z80, and potentially other 8-bit retro homebrew computers — a low-cost replacement for the FTDI UM245R USB-to-parallel-FIFO module, which can be hard to source.

## Status

Prototyping stage, with both directions of communication between USB and the Z80 proven end-to-end against real hardware.

Pico is connected to the Z80 bus: data, address, IORQ#, RD# (WR inferred form RD). Address/IORQ# decoded by a 74LS138 producing a port-block enable (EN#) and all the lines to the Pico level-shifted from 5v to 3v3. Level-shifters are tri-state so avoid bus contention (bus connection controlled by the Z80).

EN# gates a single PIO state machine that both captures genuine Z80 I/O write cycles qualified by R/W (RD#)) and, on a read, switches the data pins to outputs and drives a byte back - entirely within the PIO program - since the 10MHz Z80's ~200ns timing budget doesn't allow waiting on a C program using interrupts. An address line (A0) is also sampled, decoding two ports rather than one, proven independently on both the write and read sides.

Firmware runs split across both RP2040 cores: Core 1 owns the PIO bus loop exclusively and never calls anything that can block, while Core 0 owns USB/CDC - `putchar()` and friends can and do block on a USB frame, and that latency must never be able to stall the live, cycle-accurate bus timing on Core 1. The two cores hand data across via a pair of 256-byte software ring buffers, one per direction (Z80 write -> USB, and USB -> Z80 read), replacing an earlier single-cached-byte approach and giving each direction real buffering against bursty traffic - verified with deliberate overrun testing (fills the buffer faster than USB can drain it) as well as normal round-trip exchange.

Still exploratory, not the final bridge protocol: the two decoded ports aren't yet mapped to real STATUS/DATA register semantics (no flow control - a full buffer just drops incoming bytes for now), and R/W to the Pico is un-inverted Z80 RD# rather than the ACIA-convention polarity.

Next: give the two ports real register semantics - one read-only STATUS port (TX-ready / RX-available bits, reflecting the actual ring buffer fill levels) and one DATA port (read/write), so the Z80 side can poll readiness instead of relying on the bridge silently dropping bytes when a buffer fills up.

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
