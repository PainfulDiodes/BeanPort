# BeanPort

Raspberry Pi Pico firmware providing a USB terminal bridge for Z80, and potentially other 8-bit retro homebrew computers — a flexible, low-cost alternative to the FTDI UM245R USB-to-parallel-FIFO module.

The Pico presents a native address-mapped interface to the target system's bus, and a standard USB CDC serial port (driverless on macOS, Linux, and Windows) to the host. Bytes flow transparently in both directions.

## How it works

Two 8-bit registers, selected by a single address line (A0) as presented to the Pico:

| A0 | Name   | Access     | Meaning                                                                                    |
|----|--------|------------|--------------------------------------------------------------------------------------------|
| 0  | STATUS | Read-only  | bit 0 = read-available, bit 1 = write-ready (matches the 6850 ACIA's RDRF/TDRE convention) |
| 1  | DATA   | Read/write | write = byte to send to the host; read = byte received from the host                       |

"Read" and "write" are from the target system's point of view. Which two actual port numbers these land on is a property of the target system's own address decoder — see the schematic for the specific mapping used there as an example. A dedicated external decoder (qualified by the target's I/O strobe) selects this register pair. The Pico never has to gate its own bus access — a firmware bug can't cause output-driver contention.

### Signal path

A byte crosses through the same stages regardless of direction, just in reverse:

**Target → host** (target writes to DATA):

1. Target asserts the write strobe, drives the data bus
2. An external address decoder selects BeanPort's register pair, gating EN# and passing A0 through unchanged as the register-select line
3. 74LVC245 level shifters (5V → 3.3V) relay the data bus and control signals to the Pico
4. A PIO (Programmable I/O) state machine samples the data bus the instant the EN# strobe is recognized — PIO cycle-accurate, no ARM core involved
5. The byte lands in the PIO's own hardware FIFO
6. A dedicated core (core 1) drains that FIFO and hands the byte to the other core over the RP2040's inter-core FIFO
7. The other core (core 0) drains that and hands the byte to TinyUSB, which buffers and sends it over USB CDC

**Host → target** (target reads from DATA): the same stages, in reverse — USB CDC in, core 0, inter-core FIFO, core 1, PIO FIFO, driven onto the data bus for the target's read strobe.

Three buffers sit in that path, each doing a distinct job:

| Buffer                   | Depth                                         | Role                                                                                                                                                                                                                                                                                                                     |
|--------------------------|-----------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PIO state machine FIFO   | 4 words each direction                        | This is the communication channel between the PIO state machine and the ARM cores, its depth absorbs the gap between a bus cycle's fixed timing and the next time a core happens to service it |
| Inter-core FIFO          | 8 entries each direction (RP2040), 4 (RP2350) | Hands bytes between the two Pico ARM cores without either one blocking on the other                                                                                                                                                                                                                                               |
| USB CDC buffer (TinyUSB) | 64 bytes each direction                       | Matches bytes up with USB's own packet-based transfer rhythm                                                                                                                                                                                                                                                             |

None of these are managed by BeanPort's own code — it's two small polling loops moving bytes between fixed-size buffers that already exist in either the PIO hardware or the SDK's own USB stack.

### Why two cores

USB CDC handling runs on core 0; the bus-facing loop (steps 5-6 above, reversed for reads) runs alone on core 1. Separating this from the bus-facing loop means its timing never depends on what the USB stack happens to be doing at any given moment.  A shared-core version of this loop occasionally duplicated a byte under a fast, unpaced round-trip burst. Splitting the two loops onto separate cores removed that dependency entirely.

STATUS's two bits are passed to the PIO by setting GPIOs which can be read by the PIO. They are refreshed by core 1 every loop iteration directly by checking the PIO FIFOs' occupancy. This method was confirmed safe for the target under sustained unpaced bursts even with that loop deliberately slowed down for testing — a wedged state machine stops responding rather than answering with stale data.

## Status

Measured throughput: 10MHz Z80 target to host is solid at ~45KB/s sustained with zero data loss. Host-to-target is reliable for normal operation - using a terminal emulator for typing and pasting text file content, but a raw, unthrottled burst write from a naive client needs pacing (small chunks, a short delay between them). 

## Hardware

Built for Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), including wireless (`W`) variants. Tested only with RP2040, non-wireless.

Example schematic (KiCad): [kicad/beanport.pdf](kicad/beanport.pdf)

## Building and flashing

Firmware can be built from source - see Pico documentation for build toolchain.

Each board gets its own build directory  e.g. `build/pico2_w/`

Binary location: `build/<target>/bin/beanport.uf2`

uf2 files can be transferred to Pico with standard BOOTSEL.

## Possible future development

- **STATUS/CONFIG** STATUS is designed to double as a simple command channel for anything beyond byte transfer; STATUS is read-only today.
- **UM245R-compatible mode** — a drop-in replacement for boards built around the UM245R socket. UM245R's RD#/WR# are independent, asynchronous strobes rather than one shared enable line, so this would need 2 PIO state machines running a different PIO program
- **Wi-Fi console** — using Pico W / Pico 2 W hardware
- **RC2014 bus card** — packaging as a card for the RC2014 backplane
- **Dedicated bulk-transfer mode** — chunk+ACK handshaking for a future CLI/GUI host client, more robust than fixed-delay pacing
- **UART passthrough** — would give the target a UART alongside USB

## License

MIT — see [LICENSE](LICENSE).

## Links

- [BeanZee](https://github.com/PainfulDiodes/BeanZee)
- [BeanBoard](https://github.com/PainfulDiodes/BeanBoard)
- [Blog](https://painfuldiodes.wordpress.com)
