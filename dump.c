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

#include "dump.h"

static u64 block_count;
static u64 group_bm_bytes;
static u64 part_bm_bytes;
static u32 blocks_per_group;
static u32 feature_incompat;
static u32 groups;
static u16 descriptor_size;
static u16 block_size;

static bm_word_t* group_bm;

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
    start /= BM_WORD_BITS;
    memcpy(bm + start, group_bm, next);

    return cnt;
}

static void load_superblock(void)
{
    ext4_super_block_t* super =
        common_malloc(sizeof(ext4_super_block_t), "super block");

    print("Backing up partition %s to backup file %s\n", part_fn,
        dump_fn ? dump_fn : "stdout");

    part_open(0);

    part_seek(1024, "super block");
    part_read(super, sizeof(*super), "super block");

    if (le16_to_cpu(super->s_magic) != 0xEF53)
        error("Can't find super block\n");

    if ((le16_to_cpu(super->s_state) & 1) == 0)
        error("%s was not cleanly unmounted. try\n  sudo e2fsck -f %s\n",
            part_fn, part_fn);

    block_size = 1024u << le32_to_cpu(super->s_log_block_size);
    blocks_per_group = le32_to_cpu(super->s_blocks_per_group);

    block_count = le32_to_cpu(super->s_blocks_count_lo);
    if ((super->s_feature_incompat & le32_to_cpu(INCOMPAT_64BIT)) == 0)
        block_count |= (u64)le32_to_cpu(super->s_blocks_count_hi) << 32;

    part_bm_bytes = (block_count + 7) / 8;
    group_bm_bytes = (blocks_per_group + 0) / 8;
    feature_incompat = le32_to_cpu(super->s_feature_incompat);
    descriptor_size = le16_to_cpu(super->s_desc_size);
    groups = (u32)((block_count + blocks_per_group - 1) / blocks_per_group);

    free(super);
}

static u64 load_block_group_bitmaps(void)
{
    u32 gd_offset = block_size;
    if (block_size == 1024)
        gd_offset += 1024;

    u16 gd_size;

    if (feature_incompat & INCOMPAT_64BIT)
    {
        gd_size = descriptor_size;
        if (gd_size < EXT4_MIN_DESC_SIZE_64BIT)
            gd_size = EXT4_MIN_DESC_SIZE_64BIT;
    }
    else
        gd_size = EXT4_MIN_DESC_SIZE;

    print("Scanning block groups\n");

    char* gds = common_malloc(groups * gd_size, "group descriptors");
    char* gds_save = gds;

    part_seek(gd_offset, "group descriptors");
    part_read(gds, groups * gd_size, "group descriptors");
    u64 cnt = 0;
    for (u64 group = 0; group < groups; group++)
    {
        ext4_group_desc_t* gd = (ext4_group_desc_t*)gds;
        u64 block_bitmap = le32_to_cpu(gd->bg_block_bitmap_lo);
        if (gd_size > 32)
            block_bitmap |= (u64)le32_to_cpu(gd->bg_block_bitmap_hi) << 32;
        part_read_group_bm(block_bitmap);
        cnt += copy_group_to_global_bm(group);
        gds += gd_size;
    }

    free(gds_save);

    return cnt;
}

static void save_backup(u8 comp)
{
    print("Writing header\n");

    hdr.blocks = block_count;
    hdr.block_size = block_size;
    hdr.magic = 0xe4bae4ba;
    strcpy((char*)&hdr.version, BACKUP_E4_VERSION);

    dump_write(&hdr, sizeof(hdr), "header");

    print("Writing partition bitmap\n");

    dump_write(bm, part_bm_bytes, "bitmap");

    print("Writing data blocks\n");

    u64 block_cnt = 0;

    for (u64 block = 0; block < block_count; block++)
    {
        if (get_bm(bm, block))
        {
            part_read_block(block);
            dump_write(blk, block_size, "block");
            if ((block_cnt++ & 32767) == 0)
                print(".");
        }
    }

    u64 comp_bytes = dump_flush();

    dump_close();
    part_close();

    if (dump_fn)
    {
        if (comp)
            print(
                "\n%'lld blocks dumped (%'lld bytes, compressed to %'lld "
                "bytes)\n  Compression ratio %d%%\n",
                block_cnt, block_cnt * block_size, comp_bytes,
                (u32)(100 - (comp_bytes * 100) / (block_cnt * block_size)));
        else
            print("\n%'lld blocks dumped (%'lld bytes)\n", block_cnt,
                block_cnt * block_size);
    }
    else
        print("\n%'lld blocks dumped (%'lld bytes)\n", block_cnt,
            block_cnt * block_size);
}

void dump(u8 comp)
{
    load_superblock();

    print(
        "%'d bytes per block, %'d blocks per group, %'lld blocks, %'d groups\n",
        block_size, blocks_per_group, block_count, groups);

    bm = common_malloc(part_bm_bytes, "global bitmap");
    group_bm = common_malloc(group_bm_bytes, "group bitmap");
    blk = common_malloc(block_size, "block");

    set_bm(bm, 0);

    u64 cnt = load_block_group_bitmaps();

    print("  %'lld blocks in use\n", cnt);

    dump_open(comp, 1);

    save_backup(comp);

    free(blk);
    free(group_bm);
    free(bm);
}
