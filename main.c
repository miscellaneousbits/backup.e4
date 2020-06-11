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

u8 b_flag = 1;

static void help(char* prog)
{
    print(
        "\nUsage: %s extfs_partition\n"
        "    extfs_partition Partition file name\n",
        prog);
    exit(0);
}

static void parse_args(int ac, char* av[])
{
    char* who = strrchr(av[0], '/');
    if (who)
        who++;
    else
        who = av[0];
    if (strcmp(who, "restore.e4") == 0)
        b_flag = 0;

    if (ac < 2)
    {
        print("Missing partition file name");
        help(av[0]);
    }
    part_fn = av[1];
}

int main(int ac, char* av[])
{
    setlocale(LC_NUMERIC, "");

    parse_args(ac, av);

    time_t start_time = time(NULL);

    b_flag ? dump() : restore();

    part_close();
    dump_close();

    time_t elapsed = time(NULL) - start_time;
    u32 s = elapsed % 60;
    elapsed /= 60;
    u32 m = elapsed % 60;
    elapsed /= 60;
    print("Elapsed time %02ld:%02d:%02d\n", elapsed, m, s);

    return 0;
}
