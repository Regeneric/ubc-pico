#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"

#include "commons.h"
#include "oled.h"
#include "dht.h"

// commons.h
// typedef uint8_t  byte;
// typedef uint16_t word;
// typedef uint32_t dword;
// typedef uint64_t qword;


static float gPulseDistance  = 0.0f;
static float gInjectionValue = 0.0f;

// DHT11 sensor support
#if USE_DHT == 1
DHT gDHT = {0};
#endif


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
    volatile static byte saveCounter = SAVE_TIMEOUT;
#else
    volatile static byte saveCounter = 60;
#endif

#define SAVE_FLAG_VAL     42069
#define FLASH_VSS_OFFSET (256 * 1024)                   // 256KB from the start of flash
#define FLASH_INJ_OFFSET ((256 + sizeof(gVSS)) * 1024)  // 256KB + sizeof(gVSS) from the start of flash



#ifndef SYS_TIMER_PERIOD_MS
#define SYS_TIMER_PERIOD_MS 1000
#endif

void initTimer();
void _timerISR();


void saveData();
void loadData();

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
    // screenTest();

    #if defined(VSS_PIN) && defined(INJ_PIN)
        initIRQ(VSS_PIN, INJ_PIN);
    #else
        initIRQ(22, 21);
    #endif 
    

    while(1) {

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
            gINJ.timeHigh = to_ms_since_boot(get_absolute_time());
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


void avgSpeed() {
    // Harmonic mean
    gVSS.sumInv += 1.0f/gVSS.currSpeed;
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