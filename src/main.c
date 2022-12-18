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

    drawFastHLine(0, 127, 127, 0xFF);
    drawFastVLine(0, 0, 127, 0xFF);
    drawLine(0, 0, 0, 127, 0xFF);
    drawLine(0, 127, 127, 127, 0xFF);
    drawLine(0, 0, 127, 127, 0xFF);
    drawLine(127, 0, 0, 127, 0xFF);
    display();

    sleep_ms(2500);
    clear(0x00);

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
    drawLine(16, 0, 16, 128, 0xFF);
    drawLine(24, 0, 24, 128, 0xFF);
    drawLine(32, 0, 32, 128, 0xFF);
    drawLine(40, 0, 40, 128, 0xFF);
    drawLine(48, 0, 48, 128, 0xFF);
    drawLine(56, 0, 56, 128, 0xFF);
    drawLine(64, 0, 64, 128, 0xFF);
    drawLine(72, 0, 72, 128, 0xFF);
    drawLine(80, 0, 80, 128, 0xFF);
    drawLine(88, 0, 88, 128, 0xFF);
    drawLine(96, 0, 96, 128, 0xFF);
    drawLine(104, 0, 104, 128, 0xFF);
    drawLine(112, 0, 112, 128, 0xFF);
    drawLine(120, 0, 120, 128, 0xFF);
    
    contrast(0x01);
    display();

    sleep_ms(2500);
    displayOff();
    
    sleep_ms(2500);
    displayOn();

    sleep_ms(2500);
    brightness(0x11);

    sleep_ms(2500);
    brightness(0x51);

    sleep_ms(2500);
    invert(1);
    
    sleep_ms(2500);
    invert(0);

    sleep_ms(2500);
    contrast(0xFF);

    sleep_ms(2500);
    contrast(0x01);

    sleep_ms(2500);
    clear(0x00);

    drawFastHLine(0, 115, 127, 0xFF);
    sendc(0 , 0, 'K');
    sendc(10, 0, 'U');
    sendc(20, 0, 'T');
    sendc(30, 0, 'A');
    sendc(40, 0, 'S');

    sendc(60, 0, '2');
    sendc(70, 0, '1');
    sendc(80, 0, '3');
    sendc(90, 0, '7');


    putChar(10, 50, '7', 0xFF, 0x00, 1);
    putChar(20, 50, '7', 0xFF, 0x00, 2);
    putChar(35, 50, '7', 0xBF, 0x00, 3);
    putChar(55, 50, '7', 0x7F, 0x00, 4);
    putChar(80, 50, '7', 0x3F, 0x00, 5);
    display();

    sleep_ms(60000);
    powerOff();

    while(1) {

    } return 0;
}
