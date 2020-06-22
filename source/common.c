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

uint64_t block_count;
char* part_fn;
uint8_t* blk;
bm_word_t* part_bm;
int part_fh;
uint32_t first_block;
uint16_t block_size;
ext4_dump_hdr_t hdr;

void print(char* fmt, ...)
{
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

void error(char* fmt, ...)
{
    assert(fmt);
    fprintf(stderr, "\nFATAL ERROR: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(-1);
}

void part_open(uint32_t write)
{
    assert((write == READ) || (write = WRITE));

    part_fh = open(
        part_fn, ((write == WRITE) ? (O_WRONLY | O_EXCL) :
                                     (O_RDONLY | (force_flag ? 0 : O_EXCL))) |
                     O_LARGEFILE);
    if (part_fh < 0)
        error("Can't open partition %s\n%s\n", part_fn, strerror(errno));
}

void part_seek(uint64_t offset, char* emsg)
{
    assert(offset < block_count * block_size);
    assert(part_fh >= 0);

    if (lseek64(part_fh, offset, SEEK_SET) != offset)
        error("Can't seek for %s at 0x%'llx\n%s\n", emsg, offset,
            strerror(errno));
}

void part_read(void* buffer, uint32_t size, char* emsg)
{
    assert(buffer);
    assert(part_fh >= 0);
    assert(size);

    if (read(part_fh, buffer, size) != size)
        error("Can't read %s\n%s\n", emsg, strerror(errno));
}

void part_read_block(uint64_t block, char* emsg)
{
    assert(block < block_count);
    assert(part_fh >= 0);

    part_seek(block * block_size, emsg);
    part_read(blk, block_size, emsg);
}

void part_write_block(uint64_t block, char* emsg)
{
    assert(block < block_count);
    assert(part_fh >= 0);

    part_seek(block * block_size, emsg);
    if (write(part_fh, blk, block_size) != block_size)
        error("Can't write %s\n%s\n", emsg, strerror(errno));
}

void part_close(void)
{
    assert(part_fh >= 0);

    close(part_fh);
}

static FILE* dump_fd = NULL;

void dump_open(uint32_t write)
{
    assert((write == READ) || (write = WRITE));

    dump_fd = (write == WRITE) ? stdout : stdin;
    if (dump_fd == NULL)
        error("can't open %s\n", write ? "stdout" : "stdin");
}

void dump_read(void* buffer, uint32_t size, char* emsg)
{
    assert(buffer);
    assert(size);
    assert(dump_fd);

    if (fread(buffer, 1, size, dump_fd) != size)
        error("Can't read %s\n%s\n", emsg, strerror(errno));
}

void dump_write(void* buffer, uint32_t size, char* emsg)
{
    assert(buffer);
    assert(size);
    assert(dump_fd);

    if (fwrite(buffer, 1, size, dump_fd) != size)
        error("Can't write %s\n%s\n", emsg, strerror(errno));
}

void dump_close(void)
{
    assert(dump_fd);

    fclose(dump_fd);
}

void* common_malloc(uint64_t size, char* emsg)
{
    assert(size);

    void* p = malloc((size_t)size);
    if (p == NULL)
        error("Can't allocate memory for %s\n%s\n", emsg, strerror(errno));
    return p;
}

