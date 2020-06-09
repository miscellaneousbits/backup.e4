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

void error(char* fmt, ...)
{
    fprintf(stderr, "\n");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(-1);
}

void part_open(u32 write)
{
    part_fh =
        open(part_fn, (write ? O_WRONLY : O_RDONLY) | O_EXCL | O_LARGEFILE);
    if (part_fh < 0)
        error("Can't open partition %s\n%s\n", part_fn, strerror(errno));
}

static u64 last_offset;

void part_seek(u64 offset, char* emsg)
{
    last_offset = offset;
    if (lseek64(part_fh, offset, SEEK_SET) != offset)
        error("Can't seek for %s  at 0x%llx\n%s\n", emsg, offset,
            strerror(errno));
}

void part_read(void* buffer, u32 size, char* emsg)
{
    if (read(part_fh, buffer, size) != size)
        error("Can't read %s at offset %'lld for %d\n%s\n", emsg, last_offset,
            size, strerror(errno));
}

void part_write(void* buffer, u32 size, char* emsg)
{
    if (write(part_fh, buffer, size) != size)
        error("Can't write %s\n%s\n", emsg, strerror(errno));
}

void part_close(void)
{
    close(part_fh);
}

void dump_open(u32 comp, u32 write)
{
    char mode[4] = {'r', 'b', 0, 0};
    int f = fileno(stdin);
    if (write)
    {
        mode[0] = 'w';
        mode[2] = '0' + comp;
        f = fileno(stdout);
    }
    if (dump_fn)
        dump_fh = gzopen64(dump_fn, mode);
    else
        dump_fh = gzdopen(f, mode);

    if (dump_fh == Z_NULL)
        error("Can't open dump file\n%s\n", gzerror(dump_fh, &errno));
}

u64 dump_flush()
{
    gzflush(dump_fh, Z_FINISH);
    return gzoffset64(dump_fh);
}

void dump_read(void* buffer, u32 size, char* emsg)
{
    if (gzread(dump_fh, buffer, size) != size)
        error("Can't read dump %s\n%s\n", emsg, gzerror(dump_fh, &errno));
}

void dump_write(void* buffer, u32 size, char* emsg)
{
    if (gzwrite(dump_fh, buffer, size) != size)
        error("Can't write dump %s\n%s\n", emsg, gzerror(dump_fh, &errno));
}

void dump_close()
{
    gzclose(dump_fh);
}

void* common_malloc(u64 size, char* emsg)
{
    void* p = malloc((size_t)size);
    if (p == NULL)
        error("Can't allocate memory for %s\n%s\n", emsg, strerror(errno));
    return p;
}

