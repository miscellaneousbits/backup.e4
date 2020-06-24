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

uint8_t force_flag = 0;
uint8_t compr_flag = 0;

static uint8_t backup_flag = 0;
static char* prog = NULL;

static void help(void)
{
    print("\nVersion " BACKUP_E4_VERSION ". Compiled %s-endian " __DATE__
          "\nLincensed under GPLv2.  Author Jean M. Cyr.\n\nUsage: ",
        L_ENDIAN ? "little" : "big");
    if (backup_flag)
        print(
            "%s [-c 0-9] [-f] extfs_partition_path\n"
            "    -c Compression level (0-none, 1-low, 9-high)\n"
            "    -f Force backup of mounted file system (unsafe)",
            prog);
    else
        print("%s extfs_partition_path", prog);
    print("\n\n");
    exit(0);
}

static const char* backup_name = STRING_DEFINE(BINB);
static const char* restore_name = STRING_DEFINE(BINR);

static void parse_args(int ac, char* av[])
{
    int index;
    int c;

    if (strcmp(prog, backup_name) == 0)
        backup_flag = 1;

    if (ac < 2)
        help();

    if (!backup_flag && strcmp(prog, restore_name))
        error("I don't recognize myself!\n");

    opterr = 0;

    while ((c = getopt(ac, av, "c:f")) != -1)
        switch (c)
        {
        case 'f':
            force_flag = 1;
            break;
        case 'c':
            if ((strlen(optarg) != 1) || (optarg[0] < '0') || (optarg[0] > '9'))
            {
                print("Compression level must be between 0 and 9\n");
                help();
            }
            compr_flag = optarg[0] - '0';
            break;
        case '?':
            print("Unknown option `-%c'.\n", optopt);
        default:
            help();
        }

    for (index = optind; index < ac; index++)
        if (part_fn == NULL)
            part_fn = av[index];
        else
        {
            print("Extra parameter(s) %s ...\n", av[index]);
            help();
        }
}

int main(int ac, char* av[])
{
    setlocale(LC_NUMERIC, "");

    prog = strrchr(av[0], '/');
    if (prog)
        prog++;
    else
        prog = av[0];

    parse_args(ac, av);

    time_t start_time = time(NULL);

    if (backup_flag)
    {
        dump_flags_t flags = {compr_flag, force_flag};
        dump(flags);
    }
    else
        restore();

    part_close();
    dump_close();

    time_t elapsed = time(NULL) - start_time;
    int sec = elapsed % 60;
    elapsed /= 60;
    int min = elapsed % 60;
    elapsed /= 60;
    int hour = elapsed;
    print("Elapsed time %d:%02d:%02d\n", hour, min, sec);

    return 0;
}
