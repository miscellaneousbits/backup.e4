# backup.e4 (bare metal backup/restore for est4 file systems)

**backup.e4** is a bare metal backup and restore utility for common Linux ext file systems. It can produce a sector by sector backup image of any ext4 file system which can be later restored to any partition of equal to or greater size.

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


```
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


```
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


```
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


```
pi@raspberrypi:~ $ ls -alh sda3_backup*
-rw-r--r-- 1 root root 4.7G Jun  9 17:56 sda3_backup.bgz
-rw-r--r-- 1 root root 648M Jun  9 17:59 sda3_backup_compressed.bgz
pi@raspberrypi:~ $
```

Compression is slower but reduces dump file size.

Now let's try to restore, but first wipe the existing partition.


```
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


```
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


```
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


```
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

```
sudo apt install zlib1g-dev git
```

### Build

Retrieve the source code

```
git clone https://github.com/miscellaneousbits/backup.e4.git
```

Compile and link

```
cd backup.e4
make
```
### Install

Install **backup.e4**

```
make install
```

By default this will install to /usr/local/bin. If this is not suitable change the INSTALLDIR variable in the Makefile before installing.

## Security alert

Like any bare metal backup utility **backup.e4** copies file system data verbatim. Dump files will therefore likely contain unencrypted password and private key data. Backup dumps must remain secured at all times. Alternatively the backups should be encrypted.

For example

```
pi@raspberrypi:~ $ # Create random 256 bit password
pi@raspberrypi:~ $ PASSWORD=$(openssl rand -base64 32)
pi@raspberrypi:~ $ # Encrypt it with public key
pi@raspberrypi:~ $ echo -n $PASSWORD | openssl rsautl -encrypt -inkey ~/.ssh/id_rsa.pem.pub -pubin -out sda3.encrypted.key
pi@raspberrypi:~ $ # Encrypt the backup
pi@raspberrypi:~ $ backup.e4 -c /dev/sda3 | openssl enc -aes-256-cbc -salt -iter 10 -out sda3_encrypted.bgz -pass pass:$PASSWORD 
Backing up partition /dev/sda3 to backup file stdout
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
Scanning block groups
  1,224,569 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
......................................
1,224,569 blocks dumped (5,015,834,624 bytes)
Elapsed time 00:02:15
pi@raspberrypi:~ $ # Clear the password
pi@raspberrypi:~ $ PASWORD=
pi@raspberrypi:~ $
```

And

```
pi@raspberrypi:~ $ #decrypt encryption key
pi@raspberrypi:~ $ PASSWORD=$(openssl rsautl -decrypt -inkey ~/.ssh/id_rsa -in sda3.encrypted.key) 
pi@raspberrypi:~ $ # Decrypt the dump and restore the partition
pi@raspberrypi:~ $ openssl enc -d -aes-256-cbc -in sda3_encrypted.bgz -iter 10 -pass pass:$PASSWORD | restore.e4 /dev/sda3
Restoring partition /dev/sda3 from backup file stdin
Reading header
Bytes per block 4,096, 52,428,000 blocks
Reading bitmap
  1,224,569 blocks in use
Restoring data blocks
......................................
1,224,569 blocks restored (5,015,834,624 bytes)
Elapsed time 00:00:48
pi@raspberrypi:~ $ # Clear the password
pi@raspberrypi:~ $ PASSWORD=
```

## Integration build status

![C/C++ CI](https://github.com/miscellaneousbits/backup.e4/workflows/C/C++%20CI/badge.svg)

## Disclaimer

This software is provided "AS IS" and any expressed or implied warranties, including, but not limited to, the implied warranties of merchantability and fitness for a particular purpose are disclaimed. In no event shall the regents or contributors be liable for any direct, indirect, incidental, special, exemplary, or consequential damages (including, but not limited to, procurement of substitute goods or services; loss of use, data, or profits; or business interruption) however caused and on any theory of liability, whether in contract, strict liability, or tort (including negligence or otherwise) arising in any way out of the use of this software, even if advised of the possibility of such damage.  
