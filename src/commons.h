#ifndef COMMONS_H
#define COMMONS_H

#include <stdio.h>

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

#define ARRL(x) (sizeof(x)/sizeof((x)[0]))

#endif