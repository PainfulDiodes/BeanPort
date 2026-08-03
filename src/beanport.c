#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "bus_capture.pio.h"
#include <ctype.h>
#include <stdio.h>

int main() {
    stdio_init_all();

    for (int i = 0; i < 8; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        pio_gpio_init(pio0, i);
    }

    // IORQ# and WR# (GP8/GP10, second level shifter) are read directly by
    // the PIO program via "jmp pin"/"wait gpio", which read the raw pad
    // state regardless of pin function - no pio_gpio_init needed for them.
    gpio_init(8);
    gpio_set_dir(8, GPIO_IN);
    gpio_init(10);
    gpio_set_dir(10, GPIO_IN);

    uint sm = pio_claim_unused_sm(pio0, true);
    uint offset = pio_add_program(pio0, &bus_capture_program);
    pio_sm_config sm_cfg = bus_capture_program_get_default_config(offset);
    sm_config_set_in_pins(&sm_cfg, 0); // IN base = GP0 (D0-D7)
    sm_config_set_jmp_pin(&sm_cfg, 8); // JMP pin = GP8 (IORQ#), for "jmp pin"
    // Shift left rather than the SDK default (right): with one 8-bit "in"
    // per push, this lands the byte directly in bits [7:0] of the pushed
    // word, so the C side can just cast rather than shift.
    sm_config_set_in_shift(&sm_cfg, false, false, 32);
    pio_sm_set_consecutive_pindirs(pio0, sm, 0, 8, false); // D0-D7 as inputs
    pio_sm_init(pio0, sm, offset, &sm_cfg);
    pio_sm_set_enabled(pio0, sm, true);

    while (true) {
        int c = getchar();
        putchar(toupper(c));
        putchar(' ');

        while (!pio_sm_is_rx_fifo_empty(pio0, sm)) {
            printf("%02X ", (uint8_t)pio_sm_get(pio0, sm));
        }
        putchar('\n');
    }
}
