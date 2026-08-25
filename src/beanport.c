#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "beanport.pio.h"

#define PIO_BASE_PIN 0

#define TX_READY_PIN 11
#define RX_AVAILABLE_PIN 12

int main() {
    stdio_init_all();

    PIO pio = pio0;

    uint offset = pio_add_program(pio, &beanport_program);
    uint sm = pio_claim_unused_sm(pio, true);
    beanport_program_init(pio, sm, offset, PIO_BASE_PIN);

    gpio_init(TX_READY_PIN);
    gpio_set_dir(TX_READY_PIN, GPIO_OUT);
    gpio_init(RX_AVAILABLE_PIN);
    gpio_set_dir(RX_AVAILABLE_PIN, GPIO_OUT);

    while (true) {
        // TX ready: room for the Z80 to write another byte (capture's own RX FIFO isn't full)
        gpio_put(TX_READY_PIN, !pio_sm_is_rx_fifo_full(pio, sm));
        // RX available: a byte is waiting for the Z80 to read (read_path's own TX FIFO isn't empty)
        gpio_put(RX_AVAILABLE_PIN, !pio_sm_is_tx_fifo_empty(pio, sm));

        // pio -> USB, skips if there is no data
        if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
            uint32_t rx_frame = pio_sm_get(pio, sm);
            // beanport.pio puts the data into the low 8 bits so we can cast to a byte
            putchar((uint8_t)rx_frame);
        }

        // USB -> pio, skips if there is no data or the PIO FIFO has no room -
        // leaves the byte in TinyUSB's own buffer rather than pulling it and
        // then silently dropping it when pio_sm_put() finds the FIFO full
        if (!pio_sm_is_tx_fifo_full(pio, sm)) {
            int c = stdio_getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                pio_sm_put(pio, sm, (uint8_t)c);
            }
        }
    }
}
