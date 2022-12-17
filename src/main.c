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
    
    drawFastHLine(0, 8, 128, 0xFF);
    drawFastHLine(0, 16, 128, 0xFF);
    drawFastHLine(0, 24, 128, 0xFF);
    drawFastHLine(0, 32, 128, 0xFF);
    drawFastHLine(0, 40, 128, 0xFF);
    drawFastHLine(0, 48, 128, 0xFF);
    drawFastHLine(0, 56, 128, 0xFF);
    drawFastHLine(0, 64, 128, 0xFF);
    drawFastHLine(0, 72, 128, 0xFF);
    drawFastHLine(0, 80, 128, 0xFF);
    drawFastHLine(0, 88, 128, 0xFF);
    drawFastHLine(0, 96, 128, 0xFF);
    drawFastHLine(0, 104, 128, 0xFF);
    drawFastHLine(0, 112, 128, 0xFF);
    drawFastHLine(0, 120, 128, 0xFF);

    drawLine(8, 0, 8, 128, 0xFF);
    drawLine(24, 0, 24, 128, 0xFF);
    drawLine(40, 0, 40, 128, 0xFF);
    drawLine(56, 0, 56, 128, 0xFF);
    drawLine(72, 0, 72, 128, 0xFF);
    drawLine(88, 0, 88, 128, 0xFF);
    drawLine(104, 0, 104, 128, 0xFF);
    drawLine(120, 0, 120, 128, 0xFF);
    drawLine(136, 0, 136, 128, 0xFF);
    drawLine(152, 0, 152, 128, 0xFF);
    drawLine(168, 0, 168, 128, 0xFF);
    drawLine(184, 0, 184, 128, 0xFF);
    drawLine(200, 0, 200, 128, 0xFF);
    drawLine(216, 0, 216, 128, 0xFF);
    drawLine(232, 0, 232, 128, 0xFF);
    drawLine(248, 0, 248, 128, 0xFF);

    contrast(0x4F);
    display();

    sleep_ms(60000);
    powerOff();

    while(1) {

    } return 0;
}
