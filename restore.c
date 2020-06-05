
#include "restore.h"

#include "common.h"

static void part_write(void* buffer, u64 size, char* emsg)
{
    if (fwrite(buffer, (size_t)size, 1, part_fh) != 1)
    {
        fprintf(stderr, "Can't write %s\n%s\n", emsg, strerror(errno));
        exit(-1);
    }
}

static void part_write_block(u64 block, char* emsg)
{
    part_seek(part_fh, block * hdr.block_size, emsg);
    part_write(blk, hdr.block_size, emsg);
}

void restore(void)
{
    fprintf(stderr, "Restoring partition %s from backup file %s\n", part_fn,
        dump_fn);

    dump_fh = dump_open(dump_fn, 0, 0);
    if (dump_fh == NULL)
    {
        fprintf(stderr, "Can't open %s\n%s\n", dump_fn, strerror(errno));
        exit(-1);
    }

    fprintf(stderr, "Reading header\n");
    dump_read(&hdr, sizeof(hdr), "header");
    if (hdr.magic != 0xe4bae4ba)
    {
        fprintf(stderr, "Not dump file\n");
        exit(-1);
    }

    fprintf(
        stderr, "Block size %'d  blocks %'lld\n", hdr.block_size, hdr.blocks);

    bm = part_malloc((hdr.blocks + 7) / 8, "global bitmap");
    blk = part_malloc(hdr.block_size, "block");

    fprintf(stderr, "Reading bitmap\n");
    dump_read(bm, (hdr.blocks + 7) / 8, "bitmap");

    part_fh = part_open(part_fn, 1);
    if (part_fh == NULL)
    {
        fprintf(stderr, "Can't open %s\n%s\n", part_fn, strerror(errno));
        exit(-1);
    }

    fprintf(stderr, "Restoring data blocks\n");
    u64 block_cnt = 0;
    for (u64 block = 0; block < hdr.blocks; block++)
    {
        if (get_bm(bm, block))
        {
            dump_read(blk, hdr.block_size, "block");
            part_write_block(block, "block");
            if ((block_cnt++ & 32767) == 0)
            {
                fprintf(stderr, ".");
                fflush(stderr);
            }
        }
    }
    fprintf(stderr, "\n%'lld in-use blocks restored (%'lld bytes)\n", block_cnt,
        block_cnt * hdr.block_size);
    dump_close(dump_fh);
    fclose(part_fh);
    free(bm);
    free(blk);
}
