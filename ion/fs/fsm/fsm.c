#include "fsm.h"
#include "../fat/mbr.h"
#include "../fat/fat.h"
#include "../fat/vol.h"
#include "../lim/lim.h"
#include "../osd/osd.h"

#include <stdarg.h>
#include <string.h>
#include <ctype.h>


/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

/* Type definition to represent each operation at the file system */
typedef int32_t(*fsm_setupcallback_t)(fsm_op_t *);

/* Structure to describe the file system */
typedef struct
  {
  uint32_t fs_id, /* A id of file system */
  flag; /* A falg value of file system */
  fsm_op_t op; /* File system operations */

  } fsm_mng_t;

/* Structure for information of a volume */
typedef struct
  {
  int32_t vol_id; /* A volume's id */
  uint32_t flag; /* A flag value of volume */
  fsm_mng_t *fs; /* File system manager of volume */

  } fsm_volinfo_t;

/* Structure to describe a volume */
typedef struct
  {
  int16_t vol_id, /* A volume's id */
  dev_id, /* A id of device including the volume */
  part_no; /* The partition number of volume */

  } fsm_vol_map_t;




/*-----------------------------------------------------------------------------
 DEFINE GLOBAL VARIABLES
-----------------------------------------------------------------------------*/

/* File system mapping table */
#define FSM_ID_FAT 0
#define FSM_ID_NTFS 1

/* File system mapping table */
static fsm_setupcallback_t fsm_setup_callback[FILESYSTEM_MAX] = {
                                                                 /* 0 : FAT16/32 file system */
                                                                 fat_setup,
                                                                 /* 1 : NTFS file system */
                                                                 /* ntfs_setup, */

                                                                 /* ... etc file system */
  };




/* The Volume's map table */
static fsm_vol_map_t fsm_vol_map[VOLUME_NUM];
/* The count of used volumes */
static uint32_t fsm_next_vol_id;




/* Index of file system manager & volume manager */
#define GET_FSM(id) (&fsm_mng[id])
#define GET_FS_VOL(vol) (&fsm_vol[vol])




/* The maximum length of volume name */
#define VOL_NAME_LEN 2




/* File system manager & volume infomation manager */
static fsm_mng_t fsm_mng[FILESYSTEM_MAX];
static fsm_volinfo_t fsm_vol[VOLUME_NUM];
static bool fsm_inited;




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: __get_vol_idx
 Desc: Get the volume's index from the given string.
 Params:
   - vol: The name of volume to get the volume's index.
 Returns:
   int32_t  >= 0 on success. The returned value means the volume's index.
            <  0 on fail.
 Caveats: The value returned is used for the index in the volume's map table.
 */

static int32_t __get_vol_idx(const char *vol)
  {
#define fsm_is_path_delimiter(ch) (((ch==(char)'\\')||(ch==(char)'/')) ? true : false)
#define FIRST_ID_CHAR ((char)'A')
#define LAST_ID_CHAR ((char)'Z')

  char ch_vol_id;
  int32_t vol_idx;


  if ((NULL == vol) || !fsm_is_path_delimiter(vol[0]) || !fsm_is_path_delimiter(vol[2]))
    return set_errno(e_inval);

  ch_vol_id = (char) toupper(vol[1]);

  vol_idx = (int32_t) (ch_vol_id - FIRST_ID_CHAR);

  if ((0 > vol_idx) || (VOLUME_NUM <= vol_idx))
    return set_errno(e_inval);

  return vol_idx;
  }

/*
 Name: __get_vol_id
 Desc: Get the volume's id from the given string.
 Params:
   - vol: The name of volume.
 Returns:
   int32_t >=0 on success. The value returned means the volume's id.
           < 0 on fail.
 Caveats: The volume's id is derived from the volume's map table.
 */

static int32_t __get_vol_id(const char *vol)
  {
  int32_t vol_idx;


  vol_idx = __get_vol_idx(vol);
  if (0 > vol_idx) return vol_idx;

  return fsm_vol_map[vol_idx].vol_id;
  }

/*
 Name: __set_vol_id
 Desc: Fill volume's information in the volume's map table.
 Params:
   - vol: The name of volume to set.
   - dev_id: The ID of device to set.
   - part_no: The partition number to set.
 Returns:
   int32_t >=0 on success. The value returned means the volume's id.
           < 0 on fail.
 Caveats: None.
 */

