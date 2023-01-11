set(PROJECT ubc-pico)

add_executable(${PROJECT}
    main.c
    oled.c
    ftoa.c
    dht.c
    eeprom.c
)

target_link_libraries(${PROJECT} 
    pico_stdlib 
    pico_multicore
    pico_binary_info

    hardware_gpio
    hardware_irq
    hardware_spi
    hardware_i2c
    hardware_flash
    hardware_timer
    hardware_sync
)

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

pico_add_extra_outputs(${PROJECT})

# picotool for automated uC flashing
add_custom_command(TARGET ${PROJECT}
    POST_BUILD
    COMMAND cd ${CMAKE_CURRENT_BINARY_DIR} && picotool load -f -x ${PROJECT}.uf2
)