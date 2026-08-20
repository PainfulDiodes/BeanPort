#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "beanport.pio.h"

#define PIO_BASE_PIN 0

int main() {
    stdio_init_all();

    PIO pio = pio0;

    uint offset = pio_add_program(pio, &beanport_program);
    uint sm = pio_claim_unused_sm(pio, true);
    beanport_program_init(pio, sm, offset, PIO_BASE_PIN);

    while (true) {
        // pio -> USB, skips if there is no data
        if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
            uint32_t rx_frame = pio_sm_get(pio, sm);
            // beanport.pio puts the data into the low 8 bits so we can cast to a byte
            putchar((uint8_t)rx_frame);
        }

        // USB -> pio, skips if there is no data
        int c = stdio_getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            pio_sm_put(pio, sm, (uint8_t)c);
        }
    }
}
