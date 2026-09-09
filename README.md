# BeanPort

Raspberry Pi Pico firmware providing a USB bridge for retrocomputers.

Most homebrew retrocomputing designs will provide a UART interface and use an FTDI adapter to connect a modern host over USB to the retrocomputer "target". This means dealing with baud rates, and either fixing the CPU clock to a UART-friendly rate, or having a separate clock for the UART.

BeanPort provides an alternative solution: a Pico replaces the UART on the target system, and provides the USB connection to a host computer. This removes the need to consider baud rates, allowing the CPU clock to be set independently, and provides faster transmission speeds.

This approach is not new: the FTDI UM245R USB-to-parallel-FIFO module provides the same function, but is not as easy to obtain as a Pico, and has a higher cost. The Pico could also potentially provide Wi-Fi connectivity.

The Pico presents a native address-mapped interface to the target system's bus, and a standard USB CDC serial port (driverless on macOS, Linux, and Windows) to the host. Bytes flow transparently with buffering in both directions — able to run a virtual terminal to the retrocomputer from the host system.

## Status

Target-to-host:

- Works reliably with a 10MHz Z80 target executing a tight loop: checking BeanPort status and then sending data to the USB host
- As expected, overruns the buffer if no flow control is used writing as fast as possible (delays may be used to pace the transmission)

Host-to-target:

- Works reliably for a terminal emulator sending data to the target
- Works reliable for a Python script sending binary daoa to the target
- Fails when sending a file to the target via the `cat` command (an identical test succeeds with the UM245R) - under investigation

## How it works

BeanPort provides two 8-bit registers, distinguished by a Register Select (RS) input, which will typically be mapped to the target's A0:

| RS/A0 | Name   | Access     | Meaning                                                                                    |
|-------|--------|------------|--------------------------------------------------------------------------------------------|
| 0     | STATUS | read-only  | bit 0 = read-available, bit 1 = write-ready (matches the 6850 ACIA's RDRF/TDRE convention) |
| 1     | DATA   | read/write | write = byte to send to the host; read = byte received from the host                       |

"Read" and "Write" are from the target system's point of view. Which two actual port numbers these land on is a property of the target system's own address decoder — see the schematic for the specific mapping used there as an example. The decoder / glue logic provides an enable (EN#) signal, plus whatever logic the target's own bus needs to produce R/W. For a Z80 target: RD# maps directly to R/W.

Reading DATA when none is available returns `0x00`.

### Signal path

A byte crosses through several stages:

**Target → host**

1. Target asserts write, drives the data bus
2. An address decoder / glue logic selects BeanPort's register pair producing an EN# signal, a R/W signal, and passes A0 straight through as register-select (RS)
3. Level shifters (5V → 3.3V, e.g. 74LVC245), relay the data bus and control signals to the Pico
4. A Pico PIO (Programmable I/O) state machine samples the data bus the instant the EN# is asserted
5. The byte is pushed to the PIO's own hardware FIFO
6. A Pico ARM core (core1) drains the FIFO and hands the byte to a second ARM core (core0) over the RP2040's inter-core FIFO
7. core0 drains the inter-core FIFO and hands the byte to TinyUSB, which buffers and sends it over USB CDC to the host

**Host → target**

1. The host sends data over USB to the Pico, where it is buffered
2. Pico core0 drains the TinyUSB buffer and pushes the byte into the inter-core FIFO
3. Pico core1 drains the inter-core FIFO and pushes the byte into the PIO FIFO
4. The target system asserts read to the BeanPort's data address
5. An address decoder / glue logic selects BeanPort's register pair producing an EN# signal, a R/W signal, and passes A0 straight through as register-select (RS)
6. Level shifters (5V → 3.3V, e.g. 74LVC245), relay the control signals to the Pico, and set the direction for the data from Pico to target
7. Once the EN# is asserted, the Pico PIO state machine pulls data from the PIO FIFO
8. The PIO drives the data bus with the pulled byte
9. The data level shifter (3.3V → 5V), relays the data signals to the target system

The host-driven steps (1-3) and the target-driven steps (4-9) operate independently of each other. The Pico's USB system will manage flow control with the host. The target will be told through the status register when there is data to read, or when it can send more data:

**Status → target**

1. The target system asserts read to the BeanPort's status address
2. An address decoder / glue logic selects BeanPort's register pair producing an EN# signal, a R/W signal, and passes A0 straight through as register-select (RS)
3. Level shifters (5V → 3.3V, e.g. 74LVC245), relay the control signals to the Pico, and set the direction for the data from Pico to target
4. Once the EN# is asserted, the Pico PIO state machine gathers the status data
5. The PIO drives the data bus with the status data
6. The data level shifter (3.3V → 5V), relays the data signals to the target system

### Why two cores

USB CDC handling runs on core0; the bus-facing loop runs alone on core1. Separating the USB loop from the bus-facing loop means the bus-facing loop timing never depends on what the USB stack happens to be doing at any given moment.

### Target status register

The PIO takes the read-available signal directly from the status of the PIO FIFO. It will report "read-available" if the FIFO is not empty.

The PIO cannot be configured to check the status of both PIO FIFOs. Since read-available status already uses that mechanism, write-ready has to work differently: it's determined by the core1 program instead, and set on an additional GPIO pin, which the PIO reads as its write-ready status.

This means that write-ready status may be stale when the target checks it - the core1 program sets the status in a loop, and so the write-ready status will be set a little time after the FIFO's state changes.

When the status changes from ready to not-ready, the target may send data thinking that the BeanPort is still ready. In this case the PIO FIFO is able to absorb one or two additional target writes so that no data is lost.

## Hardware

Built for Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), including wireless (`W`) variants. To date, only tested with a non-wireless RP2040.

Example schematic (KiCad): [kicad/beanport.pdf](kicad/beanport.pdf)

## Building and flashing

The repo's uf2 files can be transferred to Pico with standard BOOTSEL. Binary location: `build/<target>/bin/beanport.uf2`

Firmware can be built from source - see Pico documentation for build toolchain.

Each board gets its own build directory, e.g. `build/pico2_w/`

## Possible future development

- **Wi-Fi console** — using Pico W / Pico 2 W hardware
- **UART passthrough** — would give the target a UART alongside USB
- **RC2014 bus card** — packaging as a card for the RC2014 backplane

## License

MIT — see [LICENSE](LICENSE).

## Links

- [BeanZee](https://github.com/PainfulDiodes/BeanZee)
- [BeanBoard](https://github.com/PainfulDiodes/BeanBoard)
- [Blog](https://painfuldiodes.wordpress.com)