static int32_t __set_vol_id(const char *vol, int32_t dev_id, int32_t part_no)
  {
  fsm_vol_map_t *vol_map;
  int32_t vol_idx;


  vol_idx = __get_vol_idx(vol);
  if (0 > vol_idx)
    return vol_idx;

  vol_map = &fsm_vol_map[vol_idx];

  /* Fill volume's information in map table */
  if (0 > vol_map->vol_id)
    {
    vol_map->vol_id = (int16_t) (fsm_next_vol_id++);
    vol_map->dev_id = (int16_t) dev_id;
    vol_map->part_no = (int16_t) part_no;
    }
  else if (vol_map->dev_id != dev_id || vol_map->part_no != part_no)
    return set_errno(e_busy);

  return vol_map->vol_id;
  }

/*
 Name: __is_valid_vol
 Desc: Check whether the volume of given string is valid or not.
 Params:
   - fvi: pointer to the structure of volume to be checked.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

static result_t __is_valid_vol(fsm_volinfo_t *fvi)
  {
  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);

  /*
  if ( fvi->flag & FSM_MS_ATTACH )
     return set_errno(e_attach);
   */

  if (fvi->flag & FSM_DEV_EJECTED)
    return set_errno(e_eject);

  return s_ok;
  }

/*
 Name: __sync
 Desc: Synchronize for write-access.
 Params:
   - fvi: pointer to the structure of volume to synchronize.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

static result_t __sync(fsm_volinfo_t *fvi)
  {
  int32_t rtn;


  if (!(FSM_MS_DIRTY & fvi->flag))
    return s_ok;

  if (0 > lock())
    return -1;

  /* Synchronize the file system when data became dirty by mass-storage. */
  rtn = fvi->fs->op.sync(fvi->vol_id);
  fvi->flag &= ~(FSM_MS_DIRTY);

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: __get_valid_vol_id
 Desc: Get valid volume's id
 Params:
   - vol: The volume's name to get the id
 Returns:
   int32_t  >= 0 on sueess. The valid returned is the valid id to the given
                 volume's name.
            <  0 on fail.
 Caveats: None.
 */

static int32_t __get_valid_vol_id(const char *vol)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id;


  if (0 > (vol_id = __get_vol_id(vol)))
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);
  if (0 > __is_valid_vol(fvi))
    return -1;

  if (0 > __sync(fvi))
    return -1;

  return vol_id;
  }

/*
 Name: __fsm_mark_dirty
 Desc: Change information of a volume to the DIRTY status.
 Params:
   - fvi: pointer to information of a volume.
 Returns: None
 Caveats: When data became dirty by mass-storage, the file system become DIRTY
          status.
 */

static void __fsm_mark_dirty(fsm_volinfo_t *fvi)
  {
  fvi->flag |= FSM_FS_DIRTY;
  }

/*
 Name: __fsm_zinit_fsm
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

static void __fsm_zinit_fsm(void)
  {
  memset(&fsm_vol_map, 0, sizeof (fsm_vol_map));
  memset(&fsm_mng, 0, sizeof (fsm_mng));
  memset(&fsm_vol, 0, sizeof (fsm_vol));
  memset(&fsm_inited, 0, sizeof (fsm_inited));
  }

/*
 Name: fat_zero_init
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void zero_init_fs(void)
  {
  __fsm_zinit_fsm();
  fat_zero_init();
  }

/*
 Name: init_fs
 Desc: Initialize the file system. This function must be called to
       use a file system.
 Params: None.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t init_fs(void)
  {
  fsm_setupcallback_t *setup_callback = &fsm_setup_callback[0];
  fsm_mng_t *fsm = GET_FSM(0);
  fsm_volinfo_t *fvi;
  int32_t rtn,
    i;


  if (true == fsm_inited)
    return s_ok;

  /* Initialize to the map table of volume. */
  for (i = 0; i < VOLUME_NUM; i++)
    {
    fsm_vol_map[i].vol_id = -1;
    fsm_vol_map[i].dev_id = -1;
    fsm_vol_map[i].part_no = -1;
    }
  fsm_next_vol_id = 0;

  /* Initialize semaphores for file system. */
  if (0 > os_init_sm())
    return get_errno();

  for (i = 0; i < FILESYSTEM_MAX; i++)
    {
    fsm[i].fs_id = i;
    fsm[i].flag = 0;

    /* Connect file system's foundational functions between FAT and FSM layer. */
    rtn = setup_callback[i](&fsm[i].op);
    if (s_ok != rtn) return set_errno(rtn);

    /* Call the low-level layer's initialization function. */
    rtn = fsm[i].op.init();
    if (s_ok != rtn) return -1;

    fsm[i].flag = FSM_INITIALIZED;
    }

  /* Initialize information of volumes */
  for (i = 0; i < VOLUME_NUM; i++)
    {
    fvi = GET_FS_VOL(i);
    fvi->vol_id = -1;
    fvi->flag = 0;
    fvi->fs = (fsm_mng_t *) NULL;
    }

  /* Initialize file system in the LIM layer */
  if (lim_init() < 0)
    return -1;

  fsm_inited = true;

  return s_ok;
  }

