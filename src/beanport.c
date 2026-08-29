#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "beanport.pio.h"

#define PIO_BASE_PIN 0

#define READ_AVAILABLE_PIN 11

static PIO pio;
static uint sm;

// core 0: USB loop
// core 1: PIO loop
// USB processing / interrupts don't disturb PIO/bus operations

static void core1_main() {
    while (true) {
        
        // write-ready is tested by the PIO via mov status
        // Set read-available
        gpio_put(READ_AVAILABLE_PIN, !pio_sm_is_tx_fifo_empty(pio, sm));

        // pio -> core0 (USB)
        if (!pio_sm_is_rx_fifo_empty(pio, sm) && multicore_fifo_wready()) {
            uint32_t rx_frame = pio_sm_get(pio, sm);
            // doesn't block: wready checked
            multicore_fifo_push_blocking(rx_frame); 
        }

        // core0 (USB) -> pio
        if (multicore_fifo_rvalid() && !pio_sm_is_tx_fifo_full(pio, sm)) {
            // doesn't block: rvalid checked
            uint32_t word = multicore_fifo_pop_blocking(); 
            pio_sm_put(pio, sm, word);
        }
    }
}

int main() {
    stdio_init_all();

    pio = pio0;

    uint offset = pio_add_program(pio, &beanport_program);
    sm = pio_claim_unused_sm(pio, true);
    beanport_program_init(pio, sm, offset, PIO_BASE_PIN);

    gpio_init(READ_AVAILABLE_PIN);
    gpio_set_dir(READ_AVAILABLE_PIN, GPIO_OUT);

    multicore_launch_core1(core1_main);

    while (true) {

        // core1 bus -> USB
        if (multicore_fifo_rvalid()) {
            // doesn't block; rvalid checked
            uint32_t word = multicore_fifo_pop_blocking(); 
            putchar((uint8_t)word); // data in lowest 8 bits
        }

        // USB -> core1 bus
        if (multicore_fifo_wready()) {
            int c = stdio_getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                // doesn't block wready checked
                multicore_fifo_push_blocking((uint8_t)c);
            }
        }
    }
}
