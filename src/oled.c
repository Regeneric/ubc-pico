#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h" 
#include "pico/binary_info.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#include "commons.h"
#include "oled.h"
#include "fonts.h"
#include "ftoa.h"


// commons.h
// typedef uint8_t  byte;
// typedef uint16_t word;
// typedef uint32_t dword;
// typedef uint64_t qword;


#define swap(x, y) do {typeof(x) swap = x; x = y; y = swap;} while(0)
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#define max(a,b)   (((a) > (b)) ? (a) : (b))



#define BITS_PER_PIXEL  4
#define BUFF_SIZE (WIDTH*HEIGHT*BITS_PER_PIXEL)/8
static byte buff[BUFF_SIZE] = {0};


struct {
    int x;
    int y;

    byte size;
    byte color;
} text = {.x = 0, .y = 0, .size = 1, .color = 0xFF};

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
                                // (SET_DISP | OLED_OFF) , 
                                // SET_DISP_START_LINE  ,  0x00,
                                SET_DISP_OFFSET      ,  0x00,
                                SET_SEG_REMAP        ,  0x51,  // 0x51 / 0xA0 / 0x30
                                // SET_MUX_RATIO        ,  0x7F,  // 128x128 display
                                SET_FN_SELECT_A      ,  0x01,
                                SET_PHASE_LEN        ,  0x51,  // 0x51 / 0x11
                                SET_DISP_CLK_DIV     ,  0x00,  // 100 Hz
                                SET_PRECHARGE        ,  0x08,
                                SET_VCOM_DESEL       ,  0x0F,  // 0x07 / 0x0F
                                SET_SECOND_PRECHARGE ,  0x04,  // 0x01 / 0x04
                                SET_FN_SELECT_B      ,  0x62,
                                SET_GRAYSCALE_LINEAR ,
                                SET_CONTRAST         ,  0x7F,  // 0-255
                                SET_DISP_MODE,
                                SET_ROW_ADDR,  0x00  ,  0x7F,  // 128 width
                                SET_COL_ADDR,  0x00  ,  0x7F,  // 128 height
                                SET_SCROLL_DEACTIVATE};

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
    sendCMD(SET_GRAYsize_LINEAR);
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

void brightness(byte brightness) {
    #if USE_I2C == 1
    const byte _brightness[] = {SET_PHASE_LEN, brightness};
    word len = ARRL(_brightness);
    sendCMD(_brightness, len);
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

    // byte *pixel = &buff[addr];
    // if(color) *pixel |=  (1<<(dy%2));
    // else      *pixel &= ~(1<<(dy%2));

    byte dx = WIDTH  - 1 - x;
    byte dy = HEIGHT - 1 - y;
    word addr = (dx + dy * WIDTH-1)/2;

    buff[addr] = color;
}

void drawLine(word x0, word y0, word x1, word y1, byte color) {
    // Bresenham's line algorithm
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

    for(;x0 <= x1; x0++) {
        if(steep) setPixel(y0, x0, color);
        else      setPixel(x0, y0, color);

        err -= dy;
        if(err < 0) {
            y0  += ysteep;
            err += dx;
        }
    }
}

__attribute__((always_inline)) inline static void drawFastRawHLine(word x, word y, word w, byte color) {memset(buff + ((y * WIDTH + x)), color, w/2);}
__attribute__((always_inline)) inline void drawHLine(word x, word y, word w, byte color) {drawLine(x, y, w, y, color);}
void drawFastHLine(word x, word y, word w, byte color) {
    if(w < 0) {
        w *=  -1;
        x -= w-1;

        if(x < 0) {
            w += x;
            x = 0;
        }
    }

    if((y < 0) || (y >= HEIGHT) || (x >= WIDTH) || ((x+w-1) < 0)) return;
    if(x < 0) {
        w += x;
        x  = 0;
    }

    if(x + w >= WIDTH) w = WIDTH-x;
    drawFastRawHLine(x, y/2, w, color);
}

static void drawFastRawVLine(word x, word y, word h, byte color) {
    byte *localBuf = buff + ((y * WIDTH + x));
    for(word i = 0; i < h; i++) {
        (*localBuf) = color;
        localBuf += WIDTH/2;
    }
}