/*
 Name: terminate_fs
 Desc: Terminate the file system.
 Params: None.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: After this function is called, the file system can't use anymore.
 */

int32_t terminate_fs(void)
  {
  fsm_mng_t *fsm = GET_FSM(0);
  int32_t i;


  if (false == fsm_inited)
    return set_errno(e_noinit);

  for (i = 0; i < FILESYSTEM_MAX; i++)
    fsm[i].flag = 0;

  /* Finish semaphores for file system. */
  if (0 > os_terminate_sm())
    return -1;

  /* Terminate file system in the LIM layer */
  if (0 > lim_terminate())
    return -1;

  fsm_inited = false;
  return s_ok;
  }

int stricmp(const char *s1, const char *s2)
  {
  while (*s1 != 0 && *s2 != 0)
    if (tolower(*s1++) != tolower(*s2))
      break;

  return *s1 == *s2 ? 0 : *s1 == 0 ? 1 : -1;
  }

/*
 Name: format
 Desc: Format a file system about a given volume.
 Params:
   - vol: The name of volume to be formatted.
   - dev_id: An ID of the device to be formatted.
   - part_no: A partition number to be formatted.
   - fs_type: A file system type. The type is one of the following string.
              "FAT6" - FAT16 system.
              "FAT32" - FAT32 system.
   - opt: A flag that describes which optional arguments are present. (Not
          supported.)
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: The mounted volume must be formatted after un-mount operation.
 */

int32_t format(const char *vol,
               uint32_t dev_id,
               uint32_t part_no,
               uint32_t start_sec,
               uint32_t cnt,
               const char *fs_type,
               uint32_t opt)
  {
#define BASE_LABEL "ionFS"
  fsm_mng_t *fsm;
  int32_t vol_id,
    rtn;
  char label[8],
    *p;
  uint32_t type;


  if (NULL == vol)
    return set_errno(e_inval);

  if (0 > (vol_id = __set_vol_id(vol, dev_id, part_no)))
    return -1;

  /* Initialize the file system */
  if (0 > (rtn = init_fs()))
    return set_errno(rtn);

  /* Create MBR partition. */
  if (0 > (rtn = mbr_creat_partition(dev_id, part_no, start_sec, cnt, fs_type)))
    return rtn;

  if (0 > (rtn = lim_open(vol_id, dev_id, part_no)))
    return rtn;

  if (!stricmp(FSNAME_FAT16, fs_type))
    {
    fsm = GET_FSM(FSM_ID_FAT);
    type = eFAT16_SIZE;
    }
  else if (!stricmp(FSNAME_FAT32, fs_type))
    {
    fsm = GET_FSM(FSM_ID_FAT);
    type = eFAT32_SIZE;
    }
    /*
    else if ( !stricmp( FSNAME_NTFS, fs_type ) ) {
       fsm = GET_FSM(FSM_ID_NTFS);
       return set_errno( e_inval );
    }
     */
  else
    return set_errno(e_inval);

  if (0 > lock())
    return -1;

  if (!(fsm->flag & FSM_INITIALIZED))
    return set_errno(e_noinit);

  /* Make label for file system */
  p = label + lstrcpy(label, BASE_LABEL, sizeof (BASE_LABEL));
  *p++ = (char) ('0' + vol_id);
  *p = '\0';

  /* Call the real format routine. */
  rtn = fsm->op.format(vol_id, label, type);

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: mount
 Desc: Mount a file system about a given volume.
 Params:
   - vol: The name of volume to be formatted. The name of volume can only
          contain the following characters: 'a' through 'z' regardless of
          character's case.
   - dev_id: An ID of the device to be mounted.
   - part_no: A partition number to be mounted.
   - opt: A flag that describes which optional arguments are present. (Not
          supported.)
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: The volumn sould be formmated for mounting the file system.
 */

int32_t mount(const char *vol, uint32_t dev_id, uint32_t part_no, uint32_t opt)
  {
  fsm_mng_t *fsm;
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    i,
    rtn;


  if (NULL == vol)
    return set_errno(e_inval);

  if (0 > (vol_id = __set_vol_id(vol, dev_id, part_no)))
    return -1;

  /* Initialize the file system */
  if (0 > (rtn = init_fs()))
    return set_errno(rtn);

  if (0 > (rtn = lim_open(vol_id, dev_id, part_no)))
    return rtn;

  fvi = GET_FS_VOL(vol_id);
  /* Returns error if the volume is already mounted. */
  if (FSM_MOUNTED & fvi->flag)
    return set_errno(e_busy);

  rtn = e_nomnt;

  if (0 > lock())
    return -1;

  for (i = 0; i < FILESYSTEM_MAX; i++)
    {
    fsm = GET_FSM(i);

    if (!(FSM_INITIALIZED & fsm->flag))
      return set_errno(e_noinit);

    /* Call the real mount routine. */
    rtn = fsm->op.mount(vol_id, opt);
    if (s_ok == rtn)
      {
      fvi->vol_id = vol_id;
      fvi->flag |= FSM_MOUNTED;
      fvi->fs = fsm;
      break;
      }
    else
      rtn = -1;
    }

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: umount
 Desc: Un-mount a file system about a given volume.
 Params:
   - vol: The volume name of a mounted file system which is to be un-mounted.
   - opt: A flag that describes which optional arguments are present. (Not
          supported.)
 Returns:
   int32_t  0  on success.
           -1  on fail.
 Caveats: If a volume is in use, it cann't be unmounted.
 */

int32_t umount(const char *vol, uint32_t opt)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == vol)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(vol)))
    return -1;

  if (0 > lock())
    return -1;

  fvi = GET_FS_VOL(vol_id);

  /* Call the real un-mount routine. */
  rtn = fvi->fs->op.umount(vol_id, opt);
  if (s_ok == rtn)
    fvi->flag &= ~(uint32_t) FSM_MOUNTED;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: mkdir
 Desc: Make a specitic directory.
 Params:
   - path: A pointer to the null-terminated path name of the directory to be
           made.
   - mode: Reserved.
 Returns:
   iint32_t  0  on success.
            -1  on fail.
 Caveats: None.
 */

