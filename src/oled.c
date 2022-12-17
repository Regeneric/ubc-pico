#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h" 
#include "pico/binary_info.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#include "commons.h"
#include "oled.h"


// commons.h
// typedef uint8_t  byte;
// typedef uint16_t word;
// typedef uint32_t dword;
// typedef uint64_t qword;


#define swap(x, y) do {typeof(x) swap = x; x = y; y = swap;} while(0)
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#define max(a,b)   (((a) > (b)) ? (a) : (b))



#define BUFF_SIZE 4*WIDTH*((HEIGHT+7)/8)
byte buff[BUFF_SIZE] = {0};


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

#pragma GCC optimize ("03")
#pragma GCC push_options
static void writeI2C(byte addr, const byte *data, word len, byte mode) {
#ifdef i2c_default
    for(word c = 0; c < len; c++) {
        const byte snd[2] = {mode, data[c]};
        i2c_write_blocking(i2c_default, addr, snd, 2, false);
    } return;
#endif  // i2c_default
}
#endif  // USE_I2C

#if USE_SPI == 1
void initSPI() {
    spi_init(SPI_PORT, SPI_SPEED * 1000000);
    spi_set_format(SPI_PORT, 8, 0, 0, SPI_MSB_FIRST);
    gpio_set_function(DIN, GPIO_FUNC_SPI);
    gpio_set_function(CLK, GPIO_FUNC_SPI);

    // Make the SPI pins available to picotool
    bi_decl(bi_2pins_with_func(DIN, CLK, GPIO_FUNC_SPI));

    RST_INIT; RST_OUT; RST_HI;
    DC_INIT ; DC_OUT ; DC_HI;
    CS_INIT ; CS_OUT ; CS_HI;
}

void SSD_RST() {
    sleep_ms(5);

    RST_HI;
    sleep_us(15);
    RST_LO;
}


void writeSPI(byte data) {spi_write_blocking(SPI_PORT, &data, 1);}
#endif  // USE_SPI

__attribute__((always_inline)) static inline void sendCMD(const byte *cmd, byte len) {
    #if USE_I2C == 1
    writeI2C(ADDRESS, cmd, len, REG_CMD);
    #endif  // USE_I2C

    #if USE_SPI == 1
    CS_LO;
    DC_LO;

    writeSPI(cmd);
    
    CS_HI;
    DC_HI;
    #endif  // USE_SPI
}

__attribute__((always_inline)) static inline void sendData(byte *data, word len) {
    #if USE_I2C == 1
    writeI2C(ADDRESS, data, len, REG_DATA);
    #endif  // USE_I2C

    #if USE_SPI == 1
    writeSPI(data);
    #endif  // USE_SPI
}
#pragma GCC pop_options

#if USE_SPI == 1
void sendCMD_and_Data(byte cmd, byte data) {
    CS_LO;
    DC_LO;

    writeSPI(cmd);
    writeSPI(data);

    CS_HI;
    DC_HI;
}
#endif  // USE_SPI


