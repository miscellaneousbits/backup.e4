
#include "common.h"

FILE* part_open(char* part_fn, u32 write)
{
    return fopen64(part_fn, write ? "wb" : "rb");
}

void part_seek(FILE* part_fh, u64 offset, char* emsg)
{
    if (fseeko64(part_fh, offset, SEEK_SET) != 0)
    {
        fprintf(stderr, "Can't seek for %s  at %016llx\n%s\n", emsg, offset,
            strerror(errno));
        exit(-1);
    }
}

void part_read(void* buffer, u64 size, char* emsg)
{
    if (fread(buffer, (size_t)size, 1, part_fh) != 1)
    {
        fprintf(stderr, "Can't read %s\n%s\n", emsg, strerror(errno));
        exit(-1);
    }
}

void* part_malloc(u64 size, char* emsg)
{
    void* p = malloc((size_t)size);
    if (p == NULL)
    {
        fprintf(stderr, "Can't allocate memory for %s\n%s\n", emsg,
            strerror(errno));
        exit(-1);
    }
    return p;
}

void dump_write(void* buffer, u64 size, char* emsg)
{
    if (gzwrite(dump_fh, buffer, size) != size)
    {
        fprintf(stderr, "Can't write dump %s\n%s\n", emsg,
            gzerror(dump_fh, &errno));
        exit(-1);
    }
}

gzFile dump_open(char* dump_fn, u32 comp, u32 write)
{
    char mode[4] = {'r', 'b', 0, 0};
    if (write)
    {
        mode[0] = 'w';
        mode[2] = '0' + comp;
    }
    gzFile fh = gzopen64(dump_fn, mode);
    if (fh == Z_NULL)
    {
        fprintf(stderr, "Can't open dump file\n%s\n", gzerror(dump_fh, &errno));
        exit(-1);
    }
    return fh;
}

u64 dump_flush(gzFile dump_fh)
{
    gzflush(dump_fh, Z_FINISH);
    return gzoffset64(dump_fh);
}

void dump_close(gzFile dump_fh)
{
    gzclose(dump_fh);
}

void dump_read(void* buffer, u64 size, char* emsg)
{
    if (gzread(dump_fh, buffer, size) != size)
    {
        fprintf(
            stderr, "Can't read dump %s\n%s\n", emsg, gzerror(dump_fh, &errno));
        exit(-1);
    }
}

