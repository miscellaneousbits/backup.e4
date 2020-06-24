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

#pragma once

#include "common.h"

typedef struct
{
    uint32_t compr_flag : 4;
    uint32_t force_flag : 1;

} dump_flags_t;

void dump(dump_flags_t flags);

typedef struct ext4_super_block_s
{
    /*00*/ uint32_t s_inodes_count;      /* Inodes count */
    uint32_t s_blocks_count_lo;          /* Blocks count */
    uint32_t s_r_blocks_count_lo;        /* Reserved blocks count */
    uint32_t s_free_blocks_count_lo;     /* Free blocks count */
    /*10*/ uint32_t s_free_inodes_count; /* Free inodes count */
    uint32_t s_first_data_block;         /* First Data Block */
    uint32_t s_log_block_size;           /* Block size */
    uint32_t s_log_cluster_size;         /* Allocation cluster size */
    /*20*/ uint32_t s_blocks_per_group;  /* # Blocks per group */
    uint32_t s_clusters_per_group;       /* # Clusters per group */
    uint32_t s_inodes_per_group;         /* # Inodes per group */
    uint32_t s_mtime;                    /* Mount time */
    /*30*/ uint32_t s_wtime;             /* Write time */
    uint16_t s_mnt_count;                /* Mount count */
    uint16_t s_max_mnt_count;            /* Maximal mount count */
    uint16_t s_magic;                    /* Magic signature */
    uint16_t s_state;                    /* File system state */
    uint16_t s_errors;                   /* Behaviour when detecting errors */
    uint16_t s_minor_rev_level;          /* minor revision level */
    /*40*/ uint32_t s_lastcheck;         /* time of last check */
    uint32_t s_checkinterval;            /* max. time between checks */
    uint32_t s_creator_os;               /* OS */
    uint32_t s_rev_level;                /* Revision level */
    /*50*/ uint16_t s_def_resuid;        /* Default uid for reserved blocks */
    uint16_t s_def_resgid;               /* Default gid for reserved blocks */
    /*
     * These fields are for EXT4_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    uint32_t s_first_ino;               /* First non-reserved inode */
    uint16_t s_inode_size;              /* size of inode structure */
    uint16_t s_block_group_nr;          /* block group # of this superblock */
    uint32_t s_feature_compat;          /* compatible feature set */
    /*60*/ uint32_t s_feature_incompat; /* incompatible feature set */
#define INCOMPAT_64BIT 0x80
    uint32_t s_feature_ro_compat;        /* readonly-compatible feature set */
    /*68*/ uint8_t s_uuid[16];           /* 128-bit uuid for volume */
    /*78*/ char s_volume_name[16];       /* volume name */
    /*88*/ char s_last_mounted[64];      /* directory where last mounted */
    /*C8*/ uint32_t s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
     */
    uint8_t s_prealloc_blocks;         /* Nr of blocks to try to preallocate*/
    uint8_t s_prealloc_dir_blocks;     /* Nr to preallocate for dirs */
    uint16_t s_reserved_gdt_blocks;    /* Per group desc for online growth */
                                       /*
                                        * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
                                        */
    /*D0*/ uint8_t s_journal_uuid[16]; /* uuid of journal superblock */
    /*E0*/ uint32_t s_journal_inum;    /* inode number of journal file */
    uint32_t s_journal_dev;            /* device number of journal file */
    uint32_t s_last_orphan;            /* start of list of inodes to delete */
    uint32_t s_hash_seed[4];           /* HTREE hash seed */
    uint8_t s_def_hash_version;        /* Default hash version to use */
    uint8_t s_jnl_backup_type;
    uint16_t s_desc_size; /* size of group descriptor */
    /*100*/ uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;  /* First metablock block group */
    uint32_t s_mkfs_time;      /* When the filesystem was created */
    uint32_t s_jnl_blocks[17]; /* Backup of the journal inode */
    /* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
    /*150*/ uint32_t s_blocks_count_hi; /* Blocks count */
    uint32_t s_r_blocks_count_hi;       /* Reserved blocks count */
    uint32_t s_free_blocks_count_hi;    /* Free blocks count */
    uint16_t s_min_extra_isize;         /* All inodes have at least # bytes */
    uint16_t s_want_extra_isize;        /* New inodes should reserve # bytes */
    uint32_t s_flags;                   /* Miscellaneous flags */
    uint16_t s_raid_stride;             /* RAID stride */
    uint16_t s_mmp_update_interval;     /* # seconds to wait in MMP checking */
    uint64_t s_mmp_block;               /* Block for multi-mount protection */
    uint32_t s_raid_stripe_width;       /* blocks on all data disks (N*stride)*/
    uint8_t s_log_groups_per_flex;      /* FLEX_BG group size */
    uint8_t s_checksum_type;            /* metadata checksum algorithm used */
    uint8_t s_encryption_level;         /* versioning level for encryption */
    uint8_t s_reserved_pad;             /* Padding to next 32bits */
    uint64_t s_kbytes_written;          /* nr of lifetime kilobytes written */
    uint32_t s_snapshot_inum;           /* Inode number of active snapshot */
    uint32_t s_snapshot_id;             /* sequential ID of active snapshot */
    uint64_t s_snapshot_r_blocks_count; /* reserved blocks for active
                                        snapshot's future use */
    uint32_t s_snapshot_list;           /* inode number of the head of the
                                              on-disk snapshot list */
    uint32_t s_error_count;             /* number of fs errors */
    uint32_t s_first_error_time;        /* first time an error happened */
    uint32_t s_first_error_ino;         /* inode involved in first error */
    uint64_t s_first_error_block;       /* block involved of first error */
    uint8_t s_first_error_func[32];     /* function where the error happened */
    uint32_t s_first_error_line;        /* line number where error happened */
    uint32_t s_last_error_time;         /* most recent time of an error */
    uint32_t s_last_error_ino;          /* inode involved in last error */
    uint32_t s_last_error_line;         /* line number where error happened */
    uint64_t s_last_error_block;        /* block involved of last error */
    uint8_t s_last_error_func[32];      /* function where the error happened */
    uint8_t s_mount_opts[64];
    uint32_t s_usr_quota_inum;     /* inode for tracking user quota */
    uint32_t s_grp_quota_inum;     /* inode for tracking group quota */
    uint32_t s_overhead_clusters;  /* overhead blocks/clusters in fs */
    uint32_t s_backup_bgs[2];      /* groups with sparse_super2 SBs */
    uint8_t s_encrypt_algos[4];    /* Encryption algorithms in use  */
    uint8_t s_encrypt_pw_salt[16]; /* Salt used for string2key algorithm */
    uint32_t s_lpf_ino;            /* Location of the lost+found inode */
    uint32_t s_prj_quota_inum;     /* inode for tracking project quota */
    uint32_t s_checksum_seed;      /* crc32c(uuid) if csum_seed set */
    uint32_t s_reserved[98];       /* Padding to the end of the block */
    uint32_t s_checksum;           /* crc32c(superblock) */
} ext4_super_block_t;

