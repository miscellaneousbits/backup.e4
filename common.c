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

#include "common.h"

u64 block_count;
char* part_fn;
u8* blk;
bm_word_t* part_bm;
int part_fh;
u32 first_block;
u16 block_size;
ext4_dump_hdr_t hdr;

ext4_dump_hdr_t hdr;

void print(char* fmt, ...)
{
    ASSERT(fmt);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

void error(char* fmt, ...)
{
    ASSERT(fmt);
    fprintf(stderr, "\n");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(-1);
}

void part_open(u32 write)
{
    ASSERT((write == READ) || (write = WRITE));
    part_fh = open(part_fn,
        ((write == WRITE) ? O_WRONLY : O_RDONLY) | O_EXCL | O_LARGEFILE);
    if (part_fh < 0)
        error("Can't open partition %s\n%s\n", part_fn, strerror(errno));
}

static u64 last_offset;

void part_seek(u64 offset, char* emsg)
{
    ASSERT(offset < block_count * block_size);
    ASSERT(part_fh >= 0);
    last_offset = offset;
    if (lseek64(part_fh, offset, SEEK_SET) != offset)
        error("Can't seek for %s at 0x%'llx\n%s\n", emsg, offset,
            strerror(errno));
}

void part_read(void* buffer, u32 size, char* emsg)
{
    ASSERT(buffer);
    ASSERT(part_fh >= 0);
    if (read(part_fh, buffer, size) != size)
        error("Can't read %s at offset %'lld for %d\n%s\n", emsg, last_offset,
            size, strerror(errno));
}

void part_read_block(u64 block, char* emsg)
{
    ASSERT(block < block_count);
    part_seek(block * block_size, emsg);
    part_read(blk, block_size, emsg);
}

void part_write(void* buffer, u32 size, char* emsg)
{
    ASSERT(buffer);
    ASSERT(part_fh >= 0);
    if (write(part_fh, buffer, size) != size)
        error("Can't write %s\n%s\n", emsg, strerror(errno));
}

void part_write_block(u64 block, char* emsg)
{
    ASSERT(block < block_count);
    part_seek(block * block_size, emsg);
    part_write(blk, block_size, emsg);
}

void part_close(void)
{
    ASSERT(part_fh >= 0);
    close(part_fh);
}

void dump_open(u32 write)
{
}

void dump_read(void* buffer, u32 size, char* emsg)
{
    if (fread(buffer, 1, size, stdin) != size)
        error("Can't read %s\n%s\n", emsg, strerror(errno));
}

void dump_write(void* buffer, u32 size, char* emsg)
{
    if (fwrite(buffer, 1, size, stdout) != size)
        error("Can't write %s\n%s\n", emsg, strerror(errno));
}

void dump_close()
{
    fflush(stdout);
}

void* common_malloc(u64 size, char* emsg)
{
    void* p = malloc((size_t)size);
    if (p == NULL)
        error("Can't allocate memory for %s\n%s\n", emsg, strerror(errno));
    return p;
}

