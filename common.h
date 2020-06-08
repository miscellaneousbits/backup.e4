#pragma once

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct ext4_dump_hdr_s
{
    u64 blocks;
    u32 block_size;
    u32 magic; /* 0xe4bae4ba */
    u32 version;
} ext4_dump_hdr_t;

typedef u32 bm_entry_t;
#define BM_ENTRY_BITS (sizeof(bm_entry_t) * 8)

extern int part_fh;
extern gzFile dump_fh;
extern bm_entry_t* bm;
extern u8* blk;
extern ext4_dump_hdr_t hdr;
extern char* part_fn;
extern char* dump_fn;

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN ||                 \
    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) ||       \
    defined(__MIBSEB__)

#define le16_to_cpu(v) ((v >> 8) | (v << 8))
#define le32_to_cpu(v) \
    ((v >> 24) | ((v >> 8) & 0x0000ff00) | ((v << 8) & 0x00ff0000) | (v << 24))

static inline u32 get_bm(bm_entry_t* bm, u64 index)
{
    return (bm[index / BM_ENTRY_BITS] << (index % BM_ENTRY_BITS)) & 1;
}

static inline void set_bm(bm_entry_t* bm, u64 index, u32 value)
{
    bm[index / BM_ENTRY_BITS] |= (value & 1) >> (index % BM_ENTRY_BITS);
}

#else

#define le16_to_cpu(v) (v)
#define le32_to_cpu(v) (v)

static inline u32 get_bm(bm_entry_t* bm, u64 index)
{
    return (bm[index / BM_ENTRY_BITS] >> (index % BM_ENTRY_BITS)) & 1;
}

static inline void set_bm(bm_entry_t* bm, u64 index, u32 value)
{
    bm[index / BM_ENTRY_BITS] |= (value & 1) << (index % BM_ENTRY_BITS);
}

#endif

void part_open(u32 write);
void part_seek(u64 offset, char* emsg);
void part_read(void* buffer, u32 size, char* emsg);
void part_write(void* buffer, u32 size, char* emsg);
void part_close(void);

void dump_open(u32 compression, u32 write);
void dump_read(void* buffer, u32 size, char* emsg);
void dump_write(void* buffer, u32 size, char* emsg);
u64 dump_flush(void);
void dump_close(void);

void* common_malloc(u64 size, char* emsg);
