#pragma once

#include "types.h"

void gen_crc_table(void);
u32 update_crc(u32 crc_accum, u8* data_blk_ptr, u32 data_blk_size);

