#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "tusb.h"

#include "beanport.pio.h"

#define PIO_BASE_PIN 0

#define TX_READY_PIN 11
#define RX_AVAILABLE_PIN 12

int main() {
    // Still needed for its side effect of bringing up the USB device stack
    // and TinyUSB's own background low-priority IRQ (which keeps tud_task()
    // running independent of this loop) - but I/O below goes straight
    // through tud_cdc_read()/write() rather than stdio's getchar/putchar,
    // to avoid stdio_usb's own mutex: this loop is the only caller of USB
    // I/O here, so the extra locking (meant for printf being callable from
    // arbitrary interrupt handlers) buys nothing, and at the call rate this
    // loop runs at, repeatedly taking that mutex risks delaying the
    // background IRQ's own attempts to acquire it and service tud_task().
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

        // pio -> USB, skips if there is no data. Always drain the PIO FIFO
        // when there's a byte, even if USB isn't connected - otherwise it
        // fills and TX_READY blocks the Z80 indefinitely with nothing
        // listening on the other end.
        if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
            uint32_t rx_frame = pio_sm_get(pio, sm);
            // beanport.pio puts the data into the low 8 bits so we can cast to a byte
            uint8_t byte = (uint8_t)rx_frame;
            if (tud_cdc_connected()) {
                tud_cdc_write(&byte, 1);
                tud_cdc_write_flush();
            }
        }

        // USB -> pio, skips if there is no data or the PIO FIFO has no room -
        // leaves the byte in TinyUSB's own buffer rather than pulling it and
        // then silently dropping it when pio_sm_put() finds the FIFO full
        if (!pio_sm_is_tx_fifo_full(pio, sm) && tud_cdc_connected() && tud_cdc_available()) {
            uint8_t byte;
            if (tud_cdc_read(&byte, 1)) {
                pio_sm_put(pio, sm, byte);
            }
        }
    }
}
