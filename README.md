# BeanPort

Raspberry Pi Pico firmware providing a USB console bridge for Z80, and potentially other 8-bit retro homebrew computers — a modern, low-cost replacement for the FTDI UM245R USB-to-parallel-FIFO module, which can be hard to source, and is relatively expensive.

## Status

Early design/prototyping stage — no firmware has been written yet. Level shifting (SN74LVC245A) between a host Z80 system and the Pico has been breadboarded and proven; firmware development (USB CDC via TinyUSB, native register-mapped bus interface) is the current focus.

This README will be filled out with build instructions, wiring diagrams, and usage once there's working firmware to document.

## Overview

The Pico acts as a transparent byte pipe between a host terminal (via USB CDC — driverless on macOS, Linux, and Windows) and an 8-bit system's I/O bus. Primary target: a native register-mapped interface (STATUS/DATA ports) for the BeanZee/BeanDeck homebrew computer.

## Hardware

- Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), including wireless (`W`) variants for the future Wi-Fi console option
- SN74LVC245A octal buffer/level shifter (5V Z80 bus <-> 3.3V Pico)

## License

MIT — see [LICENSE](LICENSE).

## Links

- [BeanZee](https://github.com/PainfulDiodes/BeanZee)
- [BeanBoard](https://github.com/PainfulDiodes/BeanBoard)
- [BeanBoardSPI](https://github.com/PainfulDiodes/BeanBoardSPI)
- [Blog](https://painfuldiodes.wordpress.com)
