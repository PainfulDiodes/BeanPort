# BeanPort

Raspberry Pi Pico firmware providing a USB terminal bridge for Z80, and potentially other 8-bit retro homebrew computers — a flexible and low-cost alternative to the FTDI UM245R USB-to-parallel-FIFO module.

## Status

Early prototyping stage, with the core read and write paths proven end-to-end against real hardware: hardware address decoding, level-shifted lines to the Pico, a PIO state machine that captures a Z80 I/O write cycle (qualified by R/W which is Z80 RD#) - and forwards each byte straight over USB. Verified with keyboard input on BeanBoard keyboard sending to the Picos port address.

Still exploratory, only a single test port is decoded so far - no STATUS/DATA register scheme yet, so not possible to poll for available data on the z80 side.

## Overview

The Pico acts as a transparent byte pipe between a host terminal (via USB CDC — driverless on macOS, Linux, and Windows) and an 8-bit system's bus. Primary target: a native interface (STATUS/DATA ports) for the BeanZee/BeanDeck homebrew computer.

## Possible Future Development

- **UM245R-compatible mode** — a drop-in replacement for boards built around the UM245R socket
- **Wi-Fi console** — on Pico W / Pico 2 W hardware
- **RC2014 bus card** — packaging as a card for the RC2014 backplane

A UART (TX/RX, optional CTS/RTS) passthrough mode may also be worth adding, which wouldn't be justified on its own: USB-UART adapters are cheap and plentiful. It may instead be more valuable as a way to give BeanZee its own UART alongside USB (it doesn't have one).

## Hardware

- Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), including wireless (`W`) variants for the future Wi-Fi option
- SN74LVC245A octal buffer/level shifter (5V Z80 bus <-> 3.3V Pico) - one for data, one for control/address signals
- 74LS138 3-to-8 line decoder, for Z80 port-bank address decoding

Breadboard schematic (KiCad): [kicad/beanport.pdf](kicad/beanport.pdf)

## License

MIT — see [LICENSE](LICENSE).

## Links

- [BeanZee](https://github.com/PainfulDiodes/BeanZee)
- [BeanBoard](https://github.com/PainfulDiodes/BeanBoard)
- [Blog](https://painfuldiodes.wordpress.com)
