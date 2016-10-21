/* FILE: ion_misc.c */
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

#include "misc.h"
#include "../fat/path.h"




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: fat_ftime_2_gtime
 Desc: Convert fat time into general time.
 Params:
   - ctime: Pointer to time which a file was created.
   - atime: Pointer to time which a file accessed recently.
   - wtime: Pointer to time which a file wrote recently.
   - de: Pointer to entry which has information of time.
 Returns: None.
 Caveats: None.
 */

void fat_ftime_2_gtime(uint32_t *ctime, uint32_t *atime, uint32_t *wtime, fat_dirent_t *de)
  {
  *ctime = de->ctime | (de->cdate << 16);
  *atime = de->adate;
  *wtime = de->wtime | (de->wdate << 16);
  }

/*
 Name: fat_gtime_2_ftime
 Desc: Convert general time into fat time.
 Params:
   - de: Pointer to entry which has information of time.
   - ctime: Pointer to time which a file was created.
   - atime: Pointer to time which a file accessed recently.
   - wtime: Pointer to time which a file wrote recently.
 Returns: None.
 Caveats: None.
 */

void fat_gtime_2_ftime(fat_dirent_t *de, uint32_t *ctime, uint32_t *atime, uint32_t *wtime)
  {
  de->ctime = (uint16_t) * ctime;
  de->cdate = (uint16_t) (*ctime >> 16);
  de->adate = (uint16_t) * atime;
  de->wtime = (uint16_t) * wtime;
  de->wdate = (uint16_t) (*wtime >> 16);
  }

/*
 Name: fat_set_ent_time
 Desc: Set the time of a Directory-Entry.
 Params:
   - de: Pointer to Directory-Entry which will be set.
   - type: A type of time to be set. The type is bitwise OR of the following
           symbol.
           TM_CREAT_ENT - the time when a file is created.
           TM_WRITE_ENT - the time when a file is written.
           TM_ACCESS_ENT - the time when a file is accessed.
           TM_ALL_ENT - includes creation time, writing time, access time.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t fat_set_ent_time(fat_dirent_t *de, uint32_t type)
  {
  tm_t *lt; /*local time*/
  uint16_t fdate;
  uint16_t ftime;
  time_t now = time(0);

  /* Get the current time from OS. */
  lt = localtime(&now);

  de->ctime_tenth = (uint8_t) (lt->tm_sec % 200);

  fdate = (uint16_t) lt->tm_mday;
  fdate |= (lt->tm_mon + 1) << 5;
  fdate |= (lt->tm_year + 1900 - 1980/*MS-DOS EPOCH*/) << 9;

  ftime = (uint16_t) (lt->tm_sec >> 1);
  ftime |= lt->tm_min << 5;
  ftime |= lt->tm_hour << 11;

  /* Update time determined by  the input type. */
  if(TM_CREAT_ENT & type)
    {
    de->cdate = fdate;
    de->ctime = ftime;
    }

  if(TM_WRITE_ENT & type)
    {
    de->wdate = fdate;
    de->wtime = ftime;
    }

  if(TM_ACCESS_ENT & type)
    de->adate = fdate;

  return s_ok;
  }

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

