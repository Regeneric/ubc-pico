#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"

#include "commons.h"
#include "oled.h"
#include "dht.h"

// commons.h
// typedef uint8_t  byte;
// typedef uint16_t word;
// typedef uint32_t dword;
// typedef uint64_t qword;



// DHT11 sensor support
#if USE_DHT == 1
DHT gDHT = {0};
#endif



static float gPulseDistance  = 0.0f;
static float gInjectionValue = 0.0f;

#ifndef INJ_PIN
#define INJ_PIN 21
#endif

typedef struct {
    word saveFlag;

    volatile byte pulseOverflows;
    volatile byte accelerationBuffer;

    volatile word avgSpeed;
    volatile word currSpeed;
    volatile word distRange;
    volatile word distPulseCount;

    volatile float sumInv;      // For harmonic mean calculations
    volatile float avgSpeedDivider;
    volatile float sailedDistance;
    volatile float traveledDistance;
    volatile float accelerationTime;
} VSS_DATA; VSS_DATA gVSS = {0};


#ifndef VSS_PIN
#define VSS_PIN 22
#endif

typedef struct {
    word saveFlag;

    volatile dword timeLow;
    volatile dword timeHigh;
    volatile dword pulseTime;   // Differnece between injector time in high and low state

    volatile float ccMin;
    volatile float sumInv;      // For harmonic mean calculations
    volatile float openTime;
    volatile float fuelLeft;
    volatile float fuelUsed;
    volatile float fuelSaved;
    volatile float insConsumption;
    volatile float avgConsumption;
    volatile float divFuelFactor;
} INJ_DATA; INJ_DATA gINJ = {.ccMin = 100};



#define SAVE_TIMEOUT 60
#ifdef  SAVE_TIMEOUT
    volatile static byte gSaveCounter = SAVE_TIMEOUT;
#else
    volatile static byte gSaveCounter = 60;
#endif

#define SAVE_FLAG_VAL     42069
#define FLASH_VSS_OFFSET (256 * 1024)                   // 256KB from the start of flash
#define FLASH_INJ_OFFSET ((256 + sizeof(gVSS)) * 1024)  // 256KB + sizeof(gVSS) from the start of flash

void saveData();
void loadData();



#ifndef SYS_TIMER_PERIOD_MS
#define SYS_TIMER_PERIOD_MS 1000
#endif

byte gTimerCounter = 4;

bool _timerISR(struct repeating_timer *t);
__force_inline static void initTimer() {
    struct repeating_timer timer;
    add_repeating_timer_ms(-250, _timerISR, NULL, &timer);
}



void avgSpeed()         __attribute__((optimize("-O3")));
void currSpeed()        __attribute__((optimize("-O3")));

void fuelConsumption()  __attribute__((optimize("-O3")));

void initIRQ(byte vssGPIO, byte injGPIO);
void _vssISR();
void _injISR(); 



int main() {
    // (2022.12.17) - Universal Board Computer (https://github.com/Regeneric/universal-board-computer) port for RPi Pico Board with added OLED and DHT11 support
    // (2022.12.18) - Screen is working, added fonts and drawing functions; Basic UBC functions implemented

    stdio_init_all();
    initTimer();
    // screenTest();

    #if defined(VSS_PIN) && defined(INJ_PIN)
        initIRQ(VSS_PIN, INJ_PIN);
    #else
        initIRQ(22, 21);
    #endif 


    // OLED init    
    oled.init();        // initOLED();
    oled.clear(0x00);   // clear(0x00);

    // 14 28 42 56 70 84 98 112 116

    while(1) {        
        // -- Range distance --
        oled.font(2);
        oled.sends(0, 0, "$&");

        if(gVSS.distRange >= 1000) {
            oled.sends(0, 0, "$&     ");
            oled.sendc(30, 0, '+'); 
            oled.sendi(40, 0, 999);
        }
        if(gVSS.distRange >= 100 && gVSS.distRange < 1000) {
            oled.sends(0, 0, "$&     ");
            oled.sendi(40, 0, gVSS.distRange);
        }
        if(gVSS.distRange > 50 && gVSS.distRange < 100) {
            oled.sends(0, 0, "$&     ");
            // oled.sends(40, 0, " ");
            oled.sendi(50, 0, gVSS.distRange);
            oled.sends(75, 0, " ");
        }
        if(gVSS.distRange <= 50) {
            oled.sendc(40, 0, '('); 
            oled.sendi(50, 0, gVSS.distRange); 
            // 64
            oled.sendc(75, 0, ')');
        } 
        if(gVSS.distRange < 10) {
            oled.sendi(50, 0, 0);
            oled.sendi(64, 0, gVSS.distRange);
        }

        oled.sends(101, 0, "KM");
        oled.fhline(0, HEIGHT-19, WIDTH-1, 0xFF);
        // -! Range distance !-


        // -- Instant fuel consumption --
        oled.font(4);

        if(gINJ.insConsumption > 0 && gINJ.insConsumption < 1)   {
            oled.sends(8, ((HEIGHT-1)/2)-28, "0");
            oled.sendf(36, ((HEIGHT-1)/2)-28, gINJ.insConsumption, 1);
            // oled.sendc(64, ((HEIGHT-1)/2)-28, ' ');
            oled.sends(92, ((HEIGHT-1)/2)-28, " ");
        } else if(gINJ.insConsumption > 1 && gINJ.insConsumption < 10) {
            oled.sendf(8, ((HEIGHT-1)/2)-28, gINJ.insConsumption, 1);
            oled.sends(92, ((HEIGHT-1)/2)-28, " ");
        } else if(gINJ.insConsumption > 99 || gINJ.insConsumption <= 0) oled.sends(8, ((HEIGHT-1)/2)-28, "--.-");
        else oled.sendf(8, ((HEIGHT-1)/2)-28, gINJ.insConsumption, 1);

        oled.font(2);
        if(gVSS.currSpeed > 5) oled.sends(0, ((HEIGHT-1)/2)+16, "    L/100");
        else oled.sends(0, ((HEIGHT-1)/2)+16, "      L/H");

        oled.fhline(0, 19, WIDTH-1, 0xFF);
        // -! Range distance !-


        // -- Average fuel consumption --
        oled.font(2);
        oled.sends(5, 113, "# ");
        
        if(gINJ.avgConsumption <= 0 || gINJ.avgConsumption > 100.0) {
            oled.sends(5, 113, "#   ");
            oled.sends(25, 113, "--.-");
        } else if(gINJ.avgConsumption > 0 && gINJ.avgConsumption < 1) {
            oled.sends(25, 113, " 0.");
            oled.sendf(54, 113, gINJ.avgConsumption, 1);
        } else if(gINJ.avgConsumption > 1 && gINJ.avgConsumption < 10){
            oled.sends(25, 113, " ");
            oled.sendf(40, 113, gINJ.avgConsumption, 1);
        } else oled.sendf(26, 113, gINJ.avgConsumption, 1);
        
        oled.font(1);
        oled.sends(90, 120, "L/100");
        // -! Average fuel consumption !-

        oled.display();
    } return 0;
}


