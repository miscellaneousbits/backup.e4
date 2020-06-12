/*
A bare metal backup/restore utility for ext4 file systems
Copyright (C) 2020  Jean M. Cyr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; If not, see <http://www.gnu.org/licenses/>.
*/

#include "restore.h"

void restore(void)
{
    print("Restoring partition %s\n", part_fn);

    dump_open(READ);

    print("Reading header\n");

    dump_read(&hdr, sizeof(hdr), "header");

    if (hdr.magic != 0xe4bae4ba)
        error("Not dump file\n");

    print("Bytes per block %'d, %'lld blocks\n", hdr.block_size, hdr.blocks);

    block_size = hdr.block_size;
    block_count = hdr.blocks;

    uint32_t bm_bytes = (uint32_t)((block_count + 7) / 8);

    part_bm = common_malloc(bm_bytes, "partition bitmap");
    blk = common_malloc(block_size, "block");

    print("Reading bitmap\n");

    dump_read(part_bm, bm_bytes, "bitmap");

    uint64_t cnt = 0;

    for (uint64_t block = 0; block < block_count; block++)
        cnt += get_bm_bit(part_bm, block);

    print("  %'lld blocks in use\n", cnt);

    part_open(WRITE);

    print("Restoring data blocks\n");

    cnt = 0;
    for (uint64_t block = 0; block < block_count; block++)
    {
        if (get_bm_bit(part_bm, block))
        {
            dump_read(blk, block_size, "block");
            part_write_block(block, "data block");
            if ((cnt++ & 32767) == 0)
            {
                print(".");
                fflush(stderr);
            }
        }
    }
    print("\n%'lld blocks restored (%'lld bytes)\n", cnt, cnt * block_size);

    free(part_bm);
    free(blk);
}
