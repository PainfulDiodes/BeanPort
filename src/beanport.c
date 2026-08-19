#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "beanport.pio.h"

// Data: GPIO 0-7
// R/W: GPIO 8
// EN#: GPIO 9
// A0: GPIO 10

int main() {
    stdio_init_all();

    // gpio_init / gpio_set_dir GPIO_IN are not needed here - pins default to inputs

    // PIO function select for the data GPIOs so that they can be used as outputs
    // Address and control lines are only ever PIO inputs, so they are not included here
    for (int i = 0; i < 8; i++) pio_gpio_init(pio0, i);

    uint sm = pio_claim_unused_sm(pio0, true);
    uint offset = pio_add_program(pio0, &bus_capture_program);
    pio_sm_config sm_cfg = bus_capture_program_get_default_config(offset);
    sm_config_set_in_pins(&sm_cfg, 0); // IN base = GP0 (D0-D7)
    sm_config_set_jmp_pin(&sm_cfg, 8); // JMP pin = GP8 (R/W), for "jmp pin"
    // Shift left rather than the SDK default (right): with one 8-bit "in"
    // per push, this lands the byte directly in bits [7:0] of the pushed
    // word, so the C side can just cast rather than shift.
    sm_config_set_in_shift(&sm_cfg, false, false, 32);
    pio_sm_set_consecutive_pindirs(pio0, sm, 0, 8, false); // D0-D7 as inputs
    pio_sm_init(pio0, sm, offset, &sm_cfg);
    pio_sm_set_enabled(pio0, sm, true);

    while (true) {
        putchar((uint8_t)pio_sm_get_blocking(pio0, sm));
    }
}
