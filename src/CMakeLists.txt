add_executable(beanport beanport.c)

# pull in common dependencies
target_link_libraries(beanport pico_stdlib pico_multicore hardware_pio)

# enable usb output (TinyUSB CDC), disable uart output
pico_enable_stdio_usb(beanport 1)
pico_enable_stdio_uart(beanport 0)

# CR/LF translation would corrupt any binary data containing 0x0A
target_compile_definitions(beanport PRIVATE PICO_STDIO_USB_DEFAULT_CRLF=0)

# TinyUSB CDC buffer sizing - stock is 64/64/64 on RP2040 (Full Speed)
target_compile_definitions(beanport PRIVATE
    CFG_TUD_CDC_EP_BUFSIZE=256
    CFG_TUD_CDC_RX_BUFSIZE=1024
    CFG_TUD_CDC_TX_BUFSIZE=256
)

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

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(beanport)
