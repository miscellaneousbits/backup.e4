#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "crc32.h"
#include "dump.h"

static ext4_super_block_t sb;
static u16 sb_block_size;
static u32 sb_blocks_per_group;
static u64 sb_block_count;
static u8* group_bm;
static u8* bm;
static u8* blk;
static FILE* part_fh;
static FILE* dump_fh;


static void part_seek(u64 offset, char* emsg)
{
    if (fseeko64(part_fh, offset, SEEK_SET) != 0) {
        fprintf(stderr, "Can't seek for %s  at %016llx\n%s\n", emsg, offset, strerror(errno));
        exit(-1);
    }
}

static void part_read(void* buffer, u64 size, char* emsg)
{
    if (fread(buffer, size, 1, part_fh) != 1) {
        fprintf(stderr, "Can't read %s\n%s\n", emsg, strerror(errno));
        exit(-1);
    }
}

static void* part_malloc(u64 size, char* emsg)
{
    void* p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "Can't allocate memory for %s\n%s\n", emsg, strerror(errno));
        exit(-1);
    }
    memset(p, 0, size);
    return p;
}

static void part_read_group_bm(u64 block)
{
    part_seek(block * sb_block_size, "block bitmap");
    part_read(group_bm, (sb_blocks_per_group + 7) / 8, "block bitmap");
}

static void part_read_block(u64 block)
{
    part_seek(block * sb_block_size, "block");
    part_read(group_bm, sb_block_size, "block");
}

static void set_bm(u64 block, u32 value)
{
    bm[block / 8] |= (value & 1) << (block % 8);
}

static u32 get_group_bm(u64 block)
{
    return (group_bm[block / 8] >> (block % 8)) & 1;
}

static u32 get_bm(u64 block)
{
    return (bm[block / 8] >> (block % 8)) & 1;
}

static void copy_group_to_global_bm(u32 group)
{
    u64 start_offset = (u64)group * sb_blocks_per_group;
    u64 next_offset = start_offset + sb_blocks_per_group;
    if (next_offset > sb_block_count)
        next_offset = sb_block_count;
    next_offset -= start_offset;
    for (u64 block = 0; block < next_offset; block++)
        set_bm(block + start_offset, get_group_bm(block));
}

static void dump_write(void *buffer, u64 size, char* emsg)
{
    if (fwrite(buffer, size, 1, dump_fh) != 1) {
        fprintf(stderr, "Can't write dump %s\n%s\n", emsg, strerror(errno));
        exit(-1);
    }
}

void dump(char* part_fn, char* dump_fn)
{
    printf("Backing up partition %s to backup file %s\n", part_fn, dump_fn);

    part_fh = fopen(part_fn, "rb");
    if (part_fh == NULL) {
        printf("Can't open %s\n%s\n", part_fn, strerror(errno));
        exit(-1);
    }
    dump_fh = fopen(dump_fn, "wb");
    if (dump_fh == NULL) {
        printf("Can't open %s\n%s\n", dump_fn, strerror(errno));
        exit(-1);
    }

    part_seek(1024, "superblock");
    part_read(&sb, sizeof(sb), "superblock");

    if (sb.s_magic != 0xEF53) {
        fprintf(stderr, "Can't find superblock\n");
        exit(-1);
    }
    if (sb.s_checksum_type == 1) {
        if (update_crc(-1, (u8*)&sb, sizeof(sb) - sizeof(sb.s_checksum)) != sb.s_checksum) {
            fprintf(stderr, "Superblock bad checksum\n");
            exit(-1);
        }
    }
    sb_block_size = 1024u << sb.s_log_block_size;
    sb_blocks_per_group = sb.s_blocks_per_group;
    sb_block_count = sb.s_blocks_count_lo;
    if ((sb.s_feature_incompat & INCOMPAT_64BIT) == 0)
        sb_block_count |= (u64)sb.s_blocks_count_hi << 32;
    u32 groups = (u32)((sb_block_count + sb_blocks_per_group - 1) / sb_blocks_per_group);

    printf("Block size %d, blocks per group %d, blocks %lld, block groups %d\n", sb_block_size, sb_blocks_per_group, sb_block_count, groups);

    bm = part_malloc((sb_block_count + 7) / 8, "global bitmap");
    group_bm = part_malloc((sb_blocks_per_group + 7) / 8, "group bitmap");
    blk = part_malloc(sb_block_size, "block");

    printf("Scanning block groups\n");
    for (u32 i = 0; i < groups; i++) {
        ext4_group_desc_t gd;
        part_seek(sb_block_size + i * sizeof(gd), "group descriptors");
        part_read(&gd, sizeof(gd), "group descriptor");
        part_read_group_bm(gd.bg_block_bitmap_lo);
        copy_group_to_global_bm(i);
    }

    printf("Writing partition bitmap\n");
    u64 bm_size = (sb_block_count + 7) / 8;
    dump_write(&bm_size, sizeof(bm_size), "bitmap length");
    dump_write(bm, bm_size, "bitmap");
    printf("Writing data blocks\n");
    u64 block_cnt = 0;
    for (u64 block = 0; block < sb_block_count; block++)
        if (get_bm(block)) {
            part_read_block(block);
            dump_write(blk, sb_block_size, "block");
            block_cnt++;
        }
    printf("%lld blocks dumped\n", block_cnt);
    fclose(dump_fh);
    fclose(part_fh);
    free(group_bm);
    free(bm);
    free(blk);
}

