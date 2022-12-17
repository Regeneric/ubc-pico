#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h" 
#include "pico/binary_info.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "commons.h"
#include "oled.h"


// commons.h
// typedef uint8_t  byte;
// typedef uint16_t word;
// typedef uint32_t dword;
// typedef uint64_t qword;


#define swap(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#define max(a,b)   (((a) > (b)) ? (a) : (b))


byte bitsPerPixel = 4;
byte buff[4*(WIDTH*HEIGHT)/8] = {0};


#if USE_I2C == 1
static void initI2C() {
    i2c_init(i2c_default, I2C_SPEED * 1000);

    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);     // GPIO 4
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);     // GPIO 5
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    printf("I2C INIT COMPLETE\r\n");
}

static void writeI2C(byte addr, const byte *data, word len, byte mode) {
#ifdef i2c_default
    for(int c = 0; c < len; c++) {
        char snd[2] = {mode, data[c]};
        i2c_write_blocking(i2c_default, addr, snd, sizeof(snd), false);
    } return;
#endif  // i2c_default
}
#endif  // USE_I2C


#if USE_SPI == 1
void initSPI() {
    spi_init(SPI_PORT, SPI_SPEED * 1000000);
    gpio_set_function(DIN, GPIO_FUNC_SPI);
    gpio_set_function(CLK, GPIO_FUNC_SPI);

    // Make the SPI pins available to picotool
    bi_decl(bi_2pins_with_func(DIN, CLK, GPIO_FUNC_SPI));

    gpio_init(DIN);
    gpio_set_dir(DIN, GPIO_OUT);
    gpio_put(DIN, 0);
    
    gpio_init(RST);
    gpio_set_dir(RST, GPIO_OUT);
    gpio_put(RST, 1);

    gpio_init(DC);
    gpio_set_dir(DC, GPIO_OUT);
    gpio_put(DC, 1);

    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_put(CS, 1);
}


__attribute__((always_inline)) static inline void SSD_DC(byte state) {
    if(state) gpio_put(DC, 1);
    else      gpio_put(DC, 0);
}

__attribute__((always_inline)) static inline void SSD_CS(byte state) {
    if(state) gpio_put(CS, 1);
    else      gpio_put(CS, 0);
}

__attribute__((always_inline)) static inline void SSD_DIN(byte state) {
    if(state) gpio_put(DIN, 1);
    else      gpio_put(DIN, 0);
}

static inline void SSD_RST() {
    sleep_ms(5);
    gpio_put(RST, 1);
    sleep_ms(25);
    gpio_put(RST, 0);
    sleep_ms(25);

    printf("RESET COMPLETE\r\n");
}


__attribute__((always_inline)) static inline void writeSPI(byte *data, byte size) {
    spi_write_blocking(SPI_PORT, data, size);
}
#endif  // USE_SPI


__attribute__((always_inline)) static inline void sendCMD(const byte *cmd, word len) {
    #if USE_I2C == 1
    writeI2C(ADDRESS, cmd, len, REG_CMD);
    #endif  // USE_I2C

    #if USE_SPI == 1
    SSD_CS(0);
    SSD_DC(0);
    writeSPI(cmd, len);
    SSD_DC(1);
    SSD_CS(1);
    #endif  // USE_SPI
}

__attribute__((always_inline)) static inline void sendData(const byte *data, word len) {
    #if USE_I2C == 1
    writeI2C(ADDRESS, data, len, REG_DATA);
    #endif  // USE_I2C

    #if USE_SPI == 1
    SSD_CS(0);
    SSD_DC(1);
    writeSPI(data, len);
    SSD_DC(0);
    SSD_CS(1);
    #endif  // USE_SPI
}


static void init() {
    // static const byte init[] = {0xAE, 0x81, 0x80, 0xA0, 0x51, 0xA1, 0x00, 0xA2,
    //                             0x00, 0xA6, 0xA8, 0x7F, 0xB1, 0x11, 0xB3, 0x00,
    //                             0xAB, 0x01, 0xB6, 0x04, 0xBE, 0x0F, 0xBC, 0x08,
    //                             0xD5, 0x62, 0xFD, 0x12, 0xA4, 0xAF};

    static const byte init[] = {SET_COMMAND_LOCK     ,  0x12,
                               (SET_DISP | OLED_OFF) , 
                                SET_DISP_START_LINE  ,  0x00,
                                SET_DISP_OFFSET      ,  0x20,
                                SET_SEG_REMAP        ,  0x51,
                                SET_MUX_RATIO        ,  0x7F,  // 128x128 display
                                SET_FN_SELECT_A      ,  0x01,
                                SET_PHASE_LEN        ,  0x51,
                                SET_DISP_CLK_DIV     ,  0x01,
                                SET_PRECHARGE        ,  0x08,
                                SET_VCOM_DESEL       ,  0x07,
                                SET_SECOND_PRECHARGE ,  0x01,
                                SET_FN_SELECT_B      ,  0x62,
                                SET_GRAYSCALE_LINEAR ,
                                SET_CONTRAST         ,  0x7F,  // 0-255
                                SET_DISP_MODE,
                                SET_ROW_ADDR,  0x00  ,  0x7F,  // 128 width
                                SET_COL_ADDR,  0x00  ,  0x3F,  // 128 height
                                SET_SCROLL_DEACTIVATE};
    word len = ARRL(init);
    sendCMD(init, len);

    sleep_ms(100);
    static const byte postInit = (SET_DISP | OLED_ON);
    sendCMD(&postInit, 1);
    contrast(0x2F);
}

