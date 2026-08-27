add_executable(beanport beanport.c)

target_link_libraries(beanport pico_stdlib hardware_pio)

pico_enable_stdio_usb(beanport 1)
pico_enable_stdio_uart(beanport 0)

# CR/LF translation would corrupt any binary data containing 0x0A
target_compile_definitions(beanport PRIVATE PICO_STDIO_USB_DEFAULT_CRLF=0)

pico_generate_pio_header(beanport ${CMAKE_CURRENT_LIST_DIR}/beanport.pio)

pico_add_extra_outputs(beanport)
