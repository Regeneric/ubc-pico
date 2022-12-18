#include "dht.h"

#if USE_DHT == 1
DHT tempRead() {
    DHT dht;

    int data[5] = {0, 0, 0, 0, 0};
    uint16_t last = 1, j = 0;

    DHT_EN_INP;
    for(int i = 0; i < MAX_TIMING; i++) {
        uint16_t count = 0;
        while(DHT_RD == last) {
            count++;
            sleep_us(1);
            if(count == 255) break;
        }

        last = DHT_RD;
        if(count == 255) break;

        if((i >= 4) && (i % 2 == 0)) {
            data[j/8] <<= 1;
            if(count > 16) data[j/8] |= 1;
            j++;
        }

        if((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
            dht.humidity = (float)((data[0]<<8) + data[1]) / 10;
            if(dht.humidity > 100) dht.humidity = data[0];

            dht.temperature = (float)(((data[2] & 0x7F)<<8) + data[3]) / 10;
            if(dht.temperature > 125) dht.temperature = data[2];
            if(data[2] & 0x80) dht.temperature = -dht.temperature;
        }
    } return dht;
}
#endif  // USE_DHT