void initIRQ(byte vssGPIO, byte injGPIO) {
    gpio_init(vssGPIO);
    gpio_init(injGPIO);

    gpio_set_dir(vssGPIO, GPIO_IN);
    gpio_set_dir(injGPIO, GPIO_IN);

    gpio_pull_up(vssGPIO);
    gpio_pull_up(injGPIO);

    gpio_set_irq_enabled_with_callback(vssGPIO, GPIO_IRQ_EDGE_FALL, 1, _vssISR);
    gpio_set_irq_enabled_with_callback(injGPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, 1, _injISR);

    return;
}

void _vssISR() {
    gVSS.distPulseCount++;
    gVSS.traveledDistance += gPulseDistance;
    
    if(gINJ.insConsumption <= 0) gVSS.sailedDistance += gPulseDistance;
}

void _injISR() {
    #ifdef INJ_PIN 
        if(gpio_get(INJ_PIN) == 0) gINJ.timeLow = to_ms_since_boot(get_absolute_time());    // Low state on INJ_PIN
        else {
            // Hight state on INJ_PIN
            gINJ.timeHigh  = to_ms_since_boot(get_absolute_time());
            gINJ.pulseTime = gINJ.pulseTime + (gINJ.timeHigh - gINJ.timeLow);
            gpio_put(INJ_PIN, 1);  
        }
    #else
        if(gpio_get(21) == 0) gINJ.timeLow = to_ms_since_boot(get_absolute_time());    // Low state on GPIO 21
        else {
            // Hight state on GPIO 21
            gINJ.timeHigh = to_ms_since_boot(get_absolute_time());
            gINJ.pulseTime = gINJ.pulseTime + (gINJ.timeHigh - gINJ.timeLow);
            gpio_put(21, 1);  
        }
    #endif
}



bool _timerISR(struct repeating_timer *t) {
    --gTimerCounter;

    // gINJ.insConsumption += 0.1;
    // gVSS.distRange++;
    // gINJ.avgConsumption += 0.1;

    // if(gINJ.insConsumption > 100.5) oled.powerOff();
    
    // // if(gVSS.distRange == 975) gVSS.distRange = 125;

    // if(gTimerCounter <= 0) {
    //     // gINJ.insConsumption += 0.1;
    //     gVSS.currSpeed++;
    //     // gINJ.avgConsumption += 0.1;
    //     // gVSS.distRange++;

    //     gTimerCounter = 4;
    // } 

    // Acceleration from 0 to 100 km/h measure time
    if(gVSS.currSpeed > 0 && gVSS.currSpeed < 100) gVSS.accelerationBuffer++;
    if(gVSS.currSpeed >= 100) gVSS.accelerationTime = (float)gVSS.accelerationBuffer/4.0f; 

    if(gTimerCounter == 3) {
        // Data saving based on speed and time - check one time every second
        if(gSaveCounter <= 0 && gINJ.pulseTime < 800 && gVSS.distPulseCount == 0) saveData();
        if(gSaveCounter <= 0 && gVSS.currSpeed == 0) saveData();
    }

    #if USE_DHT == 1
    // 25ms of delay between initializing the sensor and data read - WITHOUT `sleep_ms(25)`
        if(gTimerCounter == 2) startTempRead();
        if(gTimerCounter == 1) tempRead();
    #endif

    if(gTimerCounter <= 0) {
        currSpeed();
        fuelConsumption();

        if(gVSS.currSpeed <= 0) gVSS.accelerationBuffer = 0;
        if(gVSS.currSpeed > 5) {
            gVSS.distRange = (gINJ.fuelLeft/gINJ.avgConsumption)*100;
            gVSS.avgSpeedDivider++;
            avgSpeed();
        }

        if(gSaveCounter > 0) --gSaveCounter;

        gVSS.distPulseCount = 0;
        gINJ.pulseTime = 0;
        gTimerCounter = 4;
    } return true;
}