static void init() {
    // static const byte init[] = {0xAE, 0x81, 0x80, 0xA0, 0x51, 0xA1, 0x00, 0xA2,
    //                             0x00, 0xA6, 0xA8, 0x7F, 0xB1, 0x11, 0xB3, 0x00,
    //                             0xAB, 0x01, 0xB6, 0x04, 0xBE, 0x0F, 0xBC, 0x08,
    //                             0xD5, 0x62, 0xFD, 0x12, 0xA4, 0xAF};

    #if USE_I2C == 1
    const static byte init[] = {SET_COMMAND_LOCK     ,  0x12,
                        //  (SET_DISP | OLED_OFF) , 
                        //   SET_DISP_START_LINE  ,  0x00,
                          SET_DISP_OFFSET      ,  0x00,
                          SET_SEG_REMAP        ,  0x30,  // 0x51 / 0xA0
                        //   SET_MUX_RATIO        ,  0x7F,  // 128x128 display
                          SET_FN_SELECT_A      ,  0x01,
                          SET_PHASE_LEN        ,  0x11,  // 0x51 / 0x11
                          SET_DISP_CLK_DIV     ,  0x00,  // 0x01 / 0x00 
                          SET_PRECHARGE        ,  0x08,
                          SET_VCOM_DESEL       ,  0x0F,  // 0x07 / 0x0F
                          SET_SECOND_PRECHARGE ,  0x04,  // 0x01 / 0x04
                          SET_FN_SELECT_B      ,  0x62,
                          SET_GRAYSCALE_LINEAR ,
                          SET_CONTRAST         ,  0x7F,  // 0-255
                        //   SET_DISP_MODE,
                        //   SET_ROW_ADDR,  0x00  ,  0x7F,  // 128 width
                        //   SET_COL_ADDR,  0x00  ,  0x7F,  // 128 height
                          SET_SCROLL_DEACTIVATE
                          };

    word len = ARRL(init);
    sendCMD(init, len);
    #endif

    #if USE_SPI == 1
    sendCMD_and_Data(SET_COMMAND_LOCK    , 0x12);
    sendCMD(SET_DISP);
    sendCMD_and_Data(SET_DISP_START_LINE , 0x00);
    sendCMD_and_Data(SET_DISP_OFFSET     , 0x00);
    sendCMD_and_Data(SET_SEG_REMAP       , 0x51);
    sendCMD_and_Data(SET_MUX_RATIO       , 0x7F);
    sendCMD_and_Data(SET_FN_SELECT_A     , 0x01);
    sendCMD_and_Data(SET_PHASE_LEN       , 0x11);
    sendCMD_and_Data(SET_DISP_CLK_DIV    , 0x01);
    sendCMD_and_Data(SET_PRECHARGE       , 0x08);
    sendCMD_and_Data(SET_VCOM_DESEL      , 0x0F);
    sendCMD_and_Data(SET_SECOND_PRECHARGE, 0x04);
    sendCMD_and_Data(SET_FN_SELECT_B     , 0x62);
    sendCMD(SET_GRAYSCALE_LINEAR);
    sendCMD_and_Data(SET_CONTRAST        , 0x7F);
    sendCMD(SET_DISP_MODE);
    #endif

    sleep_ms(100);

    const static byte postInit = (SET_DISP | OLED_ON);
    sendCMD(&postInit, 1);

    contrast(0x2F);
}

#pragma GCC optimize("03")
#pragma GCC push_options
void clear(byte color) {
    memset(buff, color , BUFF_SIZE);
    word len = ARRL(buff);
    sendData(buff, len);

    display();
}
#pragma GCC pop_options

void powerOn() {
    #if USE_I2C == 1
    const static byte powerOn[] = {SET_FN_SELECT_A, VDD_ON};
    word len = ARRL(powerOn);
    sendCMD(powerOn, len);
    #endif  // USE_I2C

    #if USE_SPI == 1
    sendCMD_and_Data(SET_FN_SELECT_A, VDD_ON);
    printf("POWER ON SUCCESS\r\n");
    #endif  // USE_SPI
}

void powerOff() {
    #if USE_I2C == 1
    const static byte powerOff[] = {SET_FN_SELECT_A, VDD_OFF, (SET_DISP | OLED_OFF)};
    word len = ARRL(powerOff);
    sendCMD(powerOff, len);
    #endif  // USE_I2C

    #if USE_SPI == 1
    word len = sizeof(powerOff);
    #endif  // USE_SPI
}

void displayOn() {
    const static byte postInit = (SET_DISP | OLED_ON);
    sendCMD(&postInit, 1);
}

void displayOff() {
    const static byte postInit = (SET_DISP | OLED_OFF);
    sendCMD(&postInit, 1);
}

void contrast(byte contrast) {
    #if USE_I2C == 1
    const byte _contrast[] = {SET_CONTRAST, contrast};
    word len = ARRL(_contrast);
    sendCMD(_contrast, len);
    #endif

    #if USE_SPI == 1
    word len = sizeof(_contrast);
    #endif
}

void invert(byte invert) {
    #if USE_I2C == 1
    const byte _invert[] = {SET_DISP_MODE | (invert & 1) << 1 | (invert & 1)};  // 0xA4 - normal ; 0xA7 - inverted
    word len = ARRL(_invert);
    sendCMD(_invert, len);
    #endif

    #if USE_SPI == 1
    word len = sizeof(_invert);
    #endif
}

