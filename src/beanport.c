#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "beanport.pio.h"

#define PIO_BASE_PIN 0

#define READ_AVAILABLE_PIN 11

int main() {
    stdio_init_all();

    PIO pio = pio0;

    uint offset = pio_add_program(pio, &beanport_program);
    uint sm = pio_claim_unused_sm(pio, true);
    beanport_program_init(pio, sm, offset, PIO_BASE_PIN);

    gpio_init(READ_AVAILABLE_PIN);
    gpio_set_dir(READ_AVAILABLE_PIN, GPIO_OUT);

    while (true) {
        // write-ready is tested by the PIO via mov status
        // Set read-available
        gpio_put(READ_AVAILABLE_PIN, !pio_sm_is_tx_fifo_empty(pio, sm));

        // pio -> USB
        if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
            uint32_t rx_frame = pio_sm_get(pio, sm);
            putchar((uint8_t)rx_frame); // data in lowest 8 bits
        }

        // USB -> pio
        if (!pio_sm_is_tx_fifo_full(pio, sm)) {
            int c = stdio_getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                pio_sm_put(pio, sm, (uint8_t)c);
            }
        }
    }
}
