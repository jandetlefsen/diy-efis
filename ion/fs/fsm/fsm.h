/* FILE: ion_fsm.h */
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


/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#if !defined( FSM_H_28112005 )
#define FSM_H_28112005

#include "../../ion.h"
#include "../global/global.h"




/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES & DEFINITIONS
-----------------------------------------------------------------------------*/

/* The magic value to verify a volume */
#define VOL_MAGIC 0x3E
/* The bit position of the real file descriptor in a file descriptor */
#define FD_MASK 0xFFFF
#define VOL_MASK 0xFF
/* The volume position in a file descriptor */
#define VOL_BIT 16
#define VOL_MASIC_BIT 24
/* Get a real file descriptor from a file descriptor */
#define GET_FD(fd)  ((fd) & FD_MASK)
/* Get a volume's id from a file descriptor */
#define GET_VOL(fd)  (((fd) >> VOL_BIT) & VOL_MASK)
/* Set a volume's id in a file descriptor */
#define SET_VOL_FD(vol, fd) ((VOL_MAGIC<<VOL_MASIC_BIT) | ((vol)<<VOL_BIT) | (fd))
/* Check the fd is correct or not */
#define IS_CORRECT_FD(fd)  (VOL_MAGIC==(((fd) >> VOL_MASIC_BIT)&VOL_MASK))




/* The status of file system manager */
typedef enum fsm_flag_e {
   FSM_INITIALIZED = (1<<0),  /* initialized status */
   FSM_MOUNTED     = (1<<1),  /* mounted status */
   FSM_FS_DIRTY    = (1<<2),  /* DIRTY status by file-system */
   FSM_MS_DIRTY    = (1<<3),  /* DIRTY status by mass-storage */
   FSM_MS_ATTACH   = (1<<4),  /* ATTATCH status from mass-storage */
   FSM_DEV_EJECTED = (1<<5),  /* ejection status of device */

   FSM_FLAG_END

} fsm_flag_t;




/* Reserved */
#define F_EXIST 0
#define F_WRITE 2
#define F_READ 4
#define F_RW 6




/* Structure to include function pointers connected each operation at a file
   system */
typedef struct fsm_op_s {
   int32_t (*init)( void );
   int32_t (*format)( int32_t vol_id, const char *label, uint32_t flag );
   int32_t (*mount)( int32_t vol_id, uint32_t flag );
   int32_t (*umount)( int32_t vol_id, uint32_t flag );
   int32_t (*sync)( int32_t vol_id );
   int32_t (*statfs)( int32_t vol_id, statfs_t *statbuf );

   int32_t (*mkdir)( int32_t vol_id, const char *path, mod_t mode );
   int32_t (*rmdir)( int32_t vol_id, const char *path );
   dir_t* (*opendir)( int32_t vol_id, const char *path );
   dirent_t* (*readdir)( dir_t *debuf );
   int32_t (*rewinddir)( dir_t *debuf );
   int32_t (*closedir)( dir_t *debuf );
   int32_t (*cleandir)( int32_t vol_id, const char *path );
   int32_t (*statdir)( int32_t vol_id, const char *path, statdir_t *statbuf );

   int32_t (*access)( int32_t vol_id, const char *path, int32_t amode );
   int32_t (*creat)( int32_t vol_id, const char *path, mod_t mode );
   int32_t (*open)( int32_t vol_id, const char *path, uint32_t flag, mod_t mode );
   size_t (*read)( int32_t fd, void *buf, size_t bytes );
   size_t (*write)( int32_t fd, const void *buf, size_t bytes );
   offs_t (*lseek)( int32_t fd, offs_t offset, int32_t whence );
   int32_t (*fsync)( int32_t fd );
   int32_t (*close)( int32_t fd );
   int32_t (*closeall)( int32_t vol_id );
   int32_t (*unlink)( int32_t vol_id, const char *path );
   int32_t (*truncate)( int32_t fd, size_t new_size );
   int32_t (*tell)( int32_t fd );
   int32_t (*rename)( int32_t vol_id, const char *oldpath, const char *newpath );
   int32_t (*stat)( int32_t vol_id, const char *path, stat_t *statbuf );
   int32_t (*fstat)( int32_t fd, stat_t *statbuf );
   int32_t (*getattr)( int32_t vol_id, const char *path, uint32_t *attrbuf );
   int32_t (*fgetattr)( int32_t fd, uint32_t *attrbuf );
   int32_t (*setattr)( int32_t vol_id, const char *path, uint32_t attr );
   int32_t (*fsetattr)( int32_t fd, uint32_t attr );
} fsm_op_t;




/* The maximum number of volume per device */
#define VOLUME_PER_DEVICE_MAX 4

/* The maximum number of volume */
#define VOLUME_MAX (DEVICE_NUM * VOLUME_PER_DEVICE_MAX)




/* The name of file system as type */
#define FSNAME_FAT16    ("FAT16")
#define FSNAME_FAT32    ("FAT32")
#define FSNAME_NTFS     ("NTFS")

/* The maximum number of file system */
#define FILESYSTEM_MAX 1
/* The maximum length of file */
#if defined( UNICODE )
#define ALLPATH_LEN_MAX (1024*2)
#else
#define ALLPATH_LEN_MAX (1024*1)
#endif
/*
   The maximum number of elements in file path. Each elements in file's path
   is divided by '\\' or '/' character.
*/
#define ALLPATH_ELEMENTS_MAX 10




/* Assertion phases. This definition determines the level of assertion. */
#if defined( DBG )
#define FSM_ASSERT 3
#define fsm_assert(c)      do { if ( false == (c) ) break(); } while (0)
#else
#define FSM_ASSERT 1
#define fsm_assert(c)      do { if ( false == (c) ) set_errno(e_cfs); } while (0)
#endif

#if FSM_ASSERT >= 1
#define fsm_assert1(c)     fsm_assert(c)
#else
#define fsm_assert1(c)
#endif
#if FSM_ASSERT >= 2
#define fsm_assert2(c)     fsm_assert(c)
#else
#define fsm_assert2(c)
#endif
#if FSM_ASSERT >= 3
#define fsm_assert3(c)     fsm_assert(c)
#else
#define fsm_assert3(c)
#endif




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

void zero_init_fs( void );
int32_t init_fs( void );
int32_t terminate_fs( void );

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

