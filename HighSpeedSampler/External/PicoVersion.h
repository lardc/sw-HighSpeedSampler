#ifndef __PICOVERSION_H__
#define __PICOVERSION_H__

#include <stdint.h>

#pragma pack(push, 1)
typedef struct tPicoVersion
{
	int16_t major_;
	int16_t minor_;
	int16_t revision_;
	int16_t build_;
} PICO_VERSION;

#pragma pack(pop)

#endif