__attribute__((always_inline)) inline void drawVLine(word x, word y, word h, byte color) {drawLine(x, y, x, h, color);}
void drawFastVLine(word x, word y, word h, byte color) {
    if(h < 0) {
        h *= -1;
        y -= h-1;

        if(y < 0) {
            h += y;
            y = 0;
        }
    }

    if((x < 0) || (x >= WIDTH) || (y >= HEIGHT) || ((y+h-1) < 0)) return;
    if(y < 0) {
        h += y;
        y = 0;
    }
    
    if(y + h > HEIGHT) h = HEIGHT-y;
    drawFastRawVLine(x/2, y, h, color);
}

void fillRect(byte x, byte y, byte w, byte h, byte color) {
    for(word i = x; i < x+w; i++) drawFastVLine(i, y, h, color);
}


__attribute__((always_inline)) inline void charSize(byte size) {text.size = size;}
__attribute__((always_inline)) inline void charColor(byte color) {text.color = color;}
void putChar(byte x, byte y, char c, byte color, byte bg, byte size) {
    if((x >= WIDTH) || (y >= HEIGHT) || ((x+6*size-1) < 0) || ((y+8*size-1) < 0)) return;

    size  = (text.size != size)   ? size  : text.size;
    color = (text.color != color) ? color : text.color;

    register byte dx, dy;
	for(dx = 0; dx != 5*size; ++dx) {
		for(dy = 0; dy != 7*size; ++dy) {
			if((FONT5x7[c-32][dx/size]) & (1 << dy/size)) setPixel(x+dx, y+dy, color);
			else setPixel(x+dx, y+dy, 0x00);
        }
    }

	x += 5*size + 1;
	if(x >= 128) {
		x = 0;
		y += 7*size + 1;
	} 
    
    if(y >= 128) {
		x = 0;
		y = 0;
	}
}
__attribute__((always_inline)) inline void sendc(byte x, byte y, char c) {putChar(x, y, c, text.color, 0x00, text.size);} 

void sends(byte x, byte y, char *str) {
    text.x = x;
    text.y = y;

    while(*str) {
        sendc(text.x, text.y, *str++);
        text.x += (text.size == 1) ? text.size * 8 : text.size * 7;

        if(text.x >= WIDTH-1) text.y = text.size * 7;
    }
}

void sendi(byte x, byte y, int num) {
    char buff[16];
    sends(x, y, itoa(num, buff, 10));
}

void sendf(byte x, byte y, float num, byte precision) {
    char buff[32];
    ftoa(num, buff, precision);
    sends(x, y, buff);
}
#pragma GCC pop_options


const struct OLED oled = {
    .powerOn = powerOn,
    .powerOff = powerOff,
    
    .on = displayOn,
    .off = displayOff,

    .init = initOLED,
    .display = display,
    .clear = clear,
    .invert = invert, 
    .refresh = refresh,
    .contrast = contrast,
    .brightness = brightness,

    .pixel = setPixel,
    .rect = fillRect,
    .line = drawLine,
    .hline = drawHLine,
    .fhline = drawFastHLine,
    .vline = drawVLine,
    .fvline = drawFastVLine,

    .font = charSize,
    .color = charColor,

    .sendc = sendc,
    .sends = sends,
    .sendi = sendi,
    .sendf = sendf
};



void screenTest() {
    initOLED();

    sendc(0, 0, 'A');
    sendc(120, 0, 'B');
    sendc(0, 120, 'C');
    sendc(120, 120, 'D');
    sendc(WIDTH/2, HEIGHT/2 , 'E');
    display();

    sleep_ms(2500);
    clear(0x00);
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

    drawFastHLine(0, 110, 127, 0xFF);
    charSize(1);
    sendc(0, 0, 'D');
    sendc(8, 0, 'U');
    sendc(16, 0, 'P');
    sendc(24, 0, 'A');

    charSize(2);
    sendc(60, 0, '2');
    sendc(70, 0, '1');
    sendc(80, 0, '3');
    sendc(90, 0, '7');


    putChar(10, 50, '7', 0xFF, 0x00, 1);
    putChar(20, 50, '7', 0xFF, 0x00, 2);
    putChar(35, 50, '7', 0xBF, 0x00, 3);
    putChar(55, 50, '7', 0x7F, 0x00, 4);
    putChar(80, 50, '7', 0x3F, 0x00, 5);
    
    charColor(0xFF);
    sends(10, 80, "TEST");

    charSize(1);
    sends(70, 87, "DUPA");

    sendi(10, 100, -1337);
    sendf(10, 110, 3.14, 2);
    display();

    sleep_ms(60000);
    powerOff();
}