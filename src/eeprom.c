//  ​Universal Board Computer for cars with electronic MPI
//  Copyright © 2021-2023 IT Crowd, Hubert "hkk" Batkiewicz
// 
//  This file is part of UBC.
//  UBC is free software: you can redistribute it and/or modify
//  ​it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  ​License, or (at your option) any later version.
// 
//  ​This program is distributed in the hope that it will be useful,
//  ​but WITHOUT ANY WARRANTY; without even the implied warranty of
//  ​MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//  See the ​GNU Affero General Public License for more details.
// 
//  ​You should have received a copy of the GNU Affero General Public License
//  ​along with this program.  If not, see <https://www.gnu.org/licenses/>

// <https://itcrowd.net.pl/>


#include <stdio.h>
#include <stdlib.h>

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "commons.h"
#include "eeprom.h"


#define WRITE_ADDRESS 0xA0
#define READ_ADDRESS  0xA1

#define SEG_SIZE      128

#define EEPROM_I2C_SPEED    100     // KHz
#define EEPROM_I2C          i2c1
#define EEPROM_SDA          2
#define EEPROM_SCL          3


static void initI2C() {
    i2c_init(EEPROM_I2C, EEPROM_I2C_SPEED * 1000);

    gpio_set_function(EEPROM_SDA, GPIO_FUNC_I2C);    // GPIO 2 - SDA
    gpio_set_function(EEPROM_SCL, GPIO_FUNC_I2C);    // GPIO 3 - SDL
    gpio_pull_up(EEPROM_SDA);
    gpio_pull_up(EEPROM_SCL);

    printf("EEPROM I2C INIT COMPLETE\r\n");
}

#pragma GCC optimize ("03")
#pragma GCC push_options
static void writeI2C(byte addr, const byte *data, word len, byte segment) {
    for(word c = 0; c < len; c++) {
        const byte snd[2] = {segment, data[c]};
        i2c_write_blocking(i2c1, addr, snd, 2, false);
    } return;
}

byte *readI2C(byte addr, byte segment) {
    byte *buff;
    buff = malloc(sizeof(byte)*SEG_SIZE);    // Each EEPROM segment is 128 bytes
    
    i2c_write_blocking(i2c1, addr, &segment, 1, true);
    i2c_read_blocking(i2c1, addr, buff, SEG_SIZE, false);

    return buff;
}
#pragma GCC pop_options

__attribute__((always_inline)) static inline byte* readData(byte segment) {
    return readI2C(READ_ADDRESS, segment);
}

__attribute__((always_inline)) static inline void writeData(byte *data, word len, byte segment) {
    writeI2C(WRITE_ADDRESS, data, len, segment);
}



const struct EEPROM eeprom = {
    .init = initI2C,

    .write = writeData,
    .read = readData,
};