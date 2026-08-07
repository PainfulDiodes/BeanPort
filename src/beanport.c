#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "bus_capture.pio.h"
#include <stdio.h>

int main() {
    stdio_init_all();

    for (int i = 0; i < 8; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        pio_gpio_init(pio0, i);
    }

    // R/W and EN# (GP8/GP9) are read directly by
    // the PIO program via "jmp pin"/"wait gpio", which read the raw pad
    // state regardless of pin function - no pio_gpio_init needed for them.
    // GP10 (A0) isn't used by the PIO program yet - reserved for the
    // register-scheme work.
    gpio_init(8);
    gpio_set_dir(8, GPIO_IN);
    gpio_init(9);
    gpio_set_dir(9, GPIO_IN);

    uint sm = pio_claim_unused_sm(pio0, true);
    uint offset = pio_add_program(pio0, &bus_capture_program);
    pio_sm_config sm_cfg = bus_capture_program_get_default_config(offset);
    sm_config_set_in_pins(&sm_cfg, 0);  // IN base = GP0 (D0-D7)
    sm_config_set_out_pins(&sm_cfg, 0, 8); // OUT base = GP0, count 8 (D0-D7)
    sm_config_set_jmp_pin(&sm_cfg, 8); // JMP pin = GP8 (R/W), for "jmp pin"
    // Shift left rather than the SDK default (right): with one 8-bit "in"
    // per push, this lands the byte directly in bits [7:0] of the pushed
    // word, so the C side can just cast rather than shift.
    sm_config_set_in_shift(&sm_cfg, false, false, 32);
    pio_sm_set_consecutive_pindirs(pio0, sm, 0, 8, false); // D0-D7 as inputs
    pio_sm_init(pio0, sm, offset, &sm_cfg);
    pio_sm_set_enabled(pio0, sm, true);

    while (true) {
        if (!pio_sm_is_rx_fifo_empty(pio0, sm)) {
            uint8_t b = (uint8_t)pio_sm_get(pio0, sm);
            putchar(b);
            // Drop any stale, unconsumed echo value first - the TX FIFO
            // has real queue depth, and a byte pushed here only gets
            // consumed once a genuine read happens (PIO's "pull noblock"
            // drains oldest-first). Without this, a backlog (e.g. from
            // spurious captures during power-up) rides along as a
            // permanent lag instead of ever catching up to "latest".
            pio_sm_clear_fifos(pio0, sm);
            pio_sm_put(pio0, sm, b); // echo back for the next Z80 read
        }
    }
}