void refresh(byte refresh) {
    #if USE_I2C == 1
    const byte _refresh[] = {SET_DISP_CLK_DIV | refresh};
    word len = ARRL(_refresh);
    sendCMD(_refresh, len);
    #endif

    #if USE_SPI == 1
    word len = sizeof(_refresh);
    #endif
} 


void initOLED() {
    #if USE_I2C == 1
        initI2C();
        sleep_ms(50);
    #endif

    #if USE_SPI == 1
        initSPI();
        SSD_RST();
    #endif
    
    powerOn();
    sleep_ms(50);
    
    init();
    clear(0x00);
}


#pragma GCC optimize("03")
#pragma GCC push_options
void display() {
    const static byte display[] = {SET_COL_ADDR, 0x00, 0x3F, SET_ROW_ADDR, 0x00, 0x7F};

    #if USE_I2C == 1
    word len = ARRL(display);
    word maxBuff = 512;
    #endif

    #if USE_SPI == 1
    word len = sizeof(display);
    #endif

    sendCMD(display, len);


    #if USE_I2C == 1
    byte windowX1 = 0;
    byte windowY1 = 0;
    byte windowX2 = WIDTH-1;
    byte windowY2 = HEIGHT-1;

    byte *localBuff = buff;
    byte rows = HEIGHT;

    byte bytesPerRow = WIDTH/2;

    int16_t rowStart = min((int16_t)(bytesPerRow-1), (int16_t)(windowX1/2));
    int16_t rowEnd   = max((int16_t)0, (int16_t)(windowX2/2));

    int16_t firstRow = min((int16_t)(rows-1), (int16_t)windowY1);
    int16_t lastRow  = max((int16_t)0, (int16_t)windowY2);

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
    #endif  // USE_I2C

    #if USE_I2C == 1
    // len = ARRL(buff);
    // sendData(buff, len);
    #endif
}

void setPixel(byte x, byte y, byte color) {
    if((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT)) return;

    // if(color > 0) buff[(x/2 + (y/8)*WIDTH)] |= (color<<(y%15));
    // else buff[(x/2 + (y/8)*WIDTH)] &= ~(color<<(y%15));

    byte *localBuff = &buff[x/2 + (y*WIDTH/2)];
    if(x%2 == 0) {
        byte t = (localBuff[0] & 0x0F);
        t |= (color & 0x0F) << 4;
        localBuff[0] = t;
    } else {
        byte t = (localBuff[0] & 0xF0);
        t |= (color & 0x0F);
        localBuff[0] = t;
    } 
}

void drawLine(word x0, word y0, word x1, word y1, byte color) {
    int16_t steep = abs(y1 - y0) > (x1 - x0);
    if(steep) {
        swap(x0, y0);
        swap(x1, y1);
    }

    int16_t dx = 0, dy = 0;
        dx = x1 - x0;
        dy = abs(y1 - y0);

    int16_t err = dx/2;
    int16_t ysteep;

    if(y0 < y1) ysteep =  1;
    else        ysteep = -1;

    for(; x0 <= x1; x0++) {
        if(steep) setPixel(y0/2, x0, color);
        else      setPixel(x0, y0/2, color);

        err -= dy;
        if(err < 0) {
            y0 += ysteep;
            err += dx;
        }
    }
}

__attribute__((always_inline)) inline static void drawFastRawHLine(word x, word y, word w, byte color) {memset(buff + ((y * WIDTH + x)), color, w/2);}
void drawFastHLine(word x, word y, word w, byte color) {
    if(w < 0) {
        w *= -1;
        x -= w-1;

        if(x < 0) {
            w += x;
            x = 0;
        }
    }

    if((y < 0) || (y >= HEIGHT) || (x >= WIDTH) || ((x+w-1) < 0)) return;
    if(x < 0) {
        w += x;
        x = 0;
    }

    if(x + w >= WIDTH) w = WIDTH-x;
    drawFastRawHLine(x, y/4, w, color);
}
#pragma GCC pop_options