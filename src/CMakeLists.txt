add_executable(blink blink.c)

target_link_libraries(blink pico_stdlib)
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(blink pico_cyw43_arch_none)
endif()

pico_add_extra_outputs(blink)
