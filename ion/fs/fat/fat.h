/* FILE: ion_fat.h */
/**************************************************************************
* Copyright (C)2009 Spansion LLC and its licensors. All Rights Reserved. 
*
* This software is owned by Spansion or its licensors and published by: 
* Spansion LLC, 915 DeGuigne Dr. Sunnyvale, CA  94088-3453 ("Spansion").
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND 
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software constitutes source code for use in programming Spansion's Flash 
* memory components. This software is licensed by Spansion to be adapted only 
* for use in systems utilizing Spansion's Flash memories. Spansion is not be 
* responsible for misuse or illegal use of this software for devices not 
* supported herein.  Spansion is providing this source code "AS IS" and will 
* not be responsible for issues arising from incorrect user implementation 
* of the source code herein.  
*
* SPANSION MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE, 
* REGARDING THE SOFTWARE, ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED 
* USE, INCLUDING, WITHOUT LIMITATION, NO IMPLIED WARRANTY OF MERCHANTABILITY, 
* FITNESS FOR A  PARTICULAR PURPOSE OR USE, OR NONINFRINGEMENT.  SPANSION WILL 
* HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT, NEGLIGENCE OR 
* OTHERWISE) FOR ANY DAMAGES ARISING FROM USE OR INABILITY TO USE THE SOFTWARE, 
* INCLUDING, WITHOUT LIMITATION, ANY DIRECT, INDIRECT, INCIDENTAL, 
* SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA, SAVINGS OR PROFITS, 
* EVEN IF SPANSION HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
*
* This software may be replicated in part or whole for the licensed use, 
* with the restriction that this Copyright notice must be included with 
* this software, whether used in part or whole, at all times.  
*/


#if !defined( FAT_H_06122005 )
#define FAT_H_06122005

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "../fsm/fsm.h"




typedef enum {
   eFAT_UNKNOWN = 0,
   eFAT16_SIZE = (1<<1), /* NOTE : (uint32_t)eFAT16_SIZE must be 2 */
   eFAT32_SIZE = (1<<2), /* NOTE : (uint32_t)eFAT16_SIZE must be 4 */

   eFAT_END

} fat_type_t;


typedef struct fat_cache_entry_s {
   list_head_t head;        /* "free or active" / "dirty" list */
   list_head_t hash_head;   /* active list only */

   uint8_t vol_id,          /* volume id */
           flag;            /* cache_flag_t */
   uint16_t ref_cnt;        /* count of reference */
   uint32_t sect_no;        /* sector NO */
   uint8_t *buf;            /* cache buffer */

} fat_cache_entry_t;


typedef enum {
   eFAT_FAT_OFFSET_SIG1 = 0x1FE,
   eFAT_FAT_OFFSET_SIG2 = 0x1FF,
   eFAT_FAT_SIG1 = 0x55,
   eFAT_FAT_SIG2 = 0xAA,

   eFAT_BRANCH_INST1 = 0xEB,
   eFAT_BRANCH_INST2 = 0x58,
   eFAT_BRANCH_INST3 = 0x90,

   eFAT_BOOT_SIG = 0x29,

   eFAT_LEAD_SIG = (int32_t) 0x41615252,
   eFAT_STRUC_SIG = (int32_t) 0x61417272,
   eFAT_TRAIL_SIG = (int32_t) 0xAA550000,

   eFAT_START_CLUST = 2,

   /* volume lable max length */
   eFAT_MAX_LABEL_LEN = 11,

   eFAT_INVALID_FREECOUNT = (int32_t) 0xFFFFFFFF,

   eFAT_ATTR_RO = 0x01,
   eFAT_ATTR_HIDDEN = 0x02,
   eFAT_ATTR_SYS = 0x04,
   eFAT_ATTR_VOL = 0x08,
   eFAT_ATTR_DIR = 0x10,
   eFAT_ATTR_ARCH = 0x20,
   eFAT_ATTR_LONG_NAME = (eFAT_ATTR_RO | eFAT_ATTR_HIDDEN |
                          eFAT_ATTR_SYS | eFAT_ATTR_VOL),

   /* fat32-entry's valid bits */
   eFAT32_MASK = (int32_t) 0x0FFFFFFF,

    /* bad cluster mark */
   eFAT16_BAD = (int32_t) 0xFFF7,
   eFAT32_BAD = (int32_t) 0xFFFFFFF7,

   /* standard EOC (End Of Clusterchain) */
   eFAT16_EOC = (int32_t) 0xFFFF,
   eFAT32_EOC = (int32_t) 0x0FFFFFFF,
   eFAT_EOC = (int32_t) 0xFFFFFFFF,

   /* End of file as file system format. */
   eFAT16_EOF = (int32_t) 0xFFF8,
   eFAT32_EOF = (int32_t) 0x0FFFFFF8,
   eFAT_EOF = eFAT32_EOF,

   /* mark free cluster */
   eFAT_FREE = 0,

   eFAT_REPLACE_JAPAN = 0x05,
   eFAT_DELETED_FLAG = 0xE5,        /* marks file as deleted when in name[0] */
   eFAT_EMPTY_ENTRY = 0x00,

   eFAT_CASE_UPPER = (1<<1),
   eFAT_CASE_LOWER = (1<<2),
   eFAT_CASE_LOWER_BASE = (1<<3),   /* base is lower case */
   eFAT_CASE_LOWER_EXT = (1<<4),    /* extension is lower case */

   eFAT_ATTR_END

} fat_enum_t;