int32_t mkdir(const char *path, mod_t mode)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.mkdir(vol_id, &path[VOL_NAME_LEN], mode);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: rmdir
 Desc: Remove a specitic directory.
 Params:
   - path: A pointer to the null-terminated path name of the directory to be
           removed.
 Returns:
   iint32_t  0  on success.
            -1  on fail.
 Caveats: If a directory has at least a file or a sub-directory, it cann't be
          removed.
 */

int32_t rmdir(const char *path)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.rmdir(vol_id, &path[VOL_NAME_LEN]);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: opendir
 Desc: Open a specitic directory stream.
 Params:
   - path: A pointer to the null-terminated path name of the directory to be
           opened.
 Returns:
   DIR_t*  value on success. The value returned is a pointer to a DIR_t.
                       This DIR_t describes the directory.
                 NULL on fail.
 Caveats: None
 */

dir_t* opendir(const char *path)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id;
  dir_t *debuf;


  if (NULL == path)
    {
    set_errno(e_inval);
    return NULL;
    }

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return NULL;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return NULL;

  if (NULL == (debuf = fvi->fs->op.opendir(vol_id, &path[VOL_NAME_LEN])))
    return NULL;
  debuf->fd = SET_VOL_FD(vol_id, debuf->fd);

  if (0 > unlock())
    return NULL;

  return debuf;
  }

/*
 Name: readdir
 Desc: Read a directory entry from a given directory stream.
 Params:
   - de: A pointer to a DIR_t that refers to the open directory stream to
         be read. This pointer is returned by opendir().
 Returns:
   dirent_t*  value on success. The value returned is a pointer to a
                          dirent_t structure describing the next directory
                          entry in the directory stream.
                    NULL on fail or when the operation encounters the end of the
                         directory stream.
 Caveats: None.
 */

dirent_t *readdir(dir_t *de)
  {
  fsm_volinfo_t *fvi;
  dirent_t *dent;


  if (NULL == de)
    {
    set_errno(e_inval);
    return NULL;
    }

  if (!IS_CORRECT_FD(de->fd))
    {
    set_errno(e_badf);
    return NULL;
    }

  fvi = GET_FS_VOL(GET_VOL(de->fd));

  if (!(fvi->flag & FSM_MOUNTED))
    {
    set_errno(e_nomnt);
    return NULL;
    }

  if (0 > lock())
    return NULL;

  if (NULL == (dent = fvi->fs->op.readdir(de)))
    return NULL;

  if (0 > unlock())
    return NULL;

  return dent;
  }

