add_executable(beanport beanport.c)

target_link_libraries(beanport pico_stdlib)

pico_enable_stdio_usb(beanport 1)
pico_enable_stdio_uart(beanport 0)

# Wi-Fi-only additions (e.g. a future Wi-Fi config port) go behind
# "if (PICO_CYW43_SUPPORTED)", which pico-sdk sets true for pico_w/pico2_w
# (see pico-sdk/src/boards/include/boards/pico_w.h).

pico_add_extra_outputs(beanport)