void clear(byte color) {
    static const byte display[] = {SET_COL_ADDR, 0x00, 0x3F, SET_ROW_ADDR, 0x00, 0x7F};

    #if USE_I2C == 1
    word len = ARRL(display);
    #endif

    #if USE_SPI == 1
    word len = sizeof(display);
    #endif

    sendCMD(display, len);

    memset(buff, color , (4*(WIDTH*HEIGHT)/8));
    len = ARRL(buff);
    sendData(buff, len);
}

static void display() {
    byte windowX1 = 0;
    byte windowY1 = 0;
    byte windowX2 = WIDTH-1;
    byte windowY2 = HEIGHT-1;

    byte *localBuff = buff;
    byte rows = HEIGHT;

    byte bytesPerRow = WIDTH/2;     // See fig 10-1 (64 bytes, 128 pixels)

    int16_t rowStart = min((int16_t)(bytesPerRow-1), (int16_t)(windowX1/2));
    int16_t rowEnd   = max((int16_t)0, (int16_t)(windowX2/2));

    int16_t firstRow = min((int16_t)(rows-1), (int16_t)windowY1);
    int16_t lastRow  = max((int16_t)0, (int16_t)windowY2);

    static const byte display[] = {SET_COL_ADDR, 0x00, 0x3F, SET_ROW_ADDR, 0x00, 0x7F};

    #if USE_I2C == 1
    word len = ARRL(display);
    byte maxBuff = 254;
    #endif

    #if USE_SPI == 1
    word len = sizeof(display);
    #endif

    sendCMD(display, len);

    for(byte row = firstRow; row <= lastRow; row++) {
        byte bytesRemaining = rowEnd - rowStart+1;
        localBuff = buff + (word)row * (word)bytesPerRow;
        localBuff += rowStart;

        while(bytesRemaining) {
            byte toWrite = min(bytesRemaining, maxBuff);
            sendData(localBuff, toWrite);
            localBuff += toWrite;
            bytesRemaining -= toWrite;
        }
    }

    // len = ARRL(buff);
    // sendData(buff, len);
}

void powerOn() {
    static const byte powerOn[] = {SET_FN_SELECT_A, VDD_ON, (SET_DISP | OLED_ON)};

    #if USE_I2C == 1
    word len = ARRL(powerOn);
    #endif

    #if USE_SPI == 1
    word len = sizeof(powerOn);
    #endif

    sendCMD(powerOn, len);
}

void powerOff() {
    static const byte powerOff[] = {SET_FN_SELECT_A, VDD_OFF, (SET_DISP | OLED_OFF)};

    #if USE_I2C == 1
    word len = ARRL(powerOff);
    #endif

    #if USE_SPI == 1
    word len = sizeof(powerOff);
    #endif

    sendCMD(powerOff, len);
}

void contrast(byte contrast) {
    byte _contrast[] = {SET_CONTRAST, contrast};

    #if USE_I2C == 1
    word len = ARRL(_contrast);
    #endif

    #if USE_SPI == 1
    word len = sizeof(_contrast);
    #endif

    sendCMD(_contrast, len);
}

void invert(byte invert) {
    byte _invert[] = {SET_DISP_MODE | (invert & 1) << 1 | (invert & 1)};  // 0xA4 - normal ; 0xA7 - inverted
    
    #if USE_I2C == 1
    word len = ARRL(_invert);
    #endif

    #if USE_SPI == 1
    word len = sizeof(_invert);
    #endif

    sendCMD(_invert, len);
}

void refresh(byte refresh) {
    byte _refresh[] = {SET_DISP_CLK_DIV | refresh};

    #if USE_I2C == 1
    word len = ARRL(_refresh);
    #endif

    #if USE_SPI == 1
    word len = sizeof(_refresh);
    #endif

    sendCMD(_refresh, len);
} 


void initOLED() {
    #if USE_I2C == 1
        initI2C();
        sleep_ms(25);
    #endif

    #if USE_SPI == 1
        initSPI();
        sleep_ms(25);
        SSD_RST();
    #endif
    
    powerOn();
    init();

    invert(1);
    invert(0);

    clear(0x00);
}


void setPixel(byte x, byte y, byte color) {
    if((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT)) return;

    // if(color > 0) buff[(x/2 + (y/8)*WIDTH)] |= (color<<(y%15));
    // else buff[(x/2 + (y/8)*WIDTH)] &= ~(color<<(y%15));

    byte *localBuff = &buff[x/2 + (y*WIDTH/2)];
    if(x%2 == 0) {
        byte t = localBuff[0] & 0x0F;
        t |= (color & 0xF) << 4;
        localBuff[0] = t;
    } else {
        byte t = (localBuff[0] & 0xF0);
        t |= (color & 0xF);
        localBuff[0] = t;
    }

    display();
}