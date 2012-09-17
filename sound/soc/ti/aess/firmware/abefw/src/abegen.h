/*
 * 
 */

#ifndef __ABEGEN_H
#define __ABEGEN_H

#include <stdint.h>

typedef	uint32_t u32;
typedef	int32_t s32;
typedef	uint16_t u16;
typedef	int16_t s16;
typedef	uint8_t u8;
typedef	int8_t s8;

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))

struct abe_firmware {
	
	/* combined DMEM, CMEM, PMEM, SMEM */
	const uint32_t *text;
	int size;

	/* TODO: we can split the memory regions to save space */
};

#endif
