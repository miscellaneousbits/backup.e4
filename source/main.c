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

static uint8_t backup_flag = 0;
static char* prog = NULL;

static void help(void)
{
    print("\nVersion " BACKUP_E4_VERSION ". Compiled %s-endian " __DATE__
          "\nLincensed under GPLv2.  Author Jean M. Cyr.\n\n",
        L_ENDIAN ? "little" : "big");
    if (backup_flag)
        print(
            "Usage: %s [-f] extfs_partition_path\n"
            "    -f Force backup of mounted file system (unsafe)",
            prog);
    else
        print("Usage: %s extfs_partition_path", prog);
    print("\n\n");
    exit(0);
}

static void parse_args(int ac, char* av[])
{
    int index;
    int c;
    opterr = 0;
    prog = strrchr(av[0], '/');
    if (prog)
        prog++;
    else
        prog = av[0];
    if (strcmp(prog, "backup.e4") == 0)
        backup_flag = 1;

    if (ac < 2)
        help();

    while ((c = getopt(ac, av, "f")) != -1)
        switch (c)
        {
        case 'f':
            force_flag = 1;
            break;
        case '?':
            if (optopt == 'c')
            {
                print("Option -%c requires an argument.\n", optopt);
                help();
            }
            else if (isprint(optopt))
            {
                print("Unknown option `-%c'.\n", optopt);
                help();
            }
            else
            {
                print("Unknown option character `\\x%x'.\n", optopt);
                help();
            }
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
    setlocale(LC_TIME, "");

    parse_args(ac, av);

    time_t start_time = time(NULL);

    backup_flag ? dump() : restore();

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
