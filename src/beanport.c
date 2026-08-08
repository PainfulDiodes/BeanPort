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

static inline bool rx_ring_pop(uint8_t *byte) {
    if (rx_tail == rx_head) {
        return false;
    }
    *byte = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return true;
}

// Core 1: owns the PIO bus loop exclusively. Must never call anything that
// can block on USB (putchar() included) - stdio_usb.c's stdout path calls
// tud_task()/tud_cdc_write_flush() synchronously, and that latency must
// never sit between capturing a write and being ready to serve the next
// read. Bytes for the host are handed to Core 0 via the TX ring buffer;
// bytes from the host arrive via the RX ring buffer.
static void core1_entry() {
    while (true) {
        if (!pio_sm_is_rx_fifo_empty(pio0, bus_sm)) {
            uint32_t word = pio_sm_get(pio0, bus_sm);
            tx_ring_push(word & 0xFF); // port-agnostic since Step 12 - A0 ignored
        }

        uint8_t rx_byte;
        if (rx_ring_pop(&rx_byte)) {
            // Drop any stale, unconsumed served value first - the TX FIFO
            // has real queue depth, and a value pushed here only gets
            // consumed once a genuine Z80 read happens (PIO's "pull
            // noblock" drains oldest-first). Without this, a backlog rides
            // along as a permanent lag instead of ever catching up to
            // "latest". This also clears the write-capture RX FIFO
            // checked above - pio_sm_clear_fifos() can't target just one
            // FIFO - so a write landing in the narrow window between that
            // check and this call would be lost. Accepted as a known
            // interim gap until Step 14 gives each direction its own real
            // flow-controlled protocol.
            pio_sm_clear_fifos(pio0, bus_sm);
            // Both halves carry the same byte: port A and port B (the
            // Step 9/10 A0 test scaffold) now read identically. That
            // split only ever existed to prove A0 decoding works, not as
            // part of the final STATUS/DATA design (Step 14) - reusing
            // read_path's existing combined-word mechanism unchanged is
            // simpler than touching the PIO program to remove it.
            uint32_t combined = ((uint32_t)rx_byte << 8) | rx_byte;
            pio_sm_put(pio0, bus_sm, combined);
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
