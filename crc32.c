#include "crc32.h"

#define POLYNOMIAL 0x04c11db7L      // Standard CRC-32 ppolynomial

static u32 crc_table[256];

void gen_crc_table(void)
{
    register u16 i, j;
    register u32 crc_accum;

    for (i = 0; i < 256; i++)
    {
        crc_accum = ((u32)i << 24);
        for (j = 0; j < 8; j++)
        {
            if (crc_accum & 0x80000000L)
                crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
            else
                crc_accum = (crc_accum << 1);
        }
        crc_table[i] = crc_accum;
    }
}

u32 update_crc(u32 crc_accum, u8* data_blk_ptr, u32 data_blk_size)
{
    register u32 i, j;

    for (j = 0; j < data_blk_size; j++)
    {
        i = ((int)(crc_accum >> 24) ^ *data_blk_ptr++) & 0xFF;
        crc_accum = (crc_accum << 8) ^ crc_table[i];
    }
    crc_accum = ~crc_accum;

    return crc_accum;
}