#define IS_FAT16_EOC(clust) (eFAT16_EOF <= (clust))
#define IS_FAT32_EOC(clust) (eFAT32_EOF <= (clust))
#define IS_FAT32_FREE(clust) (eFAT_FREE == ((clust) & eFAT32_MASK))
#define FAT_DIRNAME_MAX_LEN  243
#define FAT_FILENAME_MAX_LEN  255
#define FAT_DOT ".          "          /* ".", padded to MSDOS_NAME chars */
#define FAT_DOTDOT "..         "       /* "..", padded to MSDOS_NAME chars */
#define FAT_SHORTNAME_LEN 8            /* name part */
#define FAT_SHORTEXT_LEN 3             /* extention part */
#define FAT_SHORTNAME_SIZE (FAT_SHORTNAME_LEN+FAT_SHORTEXT_LEN)  /* maximum name length */
#define FAT_LONGNAME_SIZE FAT_FILENAME_MAX_LEN  /* maximum long-name length */
#define FAT_LONGNAME_SLOTS 21          /* max # of slots needed for short and long names */
#define FAT_CHARSPERLFN 13             /* number of character in LFN entry */
#define FAT_LFN_SHORT_CNT 1            /* number of short entry in LFN. */
#define FAT_LFN_LAST_ENTRY 0x40
#define FAT_LFN_IDX_MASK 0x3F




typedef struct {
   uint8_t name[FAT_SHORTNAME_LEN];     /* Short name. */
   uint8_t ext[FAT_SHORTEXT_LEN];       /* Extender */
   uint8_t attr;                        /* File attributes */
   uint8_t char_case;                   /* Reserved for use by Windows NT. */
                                        /* Set value to 0 when a file is created and never modify or look at it after that. */
   uint8_t ctime_tenth;                 /* Millisecond stamp at file creation time. */
                                        /* This field actually contains a count of tenths of a second. */
   uint16_t ctime;                      /* Time file was created. */
   uint16_t cdate;                      /* Date file was created. */
   uint16_t adate;                      /* Last access date. */
                                        /* Note that there is no last access time, only a date. */
                                        /* This is the date of last read or write. In the case of a write, */
                                        /* this should be set to the same date as DIR_WrtDate. */
   uint16_t fst_clust_hi;               /* High word of this entry's first cluster number */
                                        /* (always 0 for a FAT12 or FAT16 volume). */
   uint16_t wtime;                      /* Time of last write. Note that file creation is considered a write. */
   uint16_t wdate;                      /* Date of last write. Note that file creation is considered a write. */
   uint16_t fst_clust_lo;               /* Low word of this entry's first cluster number. */
   uint32_t filesize;                   /* 32-bit DWORD holding this file's size in bytes. */

} fat_dirent_t;




