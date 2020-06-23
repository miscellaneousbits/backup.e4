# backup.e4 (bare metal backup/restore for ext4 file systems)

![backup.e4](images/ext4.png)

**backup.e4** is a bare metal backup and restore utility for common Linux ext file systems. It can produce a sector by sector backup image of any ext4 file system which can be later restored to any partition of equal to or greater size.

## Features

* Only saves sectors that are in use.
* Supports backup to stdout.
* Supports restore from stdin.
* Can automatically compress and decompress backups.

Want to support this open source project? Please star it.

## Table of Contents

* [Usage](#usage)
	* [Using pipes](#using-pipes)
* [Building from source](#building-from-source)
	* [Requirements](#requirements)
	* [Build](#build)
	* [Install](#install)
* [Installing from release](#installing-from-release)
* [Security alert](#security-alert)
* [Integration build status](#integration-build-status)
* [Verification](#verification)
* [Disclaimer](#disclaimer)

## Usage

**backup.e4** has simple command line parameters, the partition path name and an optional flag.

```
$ backup.e4

Version 1.3-dev. Compiled little-endian Jun 22 2020
Lincensed under GPLv2.  Author Jean M. Cyr.

Usage: backup.e4 [-c 0-9] [-f] extfs_partition_path
    -c Compression level (0-none, 1-low, 9-high)
    -f Force backup of mounted file system (unsafe)

$ restore.e4 

Version 1.3-dev. Compiled little-endian Jun 22 2020
Lincensed under GPLv2.  Author Jean M. Cyr.

Usage: restore.e4 extfs_partition_path

$
```

Note: Using no parameters will display help.

Let's backup a file system on partition /dev/sda3.

```
$ backup.e4 /dev/sda3 > sda3.bak
Backing up partition /dev/sda3
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
  32 bytes per descriptor
Scanning block groups
  1,707,113 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
.....................................................
1,707,113 blocks dumped (6,992,334,848 bytes)
Elapsed time 0:01:21
$
```
Or, we can compress the backup file.

```
$ backup.e4 -c 1 /dev/sda3 > sda3.bgz
Backing up partition /dev/sda3, with compression level 1
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
  32 bytes per descriptor
Scanning block groups
  1,707,113 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
.....................................................
1,707,113 blocks dumped (6,992,334,848 bytes, compressed to 1,240,349,478 bytes)
Elapsed time 0:02:51
$
```

```
$ ls -alh sda3.*
-rw-r--r-- 1 root root 6.6G Jun 22 20:09 sda3.bak
-rw-r--r-- 1 root root 1.2G Jun 22 20:14 sda3.bgz
$
```

Compression is slower but reduces dump file size considerably!

Now let's try to restore, but first wipe the existing partition.

```
$ dd if=/dev/zero of=/dev/sda3 bs=64K
dd: error writing '/dev/sda3': No space left on device
3276751+0 records in
3276750+0 records out
214745088000 bytes (215 GB, 200 GiB) copied, 655.281 s, 328 MB/s
$
```

```
$ cat sda3.bgz | restore.e4 /dev/sda3
Restoring partition /dev/sda3
Reading header
Bytes per block 4,096, 52,428,000 blocks
Reading bitmap
  1,707,113 blocks in use
Restoring data blocks
.....................................................
1,707,113 blocks restored (6,992,334,848 bytes)
Elapsed time 0:01:17
$
```

Check the restored partition:


```
$ e2fsck -f /dev/sda3
e2fsck 1.44.5 (15-Dec-2018)
Pass 1: Checking inodes, blocks, and sizes
Pass 2: Checking directory structure
Pass 3: Checking directory connectivity
Pass 4: Checking reference counts
Pass 5: Checking group summary information
rootfs: 109549/12928000 files (0.2% non-contiguous), 1707113/52428000 blocks
$
```

Looks good!

### Using pipes

Great flexibility is achieved through the use of stdin and stdout pipes.

For example, you could use them to send your backup to a remote location.

```
$ backup.e4 -c 1 /dev/sda3 | ssh userid@vm-ubuntu.localdomain "cat > sda3.bgz"
Backing up partition /dev/sda3, with compression level 1
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
  32 bytes per descriptor
Scanning block groups
  1,707,113 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
.....................................................
1,707,113 blocks dumped (6,992,334,848 bytes)
Elapsed time 0:02:57
$ 
```

Now, restore from remote.

```
$ scp userid@vm-ubuntu:sda3.bgz /dev/stdout | restore.e4 /dev/sda3
Restoring partition /dev/sda3
Reading header
Bytes per block 4,096, 52,428,000 blocks
Reading bitmap
  1,707,113 blocks in use
Restoring data blocks
.....................................................
1,707,113 blocks restored (6,992,334,848 bytes)
Elapsed time 0:01:16
$ 
```

## Building from source

### Requirements

All of the examples were captured on a Raspberry Pi4B but should work on any Debian (Ubuntu) host.

**backup.e4** relies on a few common packages that are usually pre-installed in your distribution. If not, Install them with:

```
sudo apt install gcc git zlib1g-dev
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

By default this will install to /usr/local/bin. You can override this with:

```
make install INSTALLDIR=~/bin
```

## Installing from release

Backup.e4 and restore.e4 are in fact a single program, hence the single binary release executable.

Install a release binary as follows:

```
$ # Retrieve the release binary
$ wget https://github.com/miscellaneousbits/backup.e4/releases/download/release-2.0/raspios-buster-armv7l-backup.e4
HTTP request sent, awaiting response... 302 Found
HTTP request sent, awaiting response... 200 OK
Length: 13796 (13K) [application/octet-stream]
Saving to: ‘raspios-buster-armv7l-backup.e4’

raspios-buster-armv7l-backup. 100%[=================================================>]  13.47K  --.-KB/s    in 0.001s  

2020-06-22 22:14:18 (11.0 MB/s) - ‘raspios-buster-armv7l-backup.e4’ saved [13796/13796]

$ # Rename the binary
$ mv raspios-buster-aarch64-backup.e4 backup.e4
$ # Make it executable
$ chmod +x backup.e4
$ # Copy it to the install directory
$ sudo cp backup.e4/backup.e4 /usr/local/bin
$ # Link the restore binary
$ sudo ln -s /usr/local/bin/backup.e4 /usr/local/bin/restore.e4
$ 
```

## Security alert

Like any bare metal backup utility **backup.e4** copies file system data verbatim. Dump files will therefore likely contain unencrypted password and private key data. Backup dumps must remain secured at all times. Alternatively the backups should be encrypted.

For example, an encrypted backup using our private and public RSA keys:

```
$ # Create random 256 bit password
$ PASSWORD=$(openssl rand -base64 32)
$ # Encrypt it with public key
$ echo -n $PASSWORD | openssl rsautl -encrypt -inkey ~/.ssh/id_rsa.pem.pub -pubin -out sda3.encrypted.key
$ # Encrypt the backup
$ backup.e4 /dev/sda3 | openssl enc -aes-256-cbc -salt -iter 10 -out sda3_encrypted.bak -pass pass:$PASSWORD
Backing up partition /dev/sda3
4,096 bytes per block, 32,768 blocks per group, 52,428,000 blocks, 1,600 groups
  32 bytes per descriptor
Scanning block groups
  4,244,566 blocks in use
Writing header
Writing partition bitmap
Writing data blocks
..................................................................................................................................
4,244,566 blocks dumped (17,385,742,336 bytes)
Elapsed time 00:07:21
$ # Clear the unencrypted password
$ PASWORD=
$
```

And, to restore:

```
$ # Decrypt the password
$ PASSWORD=$(openssl rsautl -decrypt -inkey ~/.ssh/id_rsa -in sda3.encrypted.key)
$ # Decrypt and restore the backup
$ openssl enc -d -aes-256-cbc -in sda3_encrypted.bak -iter 10 -pass pass:$PASSWORD | restore.e4 /dev/sda3
Restoring partition /dev/sda3
Reading header
Bytes per block 4,096, 52,428,000 blocks
Reading bitmap
  4,244,566 blocks in use
Restoring data blocks
..................................................................................................................................
4,244,566 blocks restored (17,385,742,336 bytes)
Elapsed time 00:06:44
$ # Destroy the unencrypted password
$ PASSWORD=
```

Keep the encrypted key file along with the backup file.

## Integration build status

![C/C++ CI](https://github.com/miscellaneousbits/backup.e4/workflows/C/C++%20CI/badge.svg)

## Verification

Tested and verified on:
* Raspberry Pi OS (32-bit)
* Raspberry Pi OS (64-bit beta)
* Ubuntu 18.04 LTS (x86_64)

## Disclaimer

This software is provided "AS IS" and any expressed or implied warranties, including, but not limited to, the implied warranties of merchantability and fitness for a particular purpose are disclaimed. In no event shall the regents or contributors be liable for any direct, indirect, incidental, special, exemplary, or consequential damages (including, but not limited to, procurement of substitute goods or services; loss of use, data, or profits; or business interruption) however caused and on any theory of liability, whether in contract, strict liability, or tort (including negligence or otherwise) arising in any way out of the use of this software, even if advised of the possibility of such damage.  
