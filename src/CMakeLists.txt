add_executable(beanport beanport.c)

target_link_libraries(beanport pico_stdlib)
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(beanport pico_cyw43_arch_none)
endif()

pico_add_extra_outputs(beanport)
