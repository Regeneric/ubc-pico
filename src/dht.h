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


#ifndef DHT_H
#define DHT_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define USE_DHT 0

#if USE_DHT == 1
    #define MAX_TIMING 85

    #define DHT_PIN 10
    #define DHT_EN_OUT gpio_set_dir(DHT_PIN, GPIO_OUT)
    #define DHT_EN_INP gpio_set_dir(DHT_PIN, GPIO_IN)
    
    #define DHT_LO     gpio_put(DHT_PIN, 0)
    #define DHT_HI     gpio_put(DHT_PIN, 1)
    #define DHT_RD     gpio_get(DHT_PIN)

    typedef struct {
        float humidity;
        float temperature;
    } DHT;

    static inline void startTempRead() {
        DHT_EN_OUT;
        DHT_LO;
    } DHT tempRead() __attribute__((optimize("-O3")));
#endif  // USE_DHT
#endif  // DHT_H