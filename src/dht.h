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