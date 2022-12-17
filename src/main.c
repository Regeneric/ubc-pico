#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"

#include "commons.h"
#include "oled.h"

// commons.h
// typedef uint8_t  byte;
// typedef uint16_t word;
// typedef uint32_t dword;
// typedef uint64_t qword;


int main() {
    stdio_init_all();
    initOLED();
    
    for(byte i = 0; i < 128; i++) setPixel(i, 1, 0xFF);
    setPixel(50, 1, 0x00);

    while(1) {

    } return 0;
}
