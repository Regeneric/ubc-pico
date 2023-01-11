#ifndef EEPROM_H
#define EEPROM_H

#include "commons.h"

#define SEGMENT_A     0x50
#define SEGMENT_B     0x51
#define SEGMENT_C     0x52
#define SEGMENT_D     0x53
#define SEGMENT_E     0x54
#define SEGMENT_F     0x55
#define SEGMENT_G     0x56
#define SEGMENT_H     0x57

struct EEPROM {
    void (*init)(void);

    void (*write)(byte *data, word len, byte segment);
    byte *(*read)(byte segment);
}; extern const struct EEPROM eeprom;


#endif