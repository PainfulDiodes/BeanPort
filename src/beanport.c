#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "bus_capture.pio.h"
#include <stdio.h>

static uint bus_sm;

#define TX_BUF_SIZE 256
static volatile uint8_t tx_buf[TX_BUF_SIZE];
static volatile uint16_t tx_head = 0; // next slot to write - Core 1 only
static volatile uint16_t tx_tail = 0; // next slot to read - Core 0 only

// Single-producer (Core 1)/single-consumer (Core 0) ring buffer: each index
// is only ever written by the core that owns it, so this needs no locking -
// RP2040's two cores share SRAM directly, with no per-core cache to desync.
// Sized to match design.md's 256-byte earmark for this direction (matches
// the FT245R's own TX buffer), decoupling the Z80 write rate from USB's
// actual drain rate. If full, the byte is dropped - Step 14 adds a real
// TX-ready status bit so the Z80 self-throttles instead of hitting this.
static inline void tx_ring_push(uint8_t byte) {
    uint16_t next = (tx_head + 1) % TX_BUF_SIZE;
    if (next == tx_tail) {
        return;
    }
    tx_buf[tx_head] = byte;
    tx_head = next;
}

#define RX_BUF_SIZE 256
static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0; // next slot to write - Core 0 only
static volatile uint16_t rx_tail = 0; // next slot to read - Core 1 only

// Symmetric counterpart to tx_ring_push - same single-producer (Core 0)/
// single-consumer (Core 1) reasoning, reversed roles. Fed from genuine
// incoming USB bytes rather than an echo of the Z80's own writes (that was
// the Step 9/10 A0 test scaffold, now retired - see core1_entry). If full,
// the byte is dropped - same interim policy as tx_ring_push, ahead of
// Step 14's real RX-available flow control.
static inline void rx_ring_push(uint8_t byte) {
    uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
    if (next == rx_tail) {
        return;
    }
    rx_buf[rx_head] = byte;
    rx_head = next;
}

// Advances past the byte currently being served on the DATA port - called
// only in response to a confirmed DATA read-completion notification
// (core1_entry), never speculatively. That's what lets RX-available
// reflect reality: it clears at the instant the Z80 actually reads DATA,
// not whenever C happens to notice a new USB byte.
static inline void rx_ring_advance(void) {
    if (rx_tail != rx_head) {
        rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    }
}

// STATUS bits (A0=0, read-only): bit 0 = TX ready (space in the TX ring),
// bit 1 = RX available (an unread byte waiting in the RX ring).
#define STATUS_TX_READY 0x01
#define STATUS_RX_AVAILABLE 0x02

// Core 1: owns the PIO bus loop exclusively. Must never call anything that
// can block on USB (putchar() included) - stdio_usb.c's stdout path calls
// tud_task()/tud_cdc_write_flush() synchronously, and that latency must
// never sit between capturing a write and being ready to serve the next
// read. Bytes for the host are handed to Core 0 via the TX ring buffer;
// bytes from the host arrive via the RX ring buffer.
static void core1_entry() {
    uint32_t served = 0xFFFFFFFF; // forces the first real snapshot to be pushed
    static volatile uint8_t diag_advance_count = 0; // TEMP diagnostic, Step 14c

    while (true) {
        // Drain every notification currently queued, not just one - the
        // FIFO holds up to 4, and a Z80 read immediately followed by a
        // write (e.g. "in a,(DATA)" then "out (DATA),a", exactly what an
        // echo does) is fast enough for both notifications to arrive
        // before this loop gets to either. Popping only one per pass and
        // then recomputing below (which calls pio_sm_clear_fifos() when
        // anything changed) would wipe the second notification while it
        // was still sitting here unprocessed - not a rare race, but a
        // near-certainty for that exact instruction pattern.
        while (!pio_sm_is_rx_fifo_empty(pio0, bus_sm)) {
            uint32_t word = pio_sm_get(pio0, bus_sm);
            // read_path (Step 14a) and write_path both push into this same
            // FIFO - bit 8 (R/W) tells them apart: 1 = write, 0 = a read
            // just completed. Bit 10 (A0) selects STATUS (0) vs DATA (1).
            bool write = (word >> 8) & 1;
            bool a0 = (word >> 10) & 1;
            if (write) {
                if (a0) { // DATA write - STATUS is read-only, ignore A0=0
                    tx_ring_push(word & 0xFF);
                }
            } else if (a0) {
                // A genuine DATA read just completed - advance past the
                // byte that was served, so the next STATUS/DATA snapshot
                // reflects it being gone. STATUS (A0=0) reads are
                // non-destructive by design - nothing to do for those.
                rx_ring_advance();
                diag_advance_count++;
            }
        }

        // Recompute the current STATUS/DATA snapshot every pass - cheap
        // (just reading a few volatile head/tail indices, no PIO access)
        // - but only touch the PIO if it actually changed. That keeps
        // pio_sm_clear_fifos() calls (which also clear the write/read
        // notification FIFO checked above, and so can lose a notification
        // landing in the same narrow window Step 14a already accepted as
        // an interim gap) down to genuine changes, not every single loop
        // pass. Cross-core reads of tx_tail (Core 0-owned) are safe here:
        // single writer, and a uint16_t read is atomic on this hardware.
        bool tx_ready = ((tx_head + 1) % TX_BUF_SIZE) != tx_tail;
        bool rx_available = (rx_tail != rx_head);
        uint8_t data_byte = rx_available ? rx_buf[rx_tail] : 0;
        uint8_t status_byte = (tx_ready ? STATUS_TX_READY : 0) |
                               (rx_available ? STATUS_RX_AVAILABLE : 0) |
                               (diag_advance_count << 2); // TEMP diagnostic, Step 14c
        uint32_t combined = ((uint32_t)data_byte << 8) | status_byte;

        if (combined != served) {
            pio_sm_clear_fifos(pio0, bus_sm);
            pio_sm_put(pio0, bus_sm, combined);
            served = combined;
        }
    }
}

int main() {
    stdio_init_all();

    for (int i = 0; i < 8; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        pio_gpio_init(pio0, i);
    }

    // R/W, EN#, and A0 (GP8/GP9/GP10) are read directly by the PIO
    // program via "jmp pin"/"wait gpio"/"in pins", all of which read the
    // raw pad state regardless of pin function - no pio_gpio_init needed
    // for them (unlike GP0-7, which also get driven as outputs on a
    // read, so do need it).
    gpio_init(8);
    gpio_set_dir(8, GPIO_IN);
    gpio_init(9);
    gpio_set_dir(9, GPIO_IN);
    gpio_init(10);
    gpio_set_dir(10, GPIO_IN);

    bus_sm = pio_claim_unused_sm(pio0, true);
    uint offset = pio_add_program(pio0, &bus_capture_program);
    pio_sm_config sm_cfg = bus_capture_program_get_default_config(offset);
    sm_config_set_in_pins(&sm_cfg, 0);  // IN base = GP0 (D0-D7)
    sm_config_set_out_pins(&sm_cfg, 0, 8); // OUT base = GP0, count 8 (D0-D7)
    sm_config_set_jmp_pin(&sm_cfg, 8); // JMP pin = GP8 (R/W), for "jmp pin"
    // Shift left rather than the SDK default (right): with one 11-bit
    // "in" per push, this lands the sample directly in bits [10:0] of
    // the pushed word (D0-D7 in bits 0-7, R/W in bit 8, EN# in bit 9,
    // A0 in bit 10), so the C side can just mask rather than shift.
    sm_config_set_in_shift(&sm_cfg, false, false, 32);
    pio_sm_set_consecutive_pindirs(pio0, bus_sm, 0, 8, false); // D0-D7 as inputs
    pio_sm_init(pio0, bus_sm, offset, &sm_cfg);
    pio_sm_set_enabled(pio0, bus_sm, true);

    multicore_launch_core1(core1_entry);

    // Core 0: USB/CDC only. Drains the TX ring buffer Core 1 fills and
    // writes bytes out - putchar()'s blocking is confined to this core, so
    // it can never stall the PIO bus loop on Core 1. Also polls incoming
    // USB bytes (non-blocking) into the RX ring buffer for Core 1 to serve
    // to the Z80.
    while (true) {
        if (tx_tail != tx_head) {
            putchar(tx_buf[tx_tail]);
            tx_tail = (tx_tail + 1) % TX_BUF_SIZE;
        }

        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            rx_ring_push((uint8_t)c);
        }
    }
}
