#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "bus_capture.pio.h"
#include <stdio.h>

static uint bus_sm;

// Core 1: owns the PIO bus loop exclusively. Must never call anything that
// can block on USB (putchar() included) - stdio_usb.c's stdout path calls
// tud_task()/tud_cdc_write_flush() synchronously, and that latency must
// never sit between capturing a write and being ready to serve the next
// read. Bytes for the host are handed to Core 0 via the inter-core FIFO.
static void core1_entry() {
    // Step 10: two independent per-port bytes, not real STATUS/DATA
    // storage yet - just enough to prove the read side can tell port A
    // (A0=0) and port B (A0=1) apart, same spirit as Step 9's write-side
    // test. Always pushed combined (port B in the high byte, port A in
    // the low byte) so the PIO's single pull/X-fallback mechanism can
    // carry both at once - see bus_capture.pio's read_path.
    uint8_t port_a_val = 0;
    uint8_t port_b_val = 0;

    while (true) {
        if (!pio_sm_is_rx_fifo_empty(pio0, bus_sm)) {
            uint32_t word = pio_sm_get(pio0, bus_sm);
            uint8_t data = word & 0xFF;
            bool a0 = (word >> 10) & 1;
            if (a0) {
                port_b_val = data;
            } else {
                port_a_val = data;
            }
            uint32_t combined = ((uint32_t)port_b_val << 8) | port_a_val;
            // Drop any stale, unconsumed echo value first - the TX FIFO
            // has real queue depth, and a byte pushed here only gets
            // consumed once a genuine read happens (PIO's "pull noblock"
            // drains oldest-first). Without this, a backlog (e.g. from
            // spurious captures during power-up) rides along as a
            // permanent lag instead of ever catching up to "latest".
            pio_sm_clear_fifos(pio0, bus_sm);
            pio_sm_put(pio0, bus_sm, combined); // both bytes, for the next Z80 read
            multicore_fifo_push_blocking(data);
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

    // Core 0: USB/CDC only. Drains bytes handed over by Core 1 and writes
    // them out - putchar()'s blocking is confined to this core, so it can
    // never stall the PIO bus loop on Core 1.
    while (true) {
        uint32_t data = multicore_fifo_pop_blocking();
        putchar((uint8_t)data);
    }
}
