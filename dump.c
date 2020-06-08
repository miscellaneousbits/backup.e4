
#include "dump.h"

#include "common.h"

static u16 block_size;
static u32 blocks_per_group;
static u64 block_count;
static u64 group_bm_bytes;
static u64 bm_bytes;

static bm_entry_t* group_bm;

static void part_read_group_bm(u64 block)
{
    part_seek(block * block_size, "block bitmap");
    part_read(group_bm, group_bm_bytes, "block bitmap");
}

static void part_read_block(u64 block)
{
    part_seek(block * block_size, "block");
    part_read(blk, block_size, "block");
}

static u64 copy_group_to_global_bm(u64 group)
{
    u64 start = group * blocks_per_group;
    u64 next = start + blocks_per_group;
    if (next > block_count)
        next = block_count;
    next -= start;

    u64 cnt = 0;
    for (u64 block = 0; block < next; block++)
        if (get_bm(group_bm, block))
            cnt++;

    next /= 8;
    start /= BM_ENTRY_BITS;
    memcpy(bm + start, group_bm, next);

    return cnt;
}

void dump(u8 comp, u8 force)
{
    ext4_super_block_t sb;

    printf(
        "Backing up partition %s to backup file %s\n  with compression level "
        "%d\n",
        part_fn, dump_fn, comp);

    part_open(0);

    part_seek(1024, "super block");
    part_read(&sb, sizeof(sb), "super block");

    if (le16_to_cpu(sb.s_magic) != 0xEF53)
        error("Can't find super block\n");
    if ((le16_to_cpu(sb.s_state) & 1) == 0)
        error("%s was not cleanly unmounted. try\n  sudo e2fsck -f %s\n",
            part_fn, part_fn);

    block_size = 1024u << le32_to_cpu(sb.s_log_block_size);
    blocks_per_group = le32_to_cpu(sb.s_blocks_per_group);
    block_count = le32_to_cpu(sb.s_blocks_count_lo);
    if ((le32_to_cpu(sb.s_feature_incompat) & INCOMPAT_64BIT) == 0)
        block_count |= (u64)le32_to_cpu(sb.s_blocks_count_hi) << 32;

    bm_bytes = (block_count + 7) / 8;
    group_bm_bytes = (blocks_per_group + 7) / 8;
    u32 groups = (u32)((block_count + blocks_per_group - 1) / blocks_per_group);

    printf(
        "%'d bytes per block, %'d blocks per group, %'lld blocks, %'d groups\n",
        block_size, blocks_per_group, block_count, groups);

    bm = common_malloc(bm_bytes, "global bitmap");
    group_bm = common_malloc(group_bm_bytes, "group bitmap");
    blk = common_malloc(block_size, "block");

    set_bm(bm, 0);

    printf("Scanning block groups\n");
    u32 gd_offset = block_size;
    if (block_size == 1024)
        gd_offset += 1024;

    ext4_group_desc_t* gd =
        common_malloc(groups * sizeof(ext4_group_desc_t), "group descriptors");
    part_seek(gd_offset, "group descriptors");
    part_read(gd, groups * sizeof(ext4_group_desc_t), "group descriptors");
    u64 cnt = 0;
    for (u64 group = 0; group < groups; group++)
    {
        u64 block_bitmap = le32_to_cpu(gd[group].bg_block_bitmap_lo);
        if (((le32_to_cpu(sb.s_feature_incompat) & INCOMPAT_64BIT) == 0) &&
            (le16_to_cpu(sb.s_desc_size) > 32))
            block_bitmap |= (u64)le32_to_cpu(gd[group].bg_block_bitmap_hi)
                            << 32;
        part_read_group_bm(block_bitmap);
        cnt += copy_group_to_global_bm(group);
    }
    free(gd);
    if (force && !get_bm(bm, block_count - 1))
    {
        set_bm(bm, block_count - 1);
        cnt++;
    }
    printf("  %'lld blocks in use\n", cnt);

    dump_open(comp, 1);

    printf("Writing header\n");
    hdr.blocks = block_count;
    hdr.block_size = block_size;
    hdr.magic = 0xe4bae4ba;
    hdr.version = 0;
    dump_write(&hdr, sizeof(hdr), "header");

    printf("Writing partition bitmap\n");
    dump_write(bm, bm_bytes, "bitmap");
    printf("Writing data blocks\n");
    u64 block_cnt = 0;
    for (u64 block = 0; block < block_count; block++)
    {
        if (get_bm(bm, block))
        {
            part_read_block(block);
            dump_write(blk, block_size, "block");
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
        "\n%'lld blocks dumped (%'lld bytes, compressed to %'lld "
        "bytes)\n",
        block_cnt, block_cnt * block_size, comp_bytes);
    free(group_bm);
    free(bm);
    free(blk);
}
