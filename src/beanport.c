#include "pico/stdlib.h"
#include <ctype.h>
#include <stdio.h>

int main() {
    stdio_init_all();

    while (true) {
        int c = getchar();
        putchar(toupper(c));
    }
}
