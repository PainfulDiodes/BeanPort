add_executable(beanport beanport.c)

target_link_libraries(beanport pico_stdlib pico_multicore hardware_pio)

pico_enable_stdio_usb(beanport 1)
pico_enable_stdio_uart(beanport 0)

# CR/LF translation would corrupt any binary data containing 0x0A
target_compile_definitions(beanport PRIVATE PICO_STDIO_USB_DEFAULT_CRLF=0)

pico_generate_pio_header(beanport ${CMAKE_CURRENT_LIST_DIR}/beanport.pio)

# Wi-Fi-only additions run only on pico_w/pico2_w, which pico-sdk flags via PICO_CYW43_SUPPORTED
# Inline C changes:
#       #if PICO_CYW43_SUPPORTED
#           ...
#       #endif
# Self-contained additions here in CMake:
#     if (PICO_CYW43_SUPPORTED)
#         target_sources(beanport PRIVATE wifi_config.c)
#     endif()

pico_add_extra_outputs(beanport)