void avgSpeed() {
    // Harmonic mean
    gVSS.sumInv  += 1.0f/gVSS.currSpeed;
    gVSS.avgSpeed = gVSS.avgSpeedDivider/gVSS.sumInv;
} void currSpeed() {gVSS.currSpeed = gPulseDistance * gVSS.distPulseCount * 3600;}


void fuelConsumption() {
    const byte inj = 4;
    word it = 3600 * inj;

    float iotv = (gINJ.openTime*gInjectionValue)*it;
    float inv  = (gINJ.openTime*gInjectionValue)*inj;

    gINJ.openTime = ((float)gINJ.pulseTime/1000);
    if(gVSS.currSpeed > 5) {
        gINJ.insConsumption = (100*iotv)/gVSS.currSpeed;

        // Harmonic mean
        if(gINJ.insConsumption > 0.0f && gINJ.insConsumption < 100.0f) {
            gINJ.sumInv += 1.0f/gINJ.insConsumption;
            gINJ.avgConsumption = gVSS.avgSpeedDivider/gINJ.sumInv;
        }
    } else gINJ.insConsumption = iotv;

    gINJ.fuelUsed += iotv;
    gINJ.fuelLeft -= iotv;
}


// -- Pico SDK code --
__force_inline static uint32_t save_and_disable_interrupts(void) {
    uint32_t status;
    __asm volatile ("mrs %0, PRIMASK" : "=r" (status)::);
    __asm volatile ("cpsid i");
    return status;
}

__force_inline static void restore_interrupts(uint32_t status) {
    __asm volatile ("msr PRIMASK,%0"::"r" (status) : );
}
// -! Pico SDK code !-

void saveData() {
    byte *vssAsBytes = (byte*)&gVSS;
    word vssWriteSize = (sizeof(gVSS) / FLASH_PAGE_SIZE) + 1;
    word vssSectorCount = ((vssWriteSize * FLASH_PAGE_SIZE) / FLASH_SECTOR_SIZE) + 1;

    byte *injAsBytes = (byte*)&gINJ;
    word injWriteSize = (sizeof(gINJ) / FLASH_PAGE_SIZE) + 1;
    word injSectorCount = ((injWriteSize * FLASH_PAGE_SIZE) / FLASH_SECTOR_SIZE) + 1;

    dword interrupts = save_and_disable_interrupts();
        flash_range_erase(FLASH_VSS_OFFSET, FLASH_SECTOR_SIZE * vssSectorCount);
        flash_range_program(FLASH_VSS_OFFSET, vssAsBytes, FLASH_PAGE_SIZE * vssWriteSize);
    
        flash_range_erase(FLASH_INJ_OFFSET, FLASH_SECTOR_SIZE * injSectorCount);
        flash_range_program(FLASH_INJ_OFFSET, injAsBytes, FLASH_PAGE_SIZE * injWriteSize);
    restore_interrupts(interrupts);
}

void loadData() {
    VSS_DATA vssSaveCheck;
    INJ_DATA injSaveCheck;

    dword interrupts = save_and_disable_interrupts();
        const byte *vdFromFlash = (const byte*)(XIP_BASE + FLASH_VSS_OFFSET);
        memcpy(&vssSaveCheck, vdFromFlash + FLASH_PAGE_SIZE, sizeof(vssSaveCheck));
        // We're looking for known flag in flash memory to know, if it's rubbish data or legit struct
        if(vssSaveCheck.saveFlag == SAVE_FLAG_VAL) memcpy(&gVSS, vdFromFlash + FLASH_PAGE_SIZE, sizeof(gVSS));

        const byte *idFromFlash = (const byte*)(XIP_BASE + FLASH_INJ_OFFSET);
        memcpy(&injSaveCheck, vdFromFlash + FLASH_PAGE_SIZE, sizeof(injSaveCheck));
        if(injSaveCheck.saveFlag == SAVE_FLAG_VAL) memcpy(&gINJ, idFromFlash + FLASH_PAGE_SIZE, sizeof(gINJ));
    restore_interrupts(interrupts);
}