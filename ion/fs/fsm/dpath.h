/* FILE: ion_dpath.h */
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


#if !defined( DPATH_H_23122005 )
#define DPATH_H_23122005

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "../fsm/fsm.h"




/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

/* The structure for each element of the path parsed. */
typedef struct fsm_arg_s {
   char *argv[ALLPATH_ELEMENTS_MAX]; /* The array for each elements. */
   uint32_t argc;                            /* the number of path elements. */

} fsm_arg_t;




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

/* The maximum character of the ASCII code. */
#define MAX_ASCII_CHAR 0x7F

/* Define specific characters. */
#define DELIMITER_CHAR '~'
#define SPACE_CHAR ((char)' ')
#define DOT_CHAR ((char)'.')




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

int32_t fsm_parse_path( char *buf, fsm_arg_t *arg, const char *path );

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