/*
 Name: rewinddir
 Desc: Set the directory position to the beginning of the directory entries in
       directory stream.
 Params:
   - de: A pointer to a DIR_t that refers to the open directory stream to
         be rewound. This pointer is returned by the opendir() function.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t rewinddir(dir_t *de)
  {
  fsm_volinfo_t *fvi;
  int32_t rtn;


  if (NULL == de)
    return set_errno(e_inval);

  if (!IS_CORRECT_FD(de->fd))
    return set_errno(e_badf);

  fvi = GET_FS_VOL(GET_VOL(de->fd));

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.rewinddir(de);

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: closedir
 Desc: Close a specific directory stream.
 Params:
   - de: A pointer to a DIR_t to be closed. This pointer is returned by
         the opendir() function.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t closedir(dir_t *de)
  {
  fsm_volinfo_t *fvi;
  int32_t rtn;


  if (NULL == de)
    return set_errno(e_inval);

  if (!IS_CORRECT_FD(de->fd))
    return set_errno(e_badf);

  fvi = GET_FS_VOL(GET_VOL(de->fd));

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.closedir(de);

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: cleandir
 Desc: Delete all files in directory. (except directory)
 Params:
   - path: A pointer to the null-terminated path name of the directory to be
           cleaned.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t cleandir(const char *path)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.cleandir(vol_id, &path[VOL_NAME_LEN])))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: statdir
 Desc: Get information of a specific directory.
 Params:
   - path: A pointer to the null-terminated path name of the directory to be
           researched.
   - statbuf: Pointer to an object of type struct statdir_t where the
              file information will be written.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t statdir(const char *path, statdir_t *statbuf)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.statdir(vol_id, &path[VOL_NAME_LEN], statbuf)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: access
 Desc: Check for file's accessibility.
 Params:
   - path: A pointer to the null-terminated path name of the file or directory
           to be checked.
   - amode: Bitwise OR of the access permissions to be checked. The file's
            access permission to be checked is one of the following symbols.
            R_OK - Read authority
            W_OK - Write authority
            X_OK - Execution authority.
            F_OK - File existence.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t access(const char *path, int32_t amode)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.access(vol_id, &path[VOL_NAME_LEN], amode)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: creat
 Desc: Create a specific file as given mode.
 Params:
   - path: A pointer to the null-terminated path name of the file to be opened.
   - mode: The file mode indicates the file permission. (Not supported.)
 Returns:
   int32_t  >=0 on success. The value returned is the file descriptor.
            < 0 on fail.
 Caveats: The function truncate the file size to 0 when same name files exist
          there.
 */

int32_t creat(const char *path, mod_t mode)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.creat(vol_id, &path[VOL_NAME_LEN], mode);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    rtn = -1;

  if (0 > unlock())
    return -1;

  if (0 > rtn) return rtn;
  return SET_VOL_FD(vol_id, rtn);
  }

/*
 Name: fopen
 Desc: Open or create a specific file as given the mode.
 Params:
   - path: A pointer to the null-terminated path name of the file to be opened.
   - flag: The status flags and access modes of the file to be opened.
           The flag is one of the following symbols.
           O_RDONLY - open file for reading only file
           O_WRONLY - open file for writing only file
           O_RDWR - open file for both reading and writing.
           O_APPEND - open file with position the file offset at the end of the
                      file. Before each write operation, file position is set
                      at the end of the file.
           O_CREAT - If the file being opened does not exist, it is created and
                     then opened. If the file being opened exists, this mode is
                     no effect.
           O_TRUNC - Truncate the file to zero length if the file exists.
           O_EXCL - Ignored if O_CREAT is not set. It causes the call to
                    open() to fail if the file already exists.
   - mode: Permission bits to use if a file is created. (Ignored)
 Returns:
   int32_t  >=0 on success. The value returned is the file descriptor.
            < 0 on fail.
 Caveats: There directory which is used for the path or files should exist.
          When a file is opened, flag is able to be used with making up each
          other through OR operator.
 */

int32_t fopen(const char *path, uint32_t flag, ... /* mod_t mode */)
  {
  fsm_volinfo_t *fvi;
  mod_t mode;
  int32_t vol_id,
    rtn;
  va_list ap;


  if (NULL == path)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  va_start(ap, flag);
  mode = (mod_t) va_arg(ap, uint32_t);
  va_end(ap);

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.open(vol_id, &path[VOL_NAME_LEN], flag, mode);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    rtn = -1;

  if (0 > unlock())
    return -1;

  if (0 > rtn) return rtn;
  return SET_VOL_FD(vol_id, rtn);
  }

/*
 Name: read
 Desc: Read from a specific file.
 Params:
   - fd: The file descriptor to be read.
   - buf: A pointer to a buffer in which the bytes read are placed.
   - bytes: The number of bytes to be read.
 Returns:
   size_t  >=0 on success. The value returned is the number of bytes actually
               read and placed in buf. If the number of bytes to be read was
               larger than the remained size of file at the current position of
               file, it may be less than bytes.
           < 0 on fail.
 Caveats: If file pointer points to the first of the file and file size is
          smaller than the requested read sizee, file is read as much as file
          size and it lets file pointer move as much as  file was read.
          If the requested read size is larger than data size which remained
          data at the postion of file pointer, this function  would read the
          data as much as remained data.
 */

