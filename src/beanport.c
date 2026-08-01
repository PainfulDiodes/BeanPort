#include "pico/stdlib.h"
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

int main() {
#if defined(PICO_DEFAULT_LED_PIN)
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    if (cyw43_arch_init()) {
        return -1;
    }
#endif

    while (true) {
#if defined(PICO_DEFAULT_LED_PIN)
        gpio_put(LED_PIN, true);
#elif defined(CYW43_WL_GPIO_LED_PIN)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
#endif
        sleep_ms(250);
#if defined(PICO_DEFAULT_LED_PIN)
        gpio_put(LED_PIN, false);
#elif defined(CYW43_WL_GPIO_LED_PIN)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
#endif
        sleep_ms(250);
    }
}
