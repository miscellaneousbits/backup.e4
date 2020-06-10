# backup.e4 (bare metal backup/restore for est4 file systems)

**backup.e4** is a bare metal backup and restore utility for common Linux ext file systems. It can produce a sector by sector backup image of any ext4 file system which can be later restore to any partition of equal to or greater size.

## Features

* Only saves sectors that are in use.
* Optionally compresses backup.
* Supports backup to stdout.
* Supports restore from stdin.

## Table of Contents

* [Usage](#usage)
	* [Using pipes](#using-pipes)
* [Building from source](#building-from-source)
	* [Requirements](#requirements)
	* [Build](#build)
	* [Install](#install)
* [Security alert](#security-alert)
* [Integration build status](#integration-build-status)
* [Disclaimer](#disclaimer)

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
Note: Using the -h option or no options at all will display help.

Let's backup a file system on partition /dev/sda3.


```sh
pi@raspberrypi:~ $ sudo backup.e4 /dev/sda3 sda3_backup
Backing up partition /dev/sda3 to backup file sda3_backup.bgz

Can't open partition /dev/sda3
Device or resource busy
pi@raspberrypi:~ $ # Oops! Can't produce a safe backup of a mounted file system
pi@raspberrypi:~ $ # Unmount it
pi@raspberrypi:~ $ sudo umount /dev/sda3
pi@raspberrypi:~ $ # Try again
pi@raspberrypi:~ $ sudo backup.e4 /dev/sda3 sda3_backup
Backing up partition /dev/sda3 to backup file sda3_backup.bgz
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
Scanning block groups
  1,226,675 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
......................................
1,226,675 blocks dumped (5,031,782,012 bytes)
Elapsed time 00:01:05
pi@raspberrypi:~ $ # Better!
```
Or, we can request compression.


```sh
pi@raspberrypi:~ $ sudo backup.e4 -c /dev/sda3 sda3_backup_compressed.bgz
Backing up partition /dev/sda3 to backup file sda3_backup_compressed.bgz
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
Scanning block groups
  1,226,675 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
......................................
1,226,675 blocks dumped (5,024,460,800 bytes, compressed to 678,670,664 bytes)
  Compression ratio 87%
Elapsed time 00:02:14
pi@raspberrypi:~ $
```

Note: If the dump file name doesn't have an extension '.bgz' will automatically be appended.


```sh
pi@raspberrypi:~ $ ls -alh sda3_backup*
-rw-r--r-- 1 root root 4.7G Jun  9 17:56 sda3_backup.bgz
-rw-r--r-- 1 root root 648M Jun  9 17:59 sda3_backup_compressed.bgz
pi@raspberrypi:~ $
```

Compression is slower but reduces dump file size.

Now let's try to restore, but first wipe the existing partition.


```sh
pi@raspberrypi:~ $ sudo dd if=/dev/zero of=/dev/sda3 bs=4096 count=1000
1000+0 records in
1000+0 records out
4096000 bytes (4.1 MB, 3.9 MiB) copied, 0.0476506 s, 86.0 MB/s
pi@raspberrypi:~ $ 
pi@raspberrypi:~ $ sudo restore.e4 sda3_backup_compressed /dev/sda3 
Restoring partition /dev/sda3 from backup file sda3_backup_compressed.bgz
Reading header
Bytes per block 4,096, 52,428,000 blocks
Reading bitmap
  1,226,675 blocks in use
Restoring data blocks
......................................
1,226,675 blocks restored (5,024,460,800 bytes)
Elapsed time 00:00:47
pi@raspberrypi:~ $ 
pi@raspberrypi:~ $ sudo e2fsck -f /dev/sda3
e2fsck 1.44.5 (15-Dec-2018)
Pass 1: Checking inodes, blocks, and sizes
Pass 2: Checking directory structure
Pass 3: Checking directory connectivity
Pass 4: Checking reference counts
Pass 5: Checking group summary information
rootfs: 55116/12876800 files (0.1% non-contiguous), 1226675/52428000 blocks
pi@raspberrypi:~ $
```

Looks good!


### Using pipes

**backup.e4** can redirect dump input and output to standard files. By omitting the dump file name you tell **backup.e4** to write the dump file to stdout, and to stdin for restore.

The following is equivalent to the commands given as examples above.


```sh
pi@raspberrypi:~ $ sudo -s
root@raspberrypi:/home/pi# backup.e4 -c /dev/sda3 > sda3_backup_compressed.bgz
Backing up partition /dev/sda3 to backup file stdout
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
Scanning block groups
  1,226,675 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
......................................
1,226,675 blocks dumped (5,024,460,800 bytes, compressed to 678,670,689 bytes)
  Compression ratio 87%
Elapsed time 00:02:14
root@raspberrypi:/home/pi# 
root@raspberrypi:/home/pi# cat /media/sda4/sda3.bgz | restore.e4 /dev/sda3
Restoring partition /dev/sda3 from backup file stdin
Reading header
Bytes per block 4,096, 52,428,000 blocks
Reading bitmap
  1,226,675 blocks in use
Restoring data blocks
......................................
1,226,675 blocks restored (5,024,460,800 bytes)
Elapsed time 00:00:55
root@raspberrypi:/home/pi#
```

Works, but not terribly useful! Wait, you could use it to send your backup to a remote location.


```sh
pi@raspberrypi:~ $ sudo chmod +rw /dev/sda3
pi@raspberrypi:~ $ backup.e4 -c /dev/sda3 | ssh jcyr@vm-ubuntu.localdomain "cat >sda3_backup_compressed.bgz"
Backing up partition /dev/sda3 to backup file stdout
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
Scanning block groups
  1,226,675 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
......................................
1,226,675 blocks dumped (-1 bytes)
Elapsed time 00:02:17
pi@raspberrypi:~
```

Now, restore from remote.


```sh
pi@raspberrypi:~ $ scp jcyr@vm-ubuntu:sda3_backup_compressed.bgz /dev/stdout | restore.e4 /dev/sda3
Restoring partition /dev/sda3 from backup file stdin
Reading header
Bytes per block 4,096, 52,428,000 blocks
Reading bitmap
  1,226,675 blocks in use
Restoring data blocks
......................................
1,226,675 blocks restored (5,024,460,800 bytes)
Elapsed time 00:00:49
pi@raspberrypi:~
```

## Building from source

### Requirements

All of the examples were captured on a Raspberry Pi4B but should work on any Debian (Ubuntu) host.

**backup.e4** relies on a few common packages that may not be pre-installed in your distribution. Install them with:

```sh
sudo apt install zlib1g-dev git
```

### Build

Retrieve the source code

```sh
git clone https://github.com/miscellaneousbits/backup.e4.git
```

Compile and link

```sh
cd backup.e4
make
```
### Install

Install **backup.e4**

```sh
make install
```

By default this will install to /usr/local/bin. If this is not suitable change the INSTALLDIR variable in the Makefile before installing.

## Security alert

Like any bare metal backup utility **backup.e4** copies file system data verbatim. Dump files will therefore likely contain unencrypted password and private key data. Backup dumps must remain secured at all times. Alternatively the backups should be encrypted.

## Integration build status

![C/C++ CI](https://github.com/miscellaneousbits/backup.e4/workflows/C/C++%20CI/badge.svg)

## Disclaimer

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