size_t read(int32_t fd, void *buf, size_t bytes)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  /* Check the status of fd */
  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  /* Get volume's ID. Exist file in volume */
  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  /* Get volume maneger's pointer */
  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);


  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.read(GET_FD(fd), buf, bytes)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
Name: write
Desc: Write to a specific file.
Params:
   - fd: The descriptor of the file to which the data is to be written.
   - buf: A pointer to a buffer containing the data to be written.
   - bytes: The size in bytes of the data to be written.
Returns:
   size_t  >=0 on success. The value returned is the number of bytes actually
               written. This number is less than or equal to bytes.
           < 0 on fail.
Caveats: If the file descriptor have the O_APPEND mode, data would be written
         from the end of file. Or not, data would be written from where file
         pointer point to. Afer file is written, File pointer would be moved
         as much as dada was written
 */

size_t write(int32_t fd, const void *buf, size_t bytes)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);


  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.write(GET_FD(fd), buf, bytes);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: lseek
 Desc: Change the position of file pointer in the file which the 'fd' parameter
       point to.
 Params:
   - fd: The file descriptor whose current file offset you wanted to change.
   - offset: The amount byte offset is to be changed. The sign indicates
             whether the offset is to be moved forward (positive) or backward
             (negative).
   - whence: One of the following symbols.
             SEEK_SET: The start of the file.
             SEEK_CUR: The current file offset in the file.
             SEEK_END: The end of the file.
 Returns:
   offs_t  >=0 on success. The value returned is the new file offset, measured
               in bytes from the beginning of the file.
           < 0 on fail.
 Caveats: lseek(fd, 0, SEEK_CUR) is equal to tell(fd).
 */

offs_t lseek(int32_t fd, offs_t offset, int32_t whence)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);


  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.lseek(GET_FD(fd), offset, whence)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return (offs_t) rtn;
  }

/*
 Name: fsync
 Desc:
 Params:
   - fd: The file descriptor whose current file offset you wanted to change.
 Returns:
   offs_t  =0 on success.
           <0 on fail.
 Caveats:
 */

int32_t fsync(int32_t fd)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);


  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.fsync(GET_FD(fd))))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: close
 Desc: Close a specific file.
 Params:
   - fd: The file descriptor to be closed.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t close(int32_t fd)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.close(GET_FD(fd));

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: closeall
 Desc: Close all files in a specific volume.
 Params:
   - vol: The volume name which all files are closed.
 Returns:
   int32_t  0 on success.
           -1 on fail.
  Caveats: Before the file is unlinked, it must be closed.
 */

int32_t closeall(const char *vol)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == vol)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(vol)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.closeall(vol_id);

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: unlink
 Desc: Remove a specific file.
 Params:
   - path: A pointer to the null-terminated path name of the file to be
           unlinked.
 Returns:
    int32_t  0 on success.
            -1 on fail.
 Caveats: Before the file removes, it must be closed. This function cann't
          remove a directory entry.
 */

int32_t unlink(const char *path)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.unlink(vol_id, &path[VOL_NAME_LEN]);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: truncate
 Desc: Truncate a specific file to a new size.
 Params:
   - fd: The descriptor of the file to which the length is to be changed.
   - new_size: The new length of the file in bytes.
 Returns:
    int32_t  0 on success.
            -1 on fail.
 Caveats: None.
 */

int32_t truncate(int32_t fd, size_t new_size)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);


  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.truncate(GET_FD(fd), new_size);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: tell
 Desc: Get the current position of a specific opend file.
 Params:
   - fd: The file descriptor whose current file offset you wanted to obtain.
 Returns:
   int32_t  >=0 on success. The value returned is the current file offset,
                 measured in bytes from the beginning of the file.
            < 0 on fail.
 Caveats: tell(fd) is equal to lseek(fd, 0, SEEK_CUR).
 */

