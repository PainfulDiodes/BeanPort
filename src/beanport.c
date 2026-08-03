#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <ctype.h>
#include <stdio.h>

// Pico pin for IORQ# from the second level shifter, active-low. This is
// the Z80's general I/O-request strobe - it fires for every port access
// (GPIO, LCD, keyboard, USB status/data, ...), not just the BeanBoard's
// GPIO port; nothing here discriminates by port yet.
#define IORQ_PIN 8

// GP0-GP7 = D0-D7 from the level shifter's B-side
static uint8_t read_data_bus(void) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= gpio_get(i) << i;
    }
    return value;
}

// Data-bus snapshots taken on IORQ# falling edges. Single producer (the
// IRQ handler below), single consumer (the main loop's drain, after each
// USB echo), so the head/tail indices need no locking.
#define IORQ_EVENT_BUF_SIZE 32
static volatile uint8_t iorq_events[IORQ_EVENT_BUF_SIZE];
static volatile uint8_t iorq_head = 0;
static volatile uint8_t iorq_tail = 0;

static void iorq_irq_handler(uint gpio, uint32_t events) {
    uint8_t next_head = (iorq_head + 1) % IORQ_EVENT_BUF_SIZE;
    if (next_head != iorq_tail) {
        iorq_events[iorq_head] = read_data_bus();
        iorq_head = next_head;
    }
    // else: buffer full - drop the event rather than block the ISR
}

int main() {
    stdio_init_all();

    for (int i = 0; i < 8; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
    }

    gpio_init(IORQ_PIN);
    gpio_set_dir(IORQ_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(IORQ_PIN, GPIO_IRQ_EDGE_FALL, true, &iorq_irq_handler);

    while (true) {
        int c = getchar();
        putchar(toupper(c));
        printf(" %02X ", read_data_bus());

        while (iorq_tail != iorq_head) {
            printf("%02X ", iorq_events[iorq_tail]);
            iorq_tail = (iorq_tail + 1) % IORQ_EVENT_BUF_SIZE;
        }
        putchar('\n');
    }
}