#define EXT4_MIN_DESC_SIZE 32
#define EXT4_MIN_DESC_SIZE_64BIT 64

typedef struct ext4_group_desc_s
{
    uint32_t bg_block_bitmap_lo;      /* Blocks bitmap block */
    uint32_t bg_inode_bitmap_lo;      /* Inodes bitmap block */
    uint32_t bg_inode_table_lo;       /* Inodes table block */
    uint16_t bg_free_blocks_count_lo; /* Free blocks count */
    uint16_t bg_free_inodes_count_lo; /* Free inodes count */
    uint16_t bg_used_dirs_count_lo;   /* Directories count */
    uint16_t bg_flags;                /* EXT4_BG_flags (INODE_UNINIT, etc) */
    uint32_t bg_exclude_bitmap_lo;    /* Exclude bitmap for snapshots */
    uint16_t bg_block_bitmap_csum_lo; /* crc32c(s_uuid+grp_num+bbitmap) LE */
    uint16_t bg_inode_bitmap_csum_lo; /* crc32c(s_uuid+grp_num+ibitmap) LE */
    uint16_t bg_itable_unused_lo;     /* Unused inodes count */
    uint16_t bg_checksum;             /* crc16(sb_uuid+group+desc) */
    uint32_t bg_block_bitmap_hi;      /* Blocks bitmap block MSB */
    uint32_t bg_inode_bitmap_hi;      /* Inodes bitmap block MSB */
    uint32_t bg_inode_table_hi;       /* Inodes table block MSB */
    uint16_t bg_free_blocks_count_hi; /* Free blocks count MSB */
    uint16_t bg_free_inodes_count_hi; /* Free inodes count MSB */
    uint16_t bg_used_dirs_count_hi;   /* Directories count MSB */
    uint16_t bg_itable_unused_hi;     /* Unused inodes count MSB */
    uint32_t bg_exclude_bitmap_hi;    /* Exclude bitmap block MSB */
    uint16_t bg_block_bitmap_csum_hi; /* crc32c(s_uuid+grp_num+bbitmap) BE */
    uint16_t bg_inode_bitmap_csum_hi; /* crc32c(s_uuid+grp_num+ibitmap) BE */
    uint32_t bg_reserved;
} ext4_group_desc_t;
