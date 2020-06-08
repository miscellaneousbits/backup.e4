
#include "restore.h"

#include "common.h"

static void part_write_block(u64 block, char* emsg)
{
    part_seek(block * hdr.block_size, emsg);
    part_write(blk, hdr.block_size, emsg);
}

void restore(void)
{
    printf("Restoring partition %s from backup file %s\n", part_fn, dump_fn);

    dump_open(0, 0);

    printf("Reading header\n");
    dump_read(&hdr, sizeof(hdr), "header");
    if (hdr.magic != 0xe4bae4ba)
        error("Not dump file\n");

    printf("Bytes per block %'d, %'lld blocks\n", hdr.block_size, hdr.blocks);

    bm = common_malloc((hdr.blocks + 7) / 8, "global bitmap");
    blk = common_malloc(hdr.block_size, "block");

    printf("Reading bitmap\n");
    dump_read(bm, (hdr.blocks + 7) / 8, "bitmap");
    u64 cnt = 0;
    for (u64 block = 0; block < hdr.blocks; block++)
        cnt += get_bm(bm, block);
    printf("  %'lld blocks in use\n", cnt);

    part_open(1);

    printf("Restoring data blocks\n");
    cnt = 0;
    for (u64 block = 0; block < hdr.blocks; block++)
    {
        if (get_bm(bm, block))
        {
            dump_read(blk, hdr.block_size, "block");
            part_write_block(block, "block");
            if ((cnt++ & 32767) == 0)
            {
                printf(".");
                fflush(stdout);
            }
        }
    }
    printf(
        "\n%'lld blocks restored (%'lld bytes)\n", cnt, cnt * hdr.block_size);
    part_close();
    dump_close();
    free(bm);
    free(blk);
}
