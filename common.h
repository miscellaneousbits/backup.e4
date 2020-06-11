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

#pragma once

#include "version.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ASSERT assert

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

typedef u32 bm_word_t;
#define BM_WORD_BITS (sizeof(bm_word_t) * 8)

extern char* part_fn;
extern int part_fh;

extern bm_word_t* part_bm;
extern u8* blk;

extern ext4_dump_hdr_t hdr;

extern u64 block_count;
extern u16 block_size;

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN ||                 \
    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) ||       \
    defined(__MIBSEB__)

#define le16_to_cpu(v) ((v >> 8) | (v << 8))
#define le32_to_cpu(v) \
    ((v >> 24) | ((v >> 8) & 0x0000ff00) | ((v << 8) & 0x00ff0000) | (v << 24))

static inline u32 get_bm_bit(bm_word_t* bm, u64 index)
{
    assert(index < block_count);
    return (bm[index / BM_ENTRY_BITS] << (index % BM_ENTRY_BITS)) & 1;
}

static inline void set_bm_bit(bm_word_t* bm, u64 index)
{
    assert(index < block_count);
    bm[index / BM_ENTRY_BITS] |= 1 >> (index % BM_ENTRY_BITS);
}

#else

#define le16_to_cpu(v) (v)
#define le32_to_cpu(v) (v)

static inline u32 get_bm_bit(bm_word_t* bm, u64 index)
{
    assert(index < block_count);
    return (bm[index / BM_WORD_BITS] >> (index % BM_WORD_BITS)) & 1;
}

static inline void set_bm_bit(bm_word_t* bm, u64 index)
{
    assert(index < block_count);
    bm[index / BM_WORD_BITS] |= 1 << (index % BM_WORD_BITS);
}

#endif

void print(char* fmt, ...);
void error(char* fmt, ...);

#define READ 0
#define WRITE 1

void part_open(u32 write);
void part_seek(u64 offset, char* emsg);
void part_read(void* buffer, u32 size, char* emsg);
void part_read_block(u64 block, char* emsg);
void part_write(void* buffer, u32 size, char* emsg);
void part_write_block(u64 block, char* emsg);
void part_close(void);

void dump_open(u32 write);
void dump_read(void* buffer, u32 size, char* emsg);
void dump_write(void* buffer, u32 size, char* emsg);
void dump_close(void);

void* common_malloc(u64 size, char* emsg);