typedef struct {
   uint8_t idx;             /* LFN Record Sequence Number */
                            /* bit[0:5] : Hold a 6-bit LFN sequence number (1..63).
                                         Note that this number is one-based.
                                         This limits the number of LFN entries per long name
                                         to 63 entries or 63 * 13 = 819 characters per name.
                                         The longest filename I was able to create using Windows 95 Explorer
                                         was 250 characters. I managed to use a 251-character name
                                         when saving a file in Microsoft Word. I don¡¯t know
                                         if this is the limitation of the FS driver or if it is limited at the application level.
                              bit[6] : Set for the last LFN record in the sequence.
                              bit[7] : Set if the LFN record is an erased long name entry
                                       or maybe if it is part of an erased long name? */
   uint8_t name0_4[5*2];    /* 5 UNICODE characters, LFN first part. */
   uint8_t attr;            /* Attribute */
                            /* This field contains the special value of 0Fh, which indicates an LFN entry */
   uint8_t nt_rsvd;         /* Reserved (probably just set to zero). */
   uint8_t chksum;          /* Checksum of short name entry,
                              used to validate that the LFN entry belongs to the short name entry following.
                              According to Ralf Brown¡¯s interrupt list, the checksum is computed
                              by adding up all the short name characters and rotating the intermediate value
                              right by one bit position before adding each character. */
   uint8_t name5_10[6*2];   /* 6 UNICODE characters, LFN second part. */
   uint16_t fst_clust_lo;   /* Initial cluster number, which is always zero for LFN entries. */
   uint8_t name11_12[2*2];  /* 2 UNICODE characters, LFN third part. */

} fat_lfnent_t;




#define LFN_CHAR_SIZE 2
#define LFN_NAME_CHARS_0_4 5
#define LFN_NAME_CHARS_5_10 6
#define LFN_NAME_CHARS_11_12 2
#define LFN_NAME_CHARS 13  /* maximum number of characters for long file name in a directory entry */
#define LFN_NAME_DEFAULT_CHAR 0xFFFF




/* Common elements in Boot Record. */
typedef struct {
   uint8_t jmp_boot[3];         /* Jump instruction to boot code */
                                /* [0]=0xEB, [1]=0x??, [2]=0x90 or [0]=0xE9, [1]=0x??, [2]=0x?? */
   uint8_t oem_name[8];         /* OEM name, "MSWIN4.1" */
   uint8_t bytes_per_sect[2];   /* Count of bytes per sector */
                                /* This value may take on only the following values: 512, 1024, 2048 or 4096. */
   uint8_t sect_per_clust;      /* Number of sectors per allocation unit. */
                                /* The legal values are 1, 2, 4, 8, 16, 32, 64, and 128. */
   uint16_t rsvd_sect_cnt;      /* Number of reserved sectors in the Reserved region of the volume
                                   starting at the first sector of the volume. */
                                /* FAT16=1, FAT32=0 or 32 */
   uint8_t fat_table_cnt;       /* The count of FAT data structures on the volume. */
                                /* This field should always contain the value 2 for any FAT volume of any type. */
   uint8_t rootent_cnt[2];      /* the count of 32-byte directory entries in the root directory. */
                                /* NOTE : On FAT32, this field must be 0. */
   uint8_t tot_sect16[2];       /* This field is the old 16-bit total count of sectors on the volume. */
                                /* NOTE : On FAT32, this field must be 0. */
   uint8_t media;               /* 0xF8 = non-removable media. 0xF0 = removable media */
   uint16_t fatz16;             /* This field is the FAT12/FAT16 16-bit count of sectors occupied by ONE FAT. */
                                /* NOTE : On FAT32, this field must be 0. */
   uint16_t sect_per_track;     /* The number of sectors per track. */
   uint16_t num_head;           /* The number of headers */
   uint32_t hidd_sect;          /* Count of hidden sectors preceding the partition that contains this FAT volume. */
   uint32_t tot_sect32;         /* This field is the new 32-bit total count of sectors on the volume. */

} fat_bootsect_comm_t;


/* Elements for FAT16 in Boot Record. */
typedef struct {
   uint8_t drv_num;                       /* Driver number. 0x00=Flopy, 0x80=HDD */
   uint8_t rsvd1;                         /* This field should always set this byte to 0. (used by WindowsNT) */
   uint8_t boot_sig;                      /* Extended boot signature (0x29). */
   uint8_t vol_id[4];                     /* Volume serial number. */
   uint8_t vlabel[eFAT_MAX_LABEL_LEN];    /* Volume label. */
   uint8_t fstype_str[8];                 /* FileSystem type. One of the strings "FAT12  ", "FAT16  ", or "FAT    ". */

} fat_bootsect16_t;


