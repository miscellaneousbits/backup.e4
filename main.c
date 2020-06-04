#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dump.h"
#include "crc32.h"

static void
help(char* prog)
{
    printf("Usage: %s ext4_partition dump_file\n", prog);
    exit(-1);
}

int main(int ac, char* av[])
{
    if (ac < 3) {
        printf("Missing arguments\n");
        help(av[0]);
    }

    gen_crc_table();

    dump(av[1], av[2]);

    return 0;
}

