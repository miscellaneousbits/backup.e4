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

static u32 part_bm_bytes;
static u32 group_bm_bytes;
static u32 blocks_per_group;
static u32 groups;
static u32 first_block;
static u16 desc_size;
static u8 feature_incompat64;

static bm_word_t* group_bm;

static void part_read_group_bm(u64 block, char* emsg)
{
    part_seek(block * block_size, emsg);
    part_read(group_bm, group_bm_bytes, emsg);
}


static u64 copy_group_to_global_bm(u64 group)
{
    u64 start = group * blocks_per_group;
    u64 next = start + blocks_per_group;
    if (next > block_count - first_block)
        next = block_count - first_block;
    next -= start;

    u64 cnt = 0;
    for (u64 block = 0; block < next - first_block; block++)
        if (get_bm_bit(group_bm, block))
        {
            set_bm_bit(part_bm, start + block + first_block);
            cnt++;
        }

    return cnt;
}

static void load_superblock(void)
{
    ext4_super_block_t* super =
        common_malloc(sizeof(ext4_super_block_t), "super block");

    print("Backing up partition %s\n", part_fn);

    // Fake values for now
    block_count = 2;
    block_size = 1024;

    part_seek(1024, "super block");
    part_read(super, sizeof(*super), "super block");

    if (le16_to_cpu(super->s_magic) != 0xEF53)
        error("Can't find super block\n");

    if ((le16_to_cpu(super->s_state) & 1) == 0)
        error("%s was not cleanly unmounted. try\n  sudo e2fsck -f %s\n",
            part_fn, part_fn);

    block_size = 1024u << le32_to_cpu(super->s_log_block_size);
    assert(block_size <= 64 * 1024);
    blocks_per_group = le32_to_cpu(super->s_blocks_per_group);

    feature_incompat64 =
        (super->s_feature_incompat & le32_to_cpu(INCOMPAT_64BIT)) ==
        le32_to_cpu(INCOMPAT_64BIT);
    block_count = le32_to_cpu(super->s_blocks_count_lo);
    if (!feature_incompat64)
        block_count |= (u64)le32_to_cpu(super->s_blocks_count_hi) << 32;

    part_bm_bytes = (u32)((block_count + 7) / 8);
    assert((blocks_per_group % 8) == 0);
    group_bm_bytes = blocks_per_group / 8;
    groups = (u32)((block_count + blocks_per_group - 1) / blocks_per_group);
    desc_size = le16_to_cpu(super->s_desc_size);
    if (!feature_incompat64)
        desc_size = EXT4_MIN_DESC_SIZE;
    assert((desc_size == EXT4_MIN_DESC_SIZE) ||
           (desc_size == EXT4_MIN_DESC_SIZE_64BIT));

    first_block = le32_to_cpu(super->s_first_data_block);

    free(super);
}

static u64 load_block_group_bitmaps(void)
{
    assert(block_size <= 64 * 1024);
    u32 gd_offset = block_size;
    if (block_size == 1024)
        gd_offset += 1024;

    print("Scanning block groups\n");

    ext4_group_desc_t* gds =
        common_malloc(groups * desc_size, "group descriptors");
    ext4_group_desc_t* gd = gds;

    part_seek(gd_offset, "group descriptors");
    part_read(gds, groups * desc_size, "group descriptors");
    u64 cnt = 0;
    for (u32 group = 0; group < groups; group++)
    {
        u64 block_bitmap = le32_to_cpu(gd->bg_block_bitmap_lo);
        if (desc_size > 32)
            block_bitmap |= (u64)le32_to_cpu(gd->bg_block_bitmap_hi) << 32;
        assert(block_bitmap < block_count);
        part_read_group_bm(block_bitmap, "block bitmap");
        cnt += copy_group_to_global_bm(group);
        gd = (ext4_group_desc_t*)((char*)gd + desc_size);
    }

    free(gds);

    return cnt;
}

static void save_backup(void)
{
    print("Writing header\n");

    hdr.blocks = block_count;
    hdr.block_size = block_size;
    hdr.magic = 0xe4bae4ba;
    strcpy((char*)&hdr.version, BACKUP_E4_VERSION);

    dump_write(&hdr, sizeof(hdr), "header");

    print("Writing partition bitmap\n");

    dump_write(part_bm, part_bm_bytes, "bitmap");

    print("Writing data blocks\n");

    u64 block_cnt = 0;

    for (u64 block = 0; block < block_count; block++)
    {
        if (get_bm_bit(part_bm, block))
        {
            part_read_block(block, "data block");
            dump_write(blk, block_size, "block");
            if ((block_cnt++ & 32767) == 0)
                print(".");
        }
    }

    print("\n%lld blocks dumped (%lld bytes)\n", block_cnt,
        block_cnt * block_size);
}

void dump(void)
{
    part_open(READ);

    load_superblock();

    print(
        "%d bytes per block, %d blocks per group, %lld blocks, %d groups\n"
        "  %d bytes per descriptor\n",
        block_size, blocks_per_group, block_count, groups, desc_size);

    part_bm = common_malloc(part_bm_bytes, "partition bitmap");
    memset(part_bm, 0, part_bm_bytes);
    group_bm = common_malloc(group_bm_bytes, "group bitmap");
    blk = common_malloc(block_size, "block");

    u64 cnt = load_block_group_bitmaps();

    if (!get_bm_bit(part_bm, 0))
    {
        set_bm_bit(part_bm, 0);
        cnt++;
    }

    print("  %lld blocks in use\n", cnt);

    dump_open(WRITE);

    save_backup();

    free(blk);
    free(group_bm);
    free(part_bm);
}