/* Elements for FAT32 in Boot Record. */
typedef struct {
   uint32_t fatsz32;                      /* This field is the FAT32 32-bit count of sectors occupied by ONE FAT. */
                                          /* NOTE : fatz16 must be 0. */
   uint16_t ext_flags;                    /* b[0:3] = Zero-based number of active FAT.  */
                                          /* Only valid if mirroring is disabled. */
                                          /* b[4:6] = Reserved. */
                                          /* b[7] = 0 means the FAT is mirrored at runtime into all FATs. */
                                          /*        1 means only one FAT is active; it is the one referenced in bits 0-3. */
                                          /* b[8:15] = Reserved. */
   uint16_t fsver;                        /* High byte is major revision number. */
                                          /* Low byte is minor revision number. */
                                          /* Use 0x0000 */
   uint32_t root_clust;                   /* The cluster number of the first cluster of the root director */
                                          /* Usually 2 */
   uint16_t fsinfo;                       /* Sector number of FSINFO structure in the reserved area of the FAT32 volume. */
                                          /* Usually 1. */
   uint16_t bk_bootsect;                  /* sector number in the reserved area of the volume of a copy of the boot record. */
                                          /* Usually 6. */
   uint8_t rsvds[12];                     /* Reserved for future expansion. */
                                          /* NOTE : On FAT32, this field should always set all of the bytes of this field to 0. */

   uint8_t drv_num;                       /* Driver number. 0x00=Flopy, 0x80=HDD */
   uint8_t rsvd1;                         /* Should always set this byte to 0. (used by WindowsNT) */
   uint8_t boot_sig;                      /* Extended boot signature (0x29). */
   uint8_t vol_id[4];                     /* Volume serial number. */
   uint8_t vlabel[eFAT_MAX_LABEL_LEN];    /* Volume label. */
   uint8_t fstype_str[8];                 /* FileSystem type. One of the strings "FAT12   ", "FAT16   ", or "FAT     ". */

} fat_bootsect32_t;


/* Structure represented all elements of Boot Record. */
typedef struct {
   fat_bootsect_comm_t c;      /* common boot sector */

   union {
      fat_bootsect16_t f16;    /* information just for FAT16 */
      fat_bootsect32_t f32;    /* information just for FAT32 */
   } u;                        /* second boot sector information */

} fat_bootsect_t;


typedef struct {
   uint32_t lead_sig;    /* 0x41615252. This lead signature is used to validate that
                           this is in fact an FSInfo sector. */
   uint8_t rsvds1[480];  /* This field is currently reserved for future expansion.
                           FAT32 format code should always initialize all bytes of this field to 0. */
   uint32_t struc_sig;   /* 0x61417272. Another signature that is more localized in the
                           sector to the location of the fields that are used. */
   uint32_t free_cnt;    /* Contains the last known free cluster count on the volume. */
   uint32_t next_free;   /* It indicates the cluster number at
                           which the driver should start looking for free clusters. */
   uint8_t rsvds2[12];   /* This field is currently reserved for future expansion.
                           FAT32 format code should always initialize all bytes of this field to 0. */
   uint32_t trail_sig;   /* Value 0xAA550000. This trail signature is used to validate that this
                           is in fact an FSInfo sector. */

} fat32_fsinfo_t;




typedef enum {
   eFAT_MediaFixed = 0xF8,
   eFAT_MediaRemovable = 0xF0,

   eFAT_End

} fat_attr_t;




#define FAT_ROOT_CLUST              1




#define DELETE_ENTRY(pEntry) (((uint8_t*)(pEntry))[0]=eFAT_DELETED_FLAG)
#define IS_DELETED_ENTRY(pEntry) (eFAT_DELETED_FLAG==((uint8_t*)(pEntry))[0])
#define IS_EMPTY_ENTRY(pEntry) (eFAT_EMPTY_ENTRY==((uint8_t*)(pEntry))[0])




/* the maximum cluster number that be able to allocate at one time */
#define CLUST_LIST_BUF_CNT 512





/* data cluster to data sector */
#define D_CLUST_2_SECT(fvi, clust) ((fvi)->br.first_data_sect + \
                                    (((clust)-eFAT_START_CLUST) << (fvi)->br.bits_per_clustsect))
/* data sector to data cluster */
#define D_SECT_2_CLUST(fvi, sect) ((((sect)-(fvi)->br.first_data_sect) >> \
                                   (fvi)->br.bits_per_clustsect) + eFAT_START_CLUST)
/* fat16 table's cluster to sector in fat-table */
#define F16T_CLUST_2_SECT(fvi, clust) \
   (((clust)<<1) >> (fvi)->br.bits_per_sect)
/* fat16 table's cluster to offset in cache sector */
#define F16T_CLUST_2_OFFS_INSECT(fvi, clust) \
   ((((clust)<<1) & (fvi)->br.bytes_per_sect_mask) >> 1)
/* fat32 table's cluster to sector in fat-table */
#define F32T_CLUST_2_SECT(fvi, clust) \
   (((clust)<<2) >> (fvi)->br.bits_per_sect)
