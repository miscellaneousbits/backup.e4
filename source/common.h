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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <zlib.h>

#define BACKUP_MAGIC 0xe4bae4ba

#define STRINGIZE(x) #x
#define STRING_DEFINE(x) STRINGIZE(x)

typedef struct ext4_dump_hdr_s
{
    uint64_t blocks;
    uint32_t block_size;
    uint32_t magic; /* 0xe4bae4ba */
    uint32_t version;

} ext4_dump_hdr_t;

typedef uint32_t bm_word_t;
#define BM_WORD_BITS (sizeof(bm_word_t) * 8)

extern uint64_t block_count;
extern char* part_fn;
extern uint8_t* blk;
extern bm_word_t* part_bm;
extern int part_fh;
extern uint32_t first_block;
extern uint16_t block_size;
extern ext4_dump_hdr_t hdr;

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN ||                 \
    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) ||       \
    defined(__MIBSEB__)

// Big endian

#define L_ENDIAN 0

static inline uint16_t le16_to_cpu(uint16_t v)
{
#if __GNUC__
    return __builtin_bswap16(v);
#else
    return (v >> 8) | (v << 8);
#endif
}

static inline uint32_t le32_to_cpu(uint32_t v)
{
#if __GNUC__
    return __builtin_bswap32(v);
#else
    return (v >> 24) | ((v >> 8) & 0x0000ff00) | ((v << 8) & 0x00ff0000) |
           (v << 24);
#endif
}

static inline uint64_t le64_to_cpu(uint64_t v)
{
#if __GNUC__
    return __builtin_bswap64(v);
#else
    v = ((v << 8) & 0xFF00FF00FF00FF00ULL) | ((v >> 8) & 0x00FF00FF00FF00FFULL);
    v = ((v << 16) & 0xFFFF0000FFFF0000ULL) |
        ((v >> 16) & 0x0000FFFF0000FFFFULL);
    return (v << 32) | (v >> 32);
#endif
}

static inline uint32_t get_bm_bit(bm_word_t* bm, uint64_t index)
{
    assert(index < block_count);
    return (bm[index / BM_WORD_BITS] << (index % BM_WORD_BITS)) & 1;
}

static inline void set_bm_bit(bm_word_t* bm, uint64_t index)
{
    assert(index < block_count);
    bm[index / BM_WORD_BITS] |= 1 >> (index % BM_WORD_BITS);
}

#else

// Little endian

#define L_ENDIAN 1

static inline uint16_t le16_to_cpu(uint16_t v)
{
    return v;
}

static inline uint32_t le32_to_cpu(uint32_t v)
{
    return v;
}

static inline uint64_t le64_to_cpu(uint64_t v)
{
    return v;
}

static inline uint32_t get_bm_bit(bm_word_t* bm, uint64_t index)
{
    assert(index < block_count);
    return (bm[index / BM_WORD_BITS] >> (index % BM_WORD_BITS)) & 1;
}

static inline void set_bm_bit(bm_word_t* bm, uint64_t index)
{
    assert(index < block_count);
    bm[index / BM_WORD_BITS] |= 1 << (index % BM_WORD_BITS);
}

#endif

void print(char* fmt, ...);
void error(char* fmt, ...);

#define READ 0
#define WRITE 1

void part_open(uint32_t write, uint32_t force_flag);
void part_seek(uint64_t offset, char* emsg);
void part_read(void* buffer, uint32_t size, char* emsg);
void part_read_block(uint64_t block, char* emsg);
void part_write_block(uint64_t block, char* emsg);
void part_close(void);

void dump_open(uint32_t write, uint32_t compr_flag);
void dump_read(void* buffer, uint32_t size, char* emsg);
void dump_write(void* buffer, uint32_t size, char* emsg);
int64_t dump_end(void);
void dump_close(void);

void* common_malloc(uint32_t size, char* emsg);
