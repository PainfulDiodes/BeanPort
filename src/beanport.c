#include "pico/stdlib.h"
#include <ctype.h>
#include <stdio.h>

// GP0-GP7 = D0-D7 from the level shifter's B-side (design.md §10)
static uint8_t read_data_bus(void) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= gpio_get(i) << i;
    }
    return value;
}

int main() {
    stdio_init_all();

    for (int i = 0; i < 8; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
    }

    while (true) {
        int c = getchar();
        putchar(toupper(c));
        printf(" %02X ", read_data_bus());
    }
}