/* fat16 table's cluster to offset in cache sector */
#define F32T_CLUST_2_OFFS_INSECT(fvi, clust) \
   ((((clust)<<2) & (fvi)->br.bytes_per_sect_mask) >> 2)
/* fat16 & fat32 table's cluster to sector in fat-table */
#define FT_CLUST_2_SECT(fvi, clust) \
   (((clust)<<((uint32_t)(fvi)->br.efat_type >> 1)) >> (fvi)->br.bits_per_sect)
#define FT_CLUST_2_OFFS_INSECT(fvi, clust) \
   (((clust)<<((uint32_t)(fvi)->br.efat_type >> 1)) & (fvi)->br.bytes_per_sect_mask)

/* get sector index */
#define GET_SECT_IDX(fvi, sect) \
   (((sect) - (fvi)->br.first_data_sect) & (fvi)->br.sects_per_clust_mask)




/*
#define fat_get_next_sectno_16root(fvi, fe, sect_no) \
   ((FAT_ROOT_CLUST==(fe)->parent_clust && eFAT16_SIZE == (fvi)->br.efat_type) ? \
   sect_no + 1 : fat_get_next_sectno( fvi->vol_id, sect_no ))
*/
#define fat_get_next_sectno_16root(fvi, fe, sect_no) \
   (((fvi)->br.last_root_sect >= (fe)->parent_sect && eFAT16_SIZE == (fvi)->br.efat_type) ? \
   ((fvi)->br.last_root_sect==sect_no ? eFAT_EOF : sect_no + 1 )\
   : fat_get_next_sectno( fvi->vol_id, sect_no ))

/* Get total data size of file system. */
#define fat_fs_total_bytes(fvi)     ((fvi)->br.data_clust_cnt << (fvi)->br.bits_per_clust)




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

void fat_zinit_fat( void );
void fat_init_fatmap_pool( void );
int32_t fat_init_cache( void );
int32_t fat_reinit_cache_vol( uint32_t vol_id );
void fat_clean_csector( fat_cache_entry_t *entry );
void fat_clean_csectors( uint32_t vol_id );
fat_cache_entry_t *fat_get_csector( uint32_t vol_id, uint32_t sect_no );
int32_t fat_rel_csector( fat_cache_entry_t *entry );
void fat_mark_dirty_csector( fat_cache_entry_t *entry );
int32_t fat_sync_table( uint32_t vol_id, bool isfifo );

int32_t fat_map_init( uint32_t vol_id );
int32_t fat_map_deinit( uint32_t vol_id );
int32_t fat_map_reinit( uint32_t vol_id );
int32_t fat_map_read_free_clusts( uint32_t vol_id, uint32_t clust_cnt, uint32_t *clust_list );
int32_t fat_map_alloc_clusts( uint32_t vol_id, uint32_t clust_cnt, uint32_t *clust_list );
int32_t fat_map_free_clusts( uint32_t vol_id, uint32_t clust_cnt, uint32_t *clust_list );
int32_t fat_map_free_link_clusts( uint32_t vol_id, uint32_t first_clust_no, uint32_t skip_cnt );
int32_t fat_map_sync_clusts( uint32_t vol_id, uint32_t clust_cnt, uint32_t *clust_list );

int32_t fat_stamp_clusts( uint32_t vol_id, uint32_t alloc_clust_cnt, uint32_t *clust_list );
int32_t fat_unlink_clusts( int32_t vol_id, uint32_t first_clust, bool mark_eoc );


int32_t fat_get_free_space( uint32_t vol_id, uint32_t *free_clust_ptr, uint32_t *free_clust_cnt_ptr );
int32_t fat_get_clust_list( uint32_t vol_id, uint32_t clust_no,
                            uint32_t clust_cnt, uint32_t *clust_list );
int32_t fat_get_clust_cnt( uint32_t vol_id, uint32_t clust_no, uint32_t *last_clust_no, uint32_t *last_clust_fat );
int32_t fat_append_clustno( uint32_t vol_id, uint32_t clust_no, uint32_t next_clust );
int32_t fat_get_next_clustno( uint32_t vol_id, uint32_t clust_no );
int32_t fat_set_next_clustno( uint32_t vol_id, uint32_t clust_no, uint32_t next_clust );
int32_t fat_get_next_sectno( uint32_t vol_id, uint32_t sect_no );
int32_t fat_get_clustno( uint32_t vol_id, uint32_t cur_clust, uint32_t clust_offs );
int32_t fat_get_sectno( uint32_t vol_id, uint32_t cur_sect, uint32_t sect_offs );

#endif

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

