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
#include "restore.h"

int part_fh = -1;
gzFile dump_fh;
u32* bm = NULL;
u8* blk = NULL;
ext4_dump_hdr_t hdr;
char* part_fn = NULL;
char* dump_fn = NULL;

u8 b_flag = 1;

static void help(char* prog)
{
    if (b_flag)
        fprintf(stderr,
            "\nUsage: %s [-c] extfs_partition [dump_file]\n"
            "    -c              Compress the dump file\n"
            "    extfs_partition Partition file name\n"
            "    dump_file       Dump file name. Use stdout if omitted.\n\n",
            prog);
    else
        fprintf(stderr,
            "\nUsage: %s [dump_file] extfs_partition\n"
            "    dump_file       Dump file name. Use stdin if omitted\n"
            "    extfs_partition Partition file name\n\n",
            prog);
    exit(0);
}

int main(int ac, char* av[])
{
    u8 c_flag = 0;

    setlocale(LC_NUMERIC, "");

    char* who = strrchr(av[0], '/');
    if (who)
        who++;
    else
        who = av[0];
    if (strcmp(who, "restore.e4") == 0)
        b_flag = 0;

    char* options = "h";
    if (b_flag)
        options = "hc";

    opterr = 0;
    int c;

    while ((c = getopt(ac, av, options)) != -1)
    {
        switch (c)
        {
        case 'c':
            c_flag = 1;
            break;
        case '?':
            if (optopt == 'c')
                fprintf(stderr, "Option -c requires an argument.\n");
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'\n", optopt);
        case 'h':
        default:
            help(av[0]);
        }
    }

    char* fn1 = NULL;
    char* fn2 = NULL;
    for (int i = optind; i < ac; i++)
    {
        if (fn1 == NULL)
            fn1 = av[i];
        else if (fn2 == NULL)
        {
            fn2 = av[i];
            break;
        }
    }
    if (fn1 == NULL)
    {
        fprintf(stderr, "File name(s) must be specified\n");
        help(av[0]);
    }
    if (fn2 == NULL)
    {
        part_fn = fn1;
        dump_fn = NULL;
    }
    else
    {
        part_fn = b_flag ? fn1 : fn2;
        dump_fn = b_flag ? fn2 : fn1;
    }

    if (dump_fn)
    {
        who = strrchr(dump_fn, '.');
        if (!who)
        {
            char* cp = common_malloc(strlen(dump_fn) + 5, "file name");
            strcpy(cp, dump_fn);
            strcat(cp, ".bgz");
            dump_fn = cp;
        }
    }

    time_t start_time = time(NULL);

    b_flag ? dump(c_flag) : restore();

    time_t elapsed = time(NULL) - start_time;
    u32 s = elapsed % 60;
    elapsed /= 60;
    u32 m = elapsed % 60;
    elapsed /= 60;
    fprintf(stderr, "Elapsed time %02ld:%02d:%02d\n", elapsed, m, s);

    return 0;
}
