# BeanPort

Raspberry Pi Pico firmware providing a USB terminal bridge for Z80, and potentially other 8-bit retro homebrew computers — a flexible and low-cost alternative to the FTDI UM245R USB-to-parallel-FIFO module.

## Status

Early prototyping stage. The native interface — STATUS and DATA ports distinguished by address line A0 — is implemented and proven end-to-end on real hardware: a PIO state machine on the Pico captures Z80 I/O writes and drives Z80 I/O reads, gated by R/W and EN# (externally derived from IORQ# and address decoding), with the STATUS byte (TX-ready/RX-available bits) reflecting live PIO FIFO occupancy. Verified as a working bidirectional terminal bridge against real Z80 hardware: bytes typed in the Pico's USB terminal reach the target's console, and target keypresses reach the USB terminal, both directions live simultaneously with no pacing workarounds.

Still exploratory: only this one port pair is decoded, buffering is limited to the PIO's own ~4-word FIFOs (no ring buffer yet), and the firmware runs single-core. Currently testing throughput on this baseline before deciding whether a ring buffer and/or a dual-core split (moving USB CDC handling off the bus-facing loop) are actually needed.

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
