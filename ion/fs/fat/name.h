/* FILE: ion_name.h */
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


#if !defined( FAT_NAME_H_29122005 )
#define FAT_NAME_H_29122005

/*-----------------------------------------------------------------------------
 INLCUDE FUNCTIONS
-----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "fat.h"
#include "dir.h"
#include "file.h"




/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

/* Structure for long file name entry. */
typedef struct lfn_nameent_s {
   uint16_t skip_ents;  /* In new sector, the number of entries to skip. Bcs, previous entry was LFN. */
   uint16_t entries;    /* LFN Record Sequence Number */
   uint16_t name[FAT_LONGNAME_SLOTS][LFN_NAME_CHARS];

} lfn_nameent_t;




#if 0
/*
Name: str_has_tilde
Desc:
Params:
Returns:
Caveats:
*/

inline bool str_has_tilde( char *str )
{
   while ( *str ) {
      if ( '~' == *str++ )
         return true;
   }
   return false;
}
#endif




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

uint8_t fat_checksum_lfn_name( uint8_t *dos_name );
int32_t fat_cp_name( fat_fileent_t *fe, char *name, int32_t ent_type );
int32_t fat_make_shortname( fat_fileent_t *fe, bool *p_is_8_3, bool *p_is_rplc );
int32_t fat_get_short_index( const uint8_t *name, const uint8_t *cmp_name );
void fat_set_short_index( uint8_t *name, uint32_t num );
int32_t fat_parse_lfn_name( fat_lfnent_t *lfn, char *name );
bool fat_cmp_lfn_entry( fat_lfnent_t *lfn, lfn_nameent_t *ne, uint32_t ne_idx );
void fat_make_lfn_name_entry( lfn_nameent_t *ne,  char *name );
char *fat_cp_shortname( char *name, fat_dirent_t *de );
char *fat_lfn_2_name( char *name, fat_lfnent_t *lfn );
char *fat_name_2_lfn( fat_lfnent_t *lfn, char *name );
char *fat_name_2_last_lfn( fat_lfnent_t *lfn, char *name, uint32_t len );

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

