#ifndef OLED_H
#define OLED_H


#define USE_SPI     0       // 0 - SPI off ; 1 - SPI on
#define SPI_SPEED   4       // MHz

#define USE_I2C     1       // 0 - I2C off ; 1 - SPI on
#define I2C_SPEED   3100    // kHz  - maximum my display can handle (standard speeds are 100 and 400)

#if USE_SPI == 1
    #define SPI_PORT spi0
    #define RST      21
    #define DC       20
    #define DIN      19
    #define CLK      18
    #define CS       17

    #define RST_LO   gpio_put(RST, 0);
    #define RST_HI   gpio_put(RST, 1);

    #define DC_LO    gpio_put(DC, 0);
    #define DC_HI    gpio_put(DC, 1);

    #define CS_LO    gpio_put(CS, 0);
    #define CS_HI    gpio_put(CS, 1);
#endif  // USE_SPI


#define USE_SSD1327     1
#define USE_SSD1351     0

#if USE_SSD1327 == 1
    #define ADDRESS               0x3D  // 0x3C or 0x3D
    #define WIDTH                 128
    #define HEIGHT                128

    #define SET_COL_ADDR          0x15
    #define SET_SCROLL_DEACTIVATE 0x2E
    #define SET_ROW_ADDR          0x75
    #define SET_CONTRAST          0x81
    #define SET_SEG_REMAP         0xA0
    #define SET_DISP_START_LINE   0xA1
    #define SET_DISP_OFFSET       0xA2
    #define SET_DISP_MODE         0xA4
    #define SET_MUX_RATIO         0xA8
    #define SET_FN_SELECT_A       0xAB
    #define SET_DISP              0xAE 
    #define SET_PHASE_LEN         0xB1
    #define SET_DISP_CLK_DIV      0xB3
    #define SET_SECOND_PRECHARGE  0xB6
    #define SET_GRAYSCALE_TABLE   0xB8
    #define SET_GRAYSCALE_LINEAR  0xB9
    #define SET_PRECHARGE         0xBC
    #define SET_VCOM_DESEL        0xBE
    #define SET_FN_SELECT_B       0xD5
    #define SET_COMMAND_LOCK      0xFD

    #define OLED_ON      0x01
    #define OLED_OFF     0x00

    #define VDD_ON       0x01
    #define VDD_OFF      0x00

    #define REG_CMD      0x80
    #define REG_DATA     0x40
#endif  // USE_SSD1327

void initOLED();

void powerOn();
void powerOff();

void contrast(byte contrast);   // 0-255
void refresh(byte refresh);     // 0-255
void invert(byte invert);       // 0-1
void clear(byte color);

void setPixel(byte x, byte y, byte color);

#endif  // OLED_H