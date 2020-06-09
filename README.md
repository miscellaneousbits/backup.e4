# backup.e4 (bare metal backup/restore for est4 file systems)

**backup.e4** is a bare metal backup and restore utility for common Linux ext file systems. It can produce a sector by sector backup image of any ext4 file system which can be later restore to any partition of equal to or greater size.

## Features

* Only saves sectors that are in use.
* Optionally compresses backup.
* Supports backup to stdout.
* Supports restore from stdin.

## Usage

## Usage

**backup.e4** has a few simple command line parameters.

```sh
pi@raspberrypi:~ $ backup.e4 -h

Usage: backup.e4 [-c] extfs_partition [dump_file]
    -c              Compress the dump file
    extfs_partition Partition file name
    dump_file       Dump file name. Use stdout if omitted.

pi@raspberrypi:~ $ restore.e4
File name(s) must be specified

Usage: restore.e4 [dump_file] extfs_partition
    dump_file       Dump file name. Use stdin if omitted
    extfs_partition Partition file name

pi@raspberrypi:~ $
```
Note: By using the -h option or no options at all will display help.