int32_t tell(int32_t fd)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.tell(GET_FD(fd))))
    return -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: rename
 Desc: Change the name of a specific file or directory.
 Params:
   - oldpath: A pointer to the path name of the file or directory to be renamed.
   - newpath: A pointer to the new path name of the file or directory.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t rename(const char *oldpath, const char *newpath)
  {
  fsm_volinfo_t *fvi;
  int32_t dwOldVolID,
    dwNewVolID,
    rtn;


  if (NULL == oldpath || NULL == newpath)
    return set_errno(e_inval);

  if (0 > (dwOldVolID = __get_valid_vol_id(oldpath)))
    return -1;
  if (0 > (dwNewVolID = __get_valid_vol_id(newpath)))
    return -1;

  if (dwOldVolID != dwNewVolID)
    return set_errno(e_xdev);


  fvi = GET_FS_VOL(dwOldVolID);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.rename(dwOldVolID, &oldpath[VOL_NAME_LEN],
                           &newpath[VOL_NAME_LEN]);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: stat
 Desc: Get information in a specific file or directory by using a file path
       name.
 Params:
   - path: A pointer to the null-terminated path name of the file or directory
           obtain information.
   - statbuf: A pointer to a buffer of type struct stat_t where file
              status information is returned.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t stat(const char *path, stat_t *statbuf)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path || NULL == statbuf)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.stat(vol_id, &path[VOL_NAME_LEN], statbuf)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: fstat
 Desc: Get information in a specific file or directory by using a file
       descriptor.
 Params:
   - fd: A file descriptor of the file or directory to obtain information.
   - statbuf: A pointer to a buffer of type struct stat_t where file
              status information is returned.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t fstat(int32_t fd, stat_t *statbuf)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.fstat(GET_FD(fd), statbuf)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: getattr
 Desc: Get an attribute in a specific file or directory by using a file path
       name.
 Params:
   - path: A pointer to the null-terminated path name of the file or directory
           to obtain an attribute.
   - attr: A pointer to a buffer where an attribute is returned.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t getattr(const char *path, uint32_t *attrbuf)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path && NULL == attrbuf)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.getattr(vol_id, &path[VOL_NAME_LEN], attrbuf)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: fgetattr
 Desc: Get an attribute in a specific file or directory by using a file
       descriptor.
 Params:
   - fd: A file descriptor of the file or directory to obtain an attribute.
   - attr: A pointer to a buffer where an attribute is returned.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t fgetattr(int32_t fd, uint32_t *attrbuf)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.fgetattr(GET_FD(fd), attrbuf)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: setattr
 Desc: Set an attribute in a specific file or directory by using a file path
       name.
 Params:
   - path: A pointer to the null-terminated path name of the file or directory
           to obtain an attribute.
   - attr:  The attribute of file to be set. It is able to be the following
            symbols and be used with making up each other through OR operator.
            FA_RDONLY - Specifies a file for reading only.
            FA_HIDDEN - Specifies a hidden file.
            FA_SYSTEM - Specifies a system file.
            FA_ARCHIVE - Specifies a archive file.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t setattr(const char *path, uint32_t attr)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == path)
    return set_errno(e_inval);

  /* If the attribute of the file is in the read-only or hidden or system or
     archive, it can't set the attribute */
  if (0 == (attr & (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_ARCHIVE)))
    return set_errno(e_inval);
  attr &= (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_ARCHIVE);

  if (0 > (vol_id = __get_valid_vol_id(path)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.setattr(vol_id, &path[VOL_NAME_LEN], attr);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    return -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: fsetattr
 Desc: Set an attribute in a specific file or directory by using a file
       descriptor.
 Params:
   - fd: A file descriptor of the file or directory to set an attribute.
   - attr: The attribute of file to be set. It is able to be the following
            symbols and be used with making up each other through OR operator.
            FA_RDONLY - Specifies a file for reading only.
            FA_HIDDEN - Specifies a hidden file.
            FA_SYSTEM - Specifies a system file.
            FA_ARCHIVE - Specifies a archive file.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None.
 */

int32_t fsetattr(int32_t fd, uint32_t attr)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (!IS_CORRECT_FD(fd))
    return set_errno(e_badf);

  /* If the attribute of the file is in the read-only or hidden or system or
     archive, it can't set the attribute */
  if (0 == (attr & (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_ARCHIVE)))
    return set_errno(e_inval);
  attr &= (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_ARCHIVE);

  vol_id = GET_VOL(fd);
  if (0 > fd || VOLUME_NUM < vol_id)
    return set_errno(e_inval);

  fvi = GET_FS_VOL(vol_id);

  if (!(fvi->flag & FSM_MOUNTED))
    return set_errno(e_nomnt);

  if (0 > lock())
    return -1;

  rtn = fvi->fs->op.fsetattr(GET_FD(fd), attr);
  __fsm_mark_dirty(fvi);

  if (0 > rtn)
    return -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: sync
 Desc: Synchronize the cache data in file system and physical data.
 Params:
   - vol: The volume name of the file system which is to be synchronized.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None
 */

int32_t sync(const char *vol)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == vol)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(vol)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  if (0 > (rtn = fvi->fs->op.sync(vol_id)))
    rtn = -1;

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: statfs
 Desc: Get information of the mounted volume.
 Params:
   - vol: The volume name of the file system which is to obtain information.
   - statbuf: A pointer to a buffer of statfs_t struct type where file
              system status information is returned.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: None
 */

int32_t statfs(const char *vol, statfs_t *statbuf)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn;


  if (NULL == vol || NULL == statbuf)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_valid_vol_id(vol)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (0 > lock())
    return -1;

  statbuf->fs_id = fvi->fs->fs_id;
  rtn = fvi->fs->op.statfs(vol_id, statbuf);

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: ioctl
 Desc: Control I/O status.
 Params:
   - vol: The name of volume which is changed the I/O status.
   - func: The requested function. The function is one of the following symbols.
           IO_MS_ATTACH - Requests attach of a mass-storage.
           IO_MS_DETACH - Requests detach of a mass-storage.
           IO_DEV_ATTACH - Requests attach of a device.
           IO_DEV_DETACH - Requests detach of a device.
           IO_PART_INFO - Requests the partition's information.
           IO_OP - Requests the following operation: read, write, erase.
           IO_READ - Requests read function.
           IO_WRITE - Requests write function.
           IO_ERASE - Requests erase function.
   - param: Pointer to the parameter to be used by function.
 Returns:
   int32_t  0 on success.
           -1 on fail.
 Caveats: In this version, ioctl() function doesn't support. So, This
          function do nothing.
 */

int32_t ioctl(const char *vol, uint32_t func, void *param)
  {
  fsm_volinfo_t *fvi;
  int32_t vol_id,
    rtn = s_ok;


  if (NULL == vol)
    return set_errno(e_inval);

  if (0 > (vol_id = __get_vol_id(vol)))
    return -1;

  fvi = GET_FS_VOL(vol_id);

  if (IO_OP & func)
    {
    if (0 > __is_valid_vol(fvi))
      return -1;
    }

  /* Check whether ionFS wrote some data or not. If so, the mass-storage can't
     write anything in memory. */
  if (IO_WRITE == func && FSM_FS_DIRTY & fvi->flag)
    return s_ok;

  if (!(fvi->flag & FSM_MOUNTED))
    {
    set_errno(e_nomnt);
    return -1;
    }

  if (0 > lock())
    return -1;

  rtn = lim_ioctl(vol_id, func, param);

  switch (func)
    {
    case IO_MS_ATTACH:
      fvi->flag |= FSM_MS_ATTACH;
      fvi->flag &= ~(FSM_FS_DIRTY);
      break;
    case IO_MS_DETACH:
      fvi->flag &= ~(FSM_MS_ATTACH | FSM_MOUNTED);
      break;
    case IO_WRITE:
      fvi->flag |= FSM_MS_DIRTY;
      break;
    }

  if (0 > unlock())
    return -1;

  return rtn;
  }

/*
 Name: fsm_set_safe_mode
 Desc: Control the safe mode.
 Params:
   - issafe: the flag value for the safe mode.
             true - Set the safe mode on.
             false - Set the safe mode off.
 Returns:
   - boot_t  true means that the safe mode was set.
             false means that the safe mode wasn't set.
 Caveats: The purpose of safe mode is debugging. In the safe mode, the file
          system never use the resource of operating system.
 */

bool fsm_set_safe_mode(bool issafe)
  {
  return set_safe_mode(issafe);
  }

/*
 Name: get_sectors
 Desc: Get total sectors from a specific device.
 Params:
   - dev_id: An ID of the device to get a number of sectors from the memory.
 Returns:
   int32_t  >=0 on success. The value returned means a number of sectors in
                the memory.
            < 0 on fail.
 Caveats: None
 */

int32_t get_sectors(int32_t dev_id)
  {
  int32_t rtn;

  if (DEVICE_NUM <= dev_id)
    {
    set_errno(e_inval);
    return -1;
    }

  rtn = pim_open(dev_id);
  if (0 > rtn)
    return rtn;

  return pim_get_sectors(dev_id);
  }

/*
 Name: get_devicetype
 Desc: Get name of specific device.
 Params:
   - dev_id: An ID of the device to get a number of sectors from the memory.
   - name: Device type name
 Returns:
   int32_t  0 on success. The value returned means that ID's name is valid.
            < 0 on fail.
 Caveats: None.
 */

result_t get_devicetype(int32_t dev_id, char * name)
  {
  result_t rtn;

  if (DEVICE_NUM <= dev_id)
    {
    set_errno(e_inval);
    return -1;
    }

  rtn = pim_open(dev_id);
  if (0 > rtn)
    return rtn;

  return pim_get_devicetype(dev_id, name);
  }

/*
 Name: get_version
 Desc: Get version of the 'ionFS'.
 Params: None.
 Returns:
   fsver_t*  value on success. The pointer returned has information of current
             file system.
 Caveats: None.
 */

const fsver_t *get_version(void)
  {
  const static fsver_t vernum = {
                                 "SpansionFS",
                                 1,
                                 2,
                                 8
    };

  return &vernum;
  }
/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

