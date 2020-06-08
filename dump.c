
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
    part_seek(block * sb_block_size, "block bitmap");
    part_read(group_bm, sb_group_bm_bytes, "block bitmap");
}

static void part_read_block(u64 block)
{
    part_seek(block * sb_block_size, "block");
    part_read(blk, sb_block_size, "block");
}

static u64 copy_group_to_global_bm(u64 group)
{
    u64 start_offset = group * sb_blocks_per_group;
    u64 next_offset = start_offset + sb_blocks_per_group;
    if (next_offset > sb_block_count)
        next_offset = sb_block_count;
    next_offset -= start_offset;
    u64 cnt = 0;
    for (u64 block = 0; block < next_offset; block++)
    {
        u32 bit = get_bm(group_bm, block);
        set_bm(bm, block + start_offset, bit);
        cnt += bit;
    }
    return cnt;
}

void dump(u8 comp, u8 force)
{
    printf(
        "Backing up partition %s to backup file %s\n  with compression level "
        "%d\n",
        part_fn, dump_fn, comp);

    part_open(0);

    part_seek(1024, "superblock");
    part_read(&sb, sizeof(sb), "superblock");

    if (le16_to_cpu(sb.s_magic) != 0xEF53)
    {
        fprintf(stderr, "Can't find superblock\n");
        exit(-1);
    }
    if ((le16_to_cpu(sb.s_state) & 1) == 0)
    {
        fprintf(stderr,
            "%s was not cleanly unmounted. try\n  sudo e2fsck -f %s\n", part_fn,
            part_fn);
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

    printf(
        "%'d bytes per block, %'d blocks per group, %'lld blocks, %'d groups\n",
        sb_block_size, sb_blocks_per_group, sb_block_count, groups);

    bm = common_malloc(sb_bm_bytes, "global bitmap");
    group_bm = common_malloc(sb_group_bm_bytes, "group bitmap");
    blk = common_malloc(sb_block_size, "block");

    set_bm(bm, 0, 1);

    printf("Scanning block groups\n");
    u32 gd_offset = sb_block_size;
    if (sb_block_size == 1024)
        gd_offset += 1024;
    u64 cnt = 0;
    for (u64 group = 0; group < groups; group++)
    {
        ext4_group_desc_t gd;
        part_seek(gd_offset + (group * sizeof(gd)), "group descriptors");
        part_read(&gd, sizeof(gd), "group descriptor");
        u64 block_bitmap = le32_to_cpu(gd.bg_block_bitmap_lo);
        if (((le32_to_cpu(sb.s_feature_incompat) & INCOMPAT_64BIT) == 0) &&
            (le16_to_cpu(sb.s_desc_size) > 32))
            block_bitmap |= (u64)le32_to_cpu(gd.bg_block_bitmap_hi) << 32;
        part_read_group_bm(block_bitmap);
        cnt += copy_group_to_global_bm(group);
    }
    if (force && !get_bm(bm, sb_block_count - 1))
    {
        set_bm(bm, sb_block_count - 1, 1);
        cnt++;
    }
    printf("  %'lld in-use blocks\n", cnt);

    dump_open(comp, 1);

    printf("Writing header\n");
    hdr.blocks = sb_block_count;
    hdr.block_size = sb_block_size;
    hdr.magic = 0xe4bae4ba;
    hdr.version = 0;
    dump_write(&hdr, sizeof(hdr), "header");

    printf("Writing partition bitmap\n");
    dump_write(bm, sb_bm_bytes, "bitmap");
    printf("Writing data blocks\n");
    u64 block_cnt = 0;
    for (u64 block = 0; block < sb_block_count; block++)
    {
        if (get_bm(bm, block))
        {
            part_read_block(block);
            dump_write(blk, sb_block_size, "block");
            if ((block_cnt++ & 32767) == 0)
            {
                printf(".");
                fflush(stdout);
            }
        }
    }

    u64 comp_bytes = dump_flush();
    dump_close();
    part_close();
    printf(
        "\n%'lld in-use blocks dumped (%'lld bytes, compressed to %'lld "
        "bytes)\n",
        block_cnt, block_cnt * sb_block_size, comp_bytes);
    free(group_bm);
    free(bm);
    free(blk);
}
