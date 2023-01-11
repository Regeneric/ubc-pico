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