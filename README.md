# BeanPort

Raspberry Pi Pico firmware providing a USB terminal bridge for Z80, and potentially other 8-bit retro homebrew computers — a flexible, low-cost alternative to the FTDI UM245R USB-to-parallel-FIFO module.

The Pico presents a native address-mapped interface to the target system's bus, and a standard USB CDC serial port (driverless on macOS, Linux, and Windows) to the host. Bytes flow transparently in both directions.

## How it works

Two 8-bit registers, selected by a single address line (A0) as presented to the Pico:

| A0 | Name   | Access     | Meaning                                                                                    |
|----|--------|------------|--------------------------------------------------------------------------------------------|
| 0  | STATUS | Read-only  | bit 0 = read-available, bit 1 = write-ready (matches the 6850 ACIA's RDRF/TDRE convention) |
| 1  | DATA   | Read/write | write = byte to send to the host; read = byte received from the host                       |

"Read" and "Write" are from the target system's point of view. Which two actual port numbers these land on is a property of the target system's own address decoder — see the schematic for the specific mapping used there as an example. A dedicated external decoder (qualified by the target's I/O strobe) selects this register pair. The target should gate its own bus. 

### Signal path

A byte crosses through the same stages regardless of direction, just in reverse:

**Target → host** (target writes to DATA):

1. Target asserts write, drives the data bus
2. An address decoder / logic selects BeanPort's register pair producing an EN# signal and passes A0 through unchanged as the register-select line
3. Level shifters (5V → 3.3V, e.g. 74LVC245), relay the data bus and control signals to the Pico
4. A Pico PIO (Programmable I/O) state machine samples the data bus the instant the EN# is asserted
5. The byte is pushed to the PIO's own hardware FIFO
6. A dedicated Pico ARM core drains that FIFO and hands the byte to the other core over the RP2040's inter-core FIFO
7. The other core drains that and hands the byte to TinyUSB, which buffers and sends it over USB CDC

**Host → target** (target reads from DATA): the same stages, in reverse — USB CDC in, core 0, inter-core FIFO, core 1, PIO FIFO, driven onto the data bus within the target's read cycle.

Three buffers sit in that path, each doing a distinct job:

| Buffer                   | Depth                                         | Role                                                                                                                                                                                                                                                                                                                     |
|--------------------------|-----------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PIO state machine FIFO   | 1 word each direction                         | This is the communication channel between the PIO state machine and the ARM cores; its depth is pinned to <=1. It allows the interactions with the target to be triggered by a target signal edge - rather than the processing loop in the ARM core, which may not loop round in time to handle the target's write cycle |
| Inter-core FIFO          | 8 entries each direction (RP2040), 4 (RP2350) | Hands bytes between the two Pico ARM cores without either one blocking on the other                                                                                                                                                                                                                                      |
| USB CDC buffer (TinyUSB) | 64 bytes each direction                       | Matches bytes up with USB's own packet-based transfer rhythm - which may have interrupts                                                                                                                                                                                                                                 |

### Why two cores

USB CDC handling runs on core 0; the bus-facing loop runs alone on core 1. Separating the USB loop from the bus-facing loop means the bus-facing loop timing never depends on what the USB stack happens to be doing at any given moment.  A shared-core version of this loop occasionally duplicated a byte under a fast, unpaced round-trip burst. Splitting the two loops onto separate cores removed that dependency entirely.

STATUS's two bits are passed to the PIO by setting GPIOs which can be read by the PIO. They are refreshed by core 1 every loop iteration directly by checking the PIO FIFOs' occupancy. This method was confirmed safe for the target under sustained unpaced bursts even with that loop deliberately slowed down for testing — a wedged state machine stops responding rather than answering with stale data - which to the target looks like not ready to write, or no data to read.

## Status

Measured throughput: 10MHz Z80 target to host is solid at ~45KB/s sustained with zero data loss.

Host-to-target is reliable, tested with both terminal emulator and a Python source sending a raw, unthrottled burst write, without chunking or pacing.

A test to `cat` a file to the virtual serial port `/dev/cu.usb...` fails - I have not attempted to determine why other than to note that small packets of 32 bytes succeed, packets of 64 bytes or more fail. However, as per the previous note, scripted binary transmission from Python succeeds with larger files.

## Hardware

Built for Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), including wireless (`W`) variants. Tested only with RP2040, non-wireless.

Example schematic (KiCad): [kicad/beanport.pdf](kicad/beanport.pdf)

## Building and flashing

Firmware can be built from source - see Pico documentation for build toolchain.

Each board gets its own build directory  e.g. `build/pico2_w/`

Binary location: `build/<target>/bin/beanport.uf2`

uf2 files can be transferred to Pico with standard BOOTSEL.

**Note:** `picotool load -f` can trigger BOOTSEL remotely instead (see `scripts/deploy.sh`), but only once BeanPort (or any other firmware with `pico_stdio_usb`/TinyUSB running) is already on the board - a first flash, or recovering from non-USB firmware, still needs manual BOOTSEL.

## Possible future development

- **STATUS/CONFIG** STATUS is designed to double as a simple command channel for anything beyond byte transfer; STATUS is read-only today.
- **UM245R-compatible mode** — a drop-in replacement for boards built around the UM245R socket. UM245R's RD#/WR# are independent, asynchronous strobes rather than one shared enable line, so this would need 2 PIO state machines running a different PIO program
- **Wi-Fi console** — using Pico W / Pico 2 W hardware
- **RC2014 bus card** — packaging as a card for the RC2014 backplane
- **UART passthrough** — would give the target a UART alongside USB

## License

MIT — see [LICENSE](LICENSE).

## Links

- [BeanZee](https://github.com/PainfulDiodes/BeanZee)
- [BeanBoard](https://github.com/PainfulDiodes/BeanBoard)
- [Blog](https://painfuldiodes.wordpress.com)
