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

    PIO pio = pio0;

    // PIO function select for the data GPIOs so that they can be used as outputs
    // Address and control lines are only ever PIO inputs, so they are not included here
    for (int i = 0; i < 8; i++) pio_gpio_init(pio, i);
    
    // Load the beanport PIO program, and configure a free state machine to run it
    uint offset = pio_add_program(pio, &beanport_program);
    uint sm = pio_claim_unused_sm(pio, true);
    beanport_program_init(pio, sm, offset);

    while (true) {
        putchar((uint8_t)pio_sm_get_blocking(pio, sm));
    }
}
