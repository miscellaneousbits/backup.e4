
#include "dump.h"

#include "common.h"

static ext4_super_block_t sb;
static u16 sb_block_size;
static u32 sb_blocks_per_group;
static u64 sb_block_count;
static u64 sb_group_bm_bytes;
static u64 sb_bm_bytes;
static bm_entry_t* group_bm;

static void part_read_group_bm(u64 block)
{
    part_seek(part_fh, block * sb_block_size, "block bitmap");
    part_read(group_bm, sb_group_bm_bytes, "block bitmap");
}

static void part_read_block(u64 block)
{
    part_seek(part_fh, block * sb_block_size, "block");
    part_read(blk, sb_block_size, "block");
}

static void copy_group_to_global_bm(u64 group)
{
    u64 start_offset = (u64)group * sb_blocks_per_group;
    u64 next_offset = start_offset + sb_blocks_per_group;
    if (next_offset > sb_block_count)
        next_offset = sb_block_count;
    next_offset -= start_offset;
    for (u64 block = 0; block < next_offset; block++)
        set_bm(bm, block + start_offset, get_bm(group_bm, block));
}

void dump(u8 comp)
{
    fprintf(stderr, "Backing up partition %s to backup file %s\n", part_fn,
        dump_fn);
    fprintf(stderr, "Compression effort %d\n", comp);

    part_fh = part_open(part_fn, 0);
    if (part_fh == NULL)
    {
        fprintf(stderr, "Can't open %s\n%s\n", part_fn, strerror(errno));
        exit(-1);
    }

    part_seek(part_fh, 1024, "superblock");
    part_read(&sb, sizeof(sb), "superblock");

    if (le16_to_cpu(sb.s_magic) != 0xEF53)
    {
        fprintf(stderr, "Can't find superblock\n");
        exit(-1);
    }
    sb_block_size = 1024u << le32_to_cpu(sb.s_log_block_size);
    sb_blocks_per_group = le32_to_cpu(sb.s_blocks_per_group);
    sb_block_count = le32_to_cpu(sb.s_blocks_count_lo);
    if ((le32_to_cpu(sb.s_feature_incompat) & INCOMPAT_64BIT) == 0)
        sb_block_count |= (u64)le32_to_cpu(sb.s_blocks_count_hi) << 32;
    sb_bm_bytes = (sb_block_count + 7) / 8;
    sb_group_bm_bytes = (sb_blocks_per_group + 7) / 8;
    u32 groups =
        (u32)((sb_block_count + sb_blocks_per_group - 1) / sb_blocks_per_group);

    fprintf(stderr,
        "Block size %'d  blocks per group %'d  blocks %'lld  groups %'d\n",
        sb_block_size, sb_blocks_per_group, sb_block_count, groups);

    bm = part_malloc(sb_bm_bytes, "global bitmap");
    memset(bm, 0, (size_t)sb_bm_bytes);
    group_bm = part_malloc(sb_group_bm_bytes, "group bitmap");
    blk = part_malloc(sb_block_size, "block");

    set_bm(bm, 0, 1);

    fprintf(stderr, "Scanning block groups\n");
    for (u64 i = 0; i < groups; i++)
    {
        ext4_group_desc_t gd;
        part_seek(part_fh, (u64)sb_block_size + (i * sizeof(gd)), "group descriptors");
        part_read(&gd, sizeof(gd), "group descriptor");
        u64 block_bitmap = le32_to_cpu(gd.bg_block_bitmap_lo);
        if (((le32_to_cpu(sb.s_feature_incompat) & INCOMPAT_64BIT) == 0) &&
            (le16_to_cpu(sb.s_desc_size) > 32))
            block_bitmap |= (u64)le32_to_cpu(gd.bg_block_bitmap_hi) << 32;
        part_read_group_bm(block_bitmap);
        copy_group_to_global_bm(i);
    }
    set_bm(bm, sb_block_count - 1, 1);

    dump_fh = dump_open(dump_fn, comp, 1);
    if (dump_fh == NULL)
    {
        fprintf(stderr, "Can't open %s\n%s\n", dump_fn, strerror(errno));
        exit(-1);
    }

    fprintf(stderr, "Writing header\n");
    hdr.blocks = sb_block_count;
    hdr.block_size = sb_block_size;
    hdr.magic = 0xe4bae4ba;
    dump_write(&hdr, sizeof(hdr), "header");

    fprintf(stderr, "Writing partition bitmap\n");
    dump_write(bm, sb_bm_bytes, "bitmap");
    fprintf(stderr, "Writing data blocks\n");
    u64 block_cnt = 0;
    for (u64 block = 0; block < sb_block_count; block++)
    {
        if (get_bm(bm, block))
        {
            part_read_block(block);
            dump_write(blk, sb_block_size, "block");
            if ((block_cnt++ & 32767) == 0)
            {
                fprintf(stderr, ".");
                fflush(stderr);
            }
        }
    }

    u64 comp_bytes = dump_flush(dump_fh);
    fprintf(stderr,
        "\n%'lld in-use blocks dumped (%'lld bytes, compressed to %'lld "
        "bytes)\n",
        block_cnt, block_cnt * sb_block_size, comp_bytes);
    dump_close(dump_fh);
    fclose(part_fh);
    free(group_bm);
    free(bm);
    free(blk);
}
