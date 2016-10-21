/* FILE: ion_name.c */
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

#include "name.h"
#include "../util/lib.h"
#include "misc.h"




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS & STRUCTURES
-----------------------------------------------------------------------------*/

/* Characters that are undesirable in an MS-DOS file name */
#define REPLACE_CHAR ((char)'_')
#define IS_REPLACE_CHAR(c) (((c)=='[') || ((c)==']') || ((c)==';') || \
                           ((c)==',') || ((c)=='+') || ((c)=='=') )

#define IS_REPLACE_UNICODE_CHAR(ch)  (((ch)==':') || ((ch)=='*') || ((ch)=='?') || \
                            ((ch)=='\"') || ((ch)=='<') || ((ch)=='>') || \
                            ((ch)=='|') )


#define RSV_NAME_MIN 3  /* The minimum number of reserved file name. */
#define RSV_NAME_MAX 4  /* The maximum number of reserved file name. */
/* Special file names in defined MS. */
static const uint8_t reserved_names[][RSV_NAME_MAX] = {
  {'C', 'O', 'N', '\0'},
  {'P', 'R', 'N', '\0'},
  {'N', 'U', 'L', '\0'},
  {'A', 'U', 'X', '\0'},
  {'C', 'O', 'M', '1'},
  {'C', 'O', 'M', '2'},
  {'C', 'O', 'M', '3'},
  {'C', 'O', 'M', '4'},
  {'C', 'O', 'M', '5'},
  {'C', 'O', 'M', '6'},
  {'C', 'O', 'M', '7'},
  {'C', 'O', 'M', '8'},
  {'C', 'O', 'M', '9'},
  {'L', 'P', 'T', '1'},
  {'L', 'P', 'T', '2'},
  {'L', 'P', 'T', '3'},
  {'L', 'P', 'T', '4'},
  {'L', 'P', 'T', '5'},
  {'L', 'P', 'T', '6'},
  {'L', 'P', 'T', '7'},
  {'L', 'P', 'T', '8'},
  {'L', 'P', 'T', '9'}
  };




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: fat_checksum_lfn_name
 Desc: Calculate the Chekchsum value of a directory name specified by the
       'dos_name' parameter.
 Params:
   - dos_name: The null-terminated string name.
 Returns:
   uint8_t   value on success. The returned value is the Checksum value about
                   a long file name.
 Caveats: None.
 */

uint8_t fat_checksum_lfn_name(uint8_t *dos_name)
  {
  uint8_t chksum, i;


  for(chksum = i = 0; i < FAT_SHORTNAME_SIZE; i++)
    chksum = (((chksum & 1) << 7) | (chksum >> 1)) + dos_name[i];

  return chksum;
  }

/*
 Name: __fat_calc_name
 Desc: Calculate the name of File-Entry. This function divide the File-Entry's
       base name into name and extension.
 Params:
   - fe: Pointer to fat_fileent_t structure which has the base file name.
   - ignore_chars: The number of ignored characters in the base file name.
   - p_len_name: Pointer to put the length of name except the extension in the
                 base name.                 .
   - p_len_ext: Pointer to put the length of extension in the base name.
   - p_is_short: Flag pointer to notify whether the base file name is short or
                 not. If the length of name is larger than 8 or the length of
                 extension is larger than 3, p_is_short will be false. If not,
                 it will be true.
   - p_is_8_3: Flag pointer to notify whether the base name conforms to 8.3
                 file name or not. If the base name conforms 8.3 filename,
                 p_is_8_3 will be true. If not, it will be false.
 Returns:
   char*  value If a dot character exists in the base file name, it returns
                  the extension string.
            NULL If a dot chracter doesn't exist, it returns NULL.
 Caveats: The MS-DOS FAT file system supports 8 characters for the base file
          name and 3 characters for the extension. This is known as an 8.3
          file name.
 */

static char *__fat_calc_name(fat_fileent_t *fe, uint16_t ignore_chars, uint16_t *p_len_name,
                             uint16_t *p_len_ext, bool *p_is_short, bool *p_is_8_3)
  {
  char *src, *dot;
  int16_t total_len, name_len, len_ext;
  bool is_short;


  dot = (char *) NULL;
  is_short = true;
  *p_is_8_3 = false;
  total_len = fe->name_len - ignore_chars;

  /* Search dot character from the end character of name. */
  src = &fe->name[fe->name_len];
  for(len_ext = 0; len_ext < total_len; len_ext++)
    {
    if(DOT_CHAR == *(--src))
      {
      dot = src;
      break;
      }
    }

  if(dot)
    /* If a dot character exists, calculate length of name again. */
    name_len = (int16_t) (dot - fe->name);
  else
    {
    len_ext = 0;
    name_len = total_len;
    }

  if(name_len > FAT_SHORTNAME_LEN)
    {
    name_len = FAT_SHORTNAME_LEN;
    is_short = false;
    }
  else if(FAT_SHORTEXT_LEN >= len_ext)
    *p_is_8_3 = true;

  if(len_ext > FAT_SHORTEXT_LEN)
    {
    len_ext = FAT_SHORTEXT_LEN;
    is_short = false;
    }


  *p_len_name = name_len;
  *p_len_ext = len_ext;
  *p_is_short = is_short;

  if(dot)
    return dot + 1/*for dot character*/;
  else
    return (char *) NULL;
  }

/*
 Name: __shortname_2_dosname
 Desc: Convert a short name  in a specific Directory-Entry to DOS-Format.
 Params:
   - fe: Pointer to fat_dirent_t structure which has a short name.
   - name: The null-terminated string name.
   - body_len: The length of name except the extension.
   - ext_len: The length of extension.
 Returns:
 Caveats: This function converts a short file name to DOS-Format. The length
          of file name is set to 8 characters by using space character, and
          the extension is set to 3 characters. For example, if the length of
          file name is 6, 8 minus 6 of rest space will fill with space character.
 */

static void __shortname_2_dosname(fat_dirent_t *de, uint8_t *name,
                                  uint32_t body_len, uint32_t ext_len)
  {
  uint32_t tmp;
  uint8_t *src8;


  memcpy(de->name, name, body_len);
  if(0 < (tmp = FAT_SHORTNAME_LEN - body_len))
    memset(de->name + body_len, SPACE_CHAR, tmp);

  if(ext_len)
    {
    src8 = name + body_len + 1/*dot character*/;
    memcpy(de->ext, src8, ext_len);
    if(0 < (tmp = FAT_SHORTEXT_LEN - ext_len))
      memset(de->ext + ext_len, SPACE_CHAR, tmp);
    }
  else
    memset(de->ext, SPACE_CHAR, FAT_SHORTEXT_LEN);
  }

/*
 Name: __fat_make_shortname
 Desc: Make a short name from the base name.
 Params:
   - fe: Pointer to file-entry which has the base name to make a short name.
   - short_name: String buffer of short name
   - p_is_rplc: Flag pointer to notify whether the name is replaced to replace-character
 Returns:
   bool  ture if the base name conforms 8.3 file name.
           false if the base name doesn't conforms 8.3 file name.
 Caveats: When you make a short name, space characters ignored. The short name
          always represents the uppercase.
 */

static bool __fat_make_shortname(fat_fileent_t *fe, uint8_t *short_name, bool *p_is_rplc)
  {
  char ch, upch, *src, *ext;
  bool is_short, is_8_3;
  uint8_t *dst;
  uint16_t name_len,
    len_ext,
    len_skip,
    base_case = 0,
    ext_case = 0,
    i;


  src = fe->name;
  for(/*ignore chars*/i = 0; i < fe->name_len; i++)
    if(!((SPACE_CHAR == src[i]) || (DOT_CHAR == src[i])))
      break;

  src += i;

  ext = __fat_calc_name(fe, i, &name_len, &len_ext, &is_short, &is_8_3);
#if defined( UNICODE )
  is_short = false;
#endif

  /* For making name */
  dst = short_name;

  *p_is_rplc = false;

  for(len_skip = i = 0; i < name_len; i++)
    {
    ch = *src++;

    if((SPACE_CHAR == ch) || (DOT_CHAR == ch))
      { /*skip characters*/
      is_short = false;
      len_skip++;
      continue;
      }

#if defined( UNICODE )
    if(!(ch & UINT8_MAX))
      {
      ch = ch >> BITS_PER_UINT8;
      }

    ch = ch & UINT8_MAX;
    if(IS_REPLACE_UNICODE_CHAR(ch) || SPACE_CHAR > ch)
      {
      *dst++ = REPLACE_CHAR;
      *p_is_rplc = true;
      continue;
      }
#endif

    if(IS_REPLACE_CHAR(ch))
      {
      *dst++ = REPLACE_CHAR;
      is_short = false;
      *p_is_rplc = true;
      continue;
      }

    /* If it isn't ASCII, regard the name as LFN */
    if(MAX_ASCII_CHAR < ch)
      is_short = false;

    upch = toupper(ch);

    if(is_short)
      {
      if(ch != upch) /* is lower? */
        base_case |= eFAT_CASE_LOWER;
      else if(isupper(ch))
        base_case |= eFAT_CASE_UPPER;
      }

    /* Change a character to an uppercase. */
    *dst++ = upch;
    }
  name_len -= len_skip;

  /* If dot character is found */
  if(ext)
    {
    /* make extent */
    src = ext;
    *dst++ = DOT_CHAR;

    for(len_skip = i = 0; i < len_ext; i++)
      {
      ch = *src++;

      if(SPACE_CHAR == ch)
        {
        is_short = false;
        len_skip++;
        continue;
        }

#if defined( UNICODE )
      if(!(ch & UINT8_MAX))
        {
        ch = ch >> BITS_PER_UINT8;
        }

      ch = ch & UINT8_MAX;
      if(IS_REPLACE_UNICODE_CHAR(ch) || SPACE_CHAR > ch)
        {
        *dst++ = REPLACE_CHAR;
        *p_is_rplc = true;
        continue;
        }
#endif

      if(IS_REPLACE_CHAR(ch))
        {
        *dst++ = REPLACE_CHAR;
        is_short = false;
        *p_is_rplc = true;
        continue;
        }

      /* If it isn't ASCII, regard the name as LFN */
      if(MAX_ASCII_CHAR < ch)
        is_short = false;

      upch = toupper(ch);

      if(ch != upch) /* is lower? */
        ext_case |= eFAT_CASE_LOWER;
      else if(isupper(ch))
        ext_case |= eFAT_CASE_UPPER;

      /* Change a character to an uppercase. */
      *dst++ = upch;
      }
    len_ext -= len_skip;
    }
  *dst = '\0';

  if(is_short)
    {
    if(((eFAT_CASE_UPPER & base_case) && (eFAT_CASE_LOWER & base_case)) ||
       ((eFAT_CASE_UPPER & ext_case) && (eFAT_CASE_LOWER & ext_case)))
      is_short = false;
    }

#ifdef SFILEMODE
  fe->dir.char_case &= ~(eFAT_CASE_LOWER_BASE | eFAT_CASE_LOWER_EXT);
#else
  fe->dir.char_case = 0;
#endif
  if(is_short)
    {
    if(eFAT_CASE_LOWER == base_case)
      fe->dir.char_case |= eFAT_CASE_LOWER_BASE;
    if(eFAT_CASE_LOWER == ext_case)
      fe->dir.char_case |= eFAT_CASE_LOWER_EXT;
    fe->flag &= ~(uint32_t) eFILE_LONGENTRY;
    }
  else
    fe->flag |= (uint32_t) eFILE_LONGENTRY;

  /* Chanage the short name to dos format */
  __shortname_2_dosname(&fe->dir, short_name, name_len, len_ext);

  return is_8_3;
  }

/*
 Name: __is_reserved_name
 Desc: Check whether a specific name is reserved or not.
 Params:
   - name: Pointer to null-terminated name to be checked.
 Returns:
   bool  true if found.
           false if not found.
 Caveats: Refer to a global variable, 'reserved_names' for the reserved name.
 */

static bool __is_reserved_name(uint8_t *name)
  {
  uint32_t i, j;

  for(i = 0; i < sizeof (reserved_names) / sizeof (reserved_names[0]); i++)
    {
    for(j = 0; j < RSV_NAME_MAX; j++)
      {
      if(reserved_names[i][j] != name[j])
        break;
      }
    if(RSV_NAME_MAX == j)
      return true;
    }

  return false;
  }

size_t lstrcpy(char *dest, const char *src, size_t max)
  {
  size_t len;

  for(len = 0; len < max; len++)
    if((*dest++ = *src++) == 0)
      break;

  return len;
  }

/*
 Name: fat_cp_name
 Desc: Write a specific name into File-Entry's name.
 Params:
   - fe: Pointer to fat_fileent_t structure.
   - name: Null-terminated string name to copy.
   - ent_type: Type of File-Entry. The value is one of the following symbols.
               eFAT_FILE: indicates a file entry.
               eFAT_DIR: indicates a directory entry.
               According to type of File-Entry, the length name to be copy
               changes.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: If 'ent_type' parameter is eFAT_ALL, it regards as eFAT_FILE.
 */

int32_t fat_cp_name(fat_fileent_t *fe, char *name, int32_t ent_type)
  {
  int32_t max_len;


  max_len = (eFAT_FILE & ent_type) ? FAT_FILENAME_MAX_LEN : FAT_DIRNAME_MAX_LEN;

  /* Setup a long name */
  fe->name_len = (uint16_t) lstrcpy(fe->name, name, FAT_LONGNAME_SIZE + 1);
  fe->name[fe->name_len] = '\0';

  if(fe->name_len > max_len)
    return set_errno(e_name_too_long);

  return s_ok;
  }

/*
 Name: __fat_adjust_japanese_char
 Desc: Japanese character(0xE5) convert to 0x05.
       0x05 convert to japanese character(0xE5).
 Params:
   - byte: Pointer of byte to be adjusted.
 Returns: None.
 Caveats: None.
 */

static void __fat_adjust_japanese_char(uint8_t * byte)
  {
  if(eFAT_DELETED_FLAG == *byte)
    *byte = eFAT_REPLACE_JAPAN;
  else if(eFAT_REPLACE_JAPAN == *byte)
    *byte = eFAT_DELETED_FLAG;
  }

/*
 Name: fat_make_shortname
 Desc: Make a short name and check if the base name is reserved name.
 Params:
   - fe: Pointer to file-entry which has the base name.
   - p_is_8_3: Flag pointer to notify whether the base name conforms to 8.3
               file name or not. If the base name conforms 8.3 filename,
               p_is_8_3 will be true. If not, it will be false.
   - p_is_rplc: Flag pointer to notify whether the name is replaced to replace-character
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: The MS-DOS FAT file system supports 8 characters for the base file
          name and 3 characters for the extension. This is known as an 8.3
          file name.
          If the base name is reserved, this function returns -1 and set
          e_path error.
 */

int32_t fat_make_shortname(fat_fileent_t *fe, bool *p_is_8_3, bool *p_is_rplc)
  {
  uint8_t short_name[FAT_SHORTNAME_SIZE + 1/*dot char*/ + 1/*null char*/];


  *p_is_8_3 = __fat_make_shortname(fe, short_name, p_is_rplc);

  __fat_adjust_japanese_char(&fe->dir.name[0]);

  if(RSV_NAME_MAX >= fe->name_len && RSV_NAME_MIN <= fe->name_len)
    if(true == __is_reserved_name(short_name))
      return set_errno(e_path);

  return s_ok;
  }

/*
 Name: fat_make_lfn_name_entry
 Desc: Set information of lfn_nameent_t structure. lfn_nameent_t structure has
       informations for long file name.
 Params:
   - ne: Pointer to lfn_nameent_t structure to be set.
   - name: Pointer to the null-terminated path name.
 Returns: None.
 Caveats: None.
 */

void fat_make_lfn_name_entry(lfn_nameent_t *ne, char *name)
  {
  uint16_t *dst,
    len,
    entries,
    i, j;


  ne->skip_ents = 0;
  len = (uint16_t) strlen(name);
  entries = ne->entries = len / LFN_NAME_CHARS;

  /* fill util index #1 */
  for(i = 1; i < entries; i++)
    for(dst = ne->name[entries - i], j = 0; j < LFN_NAME_CHARS; j++)
      *dst++ = *name++;

  /* fill index #0 */
  for(dst = ne->name[0], i = 0; i < LFN_NAME_CHARS; i++)
    {
    if('\0' == (*dst++ = *name++))
      {
      i++;
      break;
      }
    }

  /* fill the characters after '\0' of index 0 to LFN_NAME_DEFAULT_CHAR */
  for(/* no init */; i < LFN_NAME_CHARS; i++)
    *dst++ = LFN_NAME_DEFAULT_CHAR;
  }

/*
 Name: fat_get_short_index
 Desc: Get the index of short name which is a alias of long name.
 Params:
   - name: The null-terminated string which means a short name.
   - cmp_name: The null-terminated string buffer to get a real file name in a
               short name.
 Returns:
   int32_t  value The return value means the index of short name.
 Caveats: None.
 */

int32_t fat_get_short_index(const uint8_t *name, const uint8_t *cmp_name)
  {
  int32_t deli_pos, /* delimiter position */
    num;
  const uint8_t *p, *pe;


  p = name + (FAT_SHORTNAME_LEN - 1/*start index 0*/);

  while(SPACE_CHAR == *p)
    p--;

  pe = p;
  while(isdigit(*p))
    p--;

  if((pe == p) || (DELIMITER_CHAR != *p))
    return 0;

  deli_pos = (int32_t) (p - name);
  if(memcmp(cmp_name, name, deli_pos))
    return 0;

  for(num = 0, p++; p <= pe; p++)
    num = (num * 10) + (*p - '0');

  return num;
  }

/*
 Name: fat_set_short_index
 Desc: Set the index of short name which is a alias of long name.
 Params:
   - name: The null-terminated string which means a short name.
   - num: The index number of short name to be set.
 Returns: None.
 Caveats: When creating the Short File Name, which a alias is a Long File Name,
          Fat File System defines to put the last second string of Short File
          Name¡®~¡¯character then a index value. This function is for set the
          index value.
 */

void fat_set_short_index(uint8_t *name, uint32_t num)
  {
  uint32_t cipher, /* number of figures */
    tmp,
    space_len, /* the number of space-characters */
    i;
  uint8_t *p;


  tmp = num;
  cipher = 0;

  while(0 != tmp)
    {
    tmp /= 10;
    cipher++;
    }

  p = (name + (FAT_SHORTNAME_LEN - 1/*start idx 0*/)/*last character*/) - cipher;
  space_len = 0;

  while(p > name)
    {
    if(SPACE_CHAR == *p)
      space_len++;
    else
      {
      if(space_len) p++;
      break;
      }
    p--;
    }

  *p = DELIMITER_CHAR;
  p += cipher;

  for(i = 0, tmp = num; i < cipher; i++)
    {
    *p-- = (uint8_t) ((tmp % 10) + '0');
    tmp = (tmp / 10);
    }
  }

/*
 Name: fat_parse_lfn_name
 Desc: This function converts long file name in fat_lfnent_t structure to a
       string format.
 Params:
   - lfn: Pointer to fat_lfnent_t structure which have a long file name.
   - name: String buffer where a name is writting.
 Returns:
   int32_t  LFN_NAME_CHARS always.
 Caveats: The returned value is always LFN_NAME_CHARS. It means the length of
          a long file name conformed from FAT File System.

 */

int32_t fat_parse_lfn_name(fat_lfnent_t *lfn, char *name)
  {
  char *dst = name;
  uint8_t *s8, i;


  for(s8 = lfn->name0_4, i = 0; i < LFN_NAME_CHARS_0_4; i++, s8 += LFN_CHAR_SIZE)
    *dst++ = (char) ARR8_2_UINT16(s8);
  for(s8 = lfn->name5_10, i = 0; i < LFN_NAME_CHARS_5_10; i++, s8 += LFN_CHAR_SIZE)
    *dst++ = (char) ARR8_2_UINT16(s8);
  for(s8 = lfn->name11_12, i = 0; i < LFN_NAME_CHARS_11_12; i++, s8 += LFN_CHAR_SIZE)
    *dst++ = (char) ARR8_2_UINT16(s8);

  return LFN_NAME_CHARS;
  }

/*
 Name: fat_cmp_lfn_entry
 Desc: Compare two long names
 Params:
   - lfn: Psoint to fat_lfnent_t structure included name to be compared.
   - ne: Psoint to lfn_nameent_t structure included name to be compared.
   - ne_idx: Index of name entry to be compared.
 Returns:
   bool  true if two names are the same.
           false if two names are different each other.
 Caveats: None.
 */

bool fat_cmp_lfn_entry(fat_lfnent_t *lfn, lfn_nameent_t *ne, uint32_t ne_idx)
  {
  uint8_t *s8;
  uint16_t *s16, c1, c2;
  int32_t i;


  /* is same index? */
  if((FAT_LFN_IDX_MASK & lfn->idx) != (ne->entries - ne_idx))
    return false;

  s16 = ne->name[ne_idx];

  for(s8 = lfn->name0_4, i = 0; i < LFN_NAME_CHARS_0_4; i++, s8 += LFN_CHAR_SIZE)
    {
    c1 = *s16++;
    c2 = ARR8_2_UINT16(s8);
    if(tolower(c1) != tolower(c2))
      return false;
    }
  for(s8 = lfn->name5_10, i = 0; i < LFN_NAME_CHARS_5_10; i++, s8 += LFN_CHAR_SIZE)
    {
    c1 = *s16++;
    c2 = ARR8_2_UINT16(s8);
    if(tolower(c1) != tolower(c2))
      return false;
    }
  for(s8 = lfn->name11_12, i = 0; i < LFN_NAME_CHARS_11_12; i++, s8 += LFN_CHAR_SIZE)
    {
    c1 = *s16++;
    c2 = ARR8_2_UINT16(s8);
    if(tolower(c1) != tolower(c2))
      return false;
    }

  return true;
  }

/*
 Name: fat_cp_shortname
 Desc: Copy a name and extension in a specific Directory Entry to a string.
 Params:
   - name: The null-terminated string name.
   - de: Pointer to fat_dirent_t structure.
 Returns:
   char*  value on success. The returned value is the string which means
                  a name.
 Caveats: None.
 */

char * fat_cp_shortname(char *name, fat_dirent_t *de)
  {
  uint8_t *p;
  int32_t i;

  /* copy 8 characters */
  p = de->name;
  if(eFAT_CASE_LOWER_BASE & de->char_case)
    for(i = 0; i < FAT_SHORTNAME_LEN; i++, p++)
      *name++ = tolower(*p);
  else
    for(i = 0; i < FAT_SHORTNAME_LEN; i++)
      *name++ = *p++;

  for(i = 0; i < FAT_SHORTNAME_LEN; i++)
    if(SPACE_CHAR == *(--p))
      name--;


  /* copy 3 characters */
  p = de->ext;

  if(SPACE_CHAR != *p)
    *name++ = DOT_CHAR;

  if(eFAT_CASE_LOWER_EXT & de->char_case)
    for(i = 0; i < FAT_SHORTEXT_LEN; i++, p++)
      *name++ = tolower(*p);
  else
    for(i = 0; i < FAT_SHORTEXT_LEN; i++)
      *name++ = *p++;

  for(i = 0; i < FAT_SHORTEXT_LEN; i++)
    if(SPACE_CHAR == *(--p))
      name--;

  *name = '\0';

  return name;
  }

/*
 Name: fat_lfn_2_name
 Desc: Convert LFN format specified by 'lfn' parameter to one string .
 Params:
   - name: The null-terminated string name to be filled.
   - lfn: Pointer to fat_lfnent_t structure which has information of long name.
 Returns:
   char*  value on success. The returned value means the address of remained
                  string after converting.
 Caveats: None.
 */

char *fat_lfn_2_name(char *name, fat_lfnent_t *lfn)
  {
  uint8_t *src;
  char *dst = name;
  int32_t i;


  for(src = lfn->name0_4, i = 0; i < LFN_NAME_CHARS_0_4; i++, src += LFN_CHAR_SIZE)
    *dst++ = (char) ARR8_2_UINT16(src);
  for(src = lfn->name5_10, i = 0; i < LFN_NAME_CHARS_5_10; i++, src += LFN_CHAR_SIZE)
    *dst++ = (char) ARR8_2_UINT16(src);
  for(src = lfn->name11_12, i = 0; i < LFN_NAME_CHARS_11_12; i++, src += LFN_CHAR_SIZE)
    *dst++ = (char) ARR8_2_UINT16(src);

  return name - LFN_NAME_CHARS;
  }

/*
 Name: fat_name_2_lfn
 Desc: Convert file name specified by 'name' parameter to the LFN format.
 Params:
   - lfn: Pointer to fat_lfnent_t structure which has information of long name.
   - name: The null-terminated string name.
 Returns:
   char*  value on success. The returned value means the address of remained
                  string after converting.
 Caveats: The maximum characters of name in a LFN entry is 13
         (defined by FAT File System).
 */

char *fat_name_2_lfn(fat_lfnent_t *lfn, char *name)
  {
  uint8_t *dst;
  char *src = name;
  int32_t i;


  for(dst = lfn->name0_4, i = 0; i < LFN_NAME_CHARS_0_4; i++, dst += LFN_CHAR_SIZE, src++)
    UINT16_2_ARR8(dst, *src);
  for(dst = lfn->name5_10, i = 0; i < LFN_NAME_CHARS_5_10; i++, dst += LFN_CHAR_SIZE, src++)
    UINT16_2_ARR8(dst, *src);
  for(dst = lfn->name11_12, i = 0; i < LFN_NAME_CHARS_11_12; i++, dst += LFN_CHAR_SIZE, src++)
    UINT16_2_ARR8(dst, *src);

  return name - LFN_NAME_CHARS;
  }

/*
 Name: fat_name_2_last_lfn
 Desc: Convert the file name specified by 'name' parameter to the LFN format
       when given LFN entry is the last. If the length of name is smaller than
       long file name which can fill up in the LFN Entry, the LFN Entry fills
       with the name first and 0xFFFF value in the rest.
 Params:
   - lfn: Pointer to fat_lfnent_t structure which has information of long name.
   - name: The null-terminated string name.
 Returns:
   char*  value on success. The returned value means the address of remained
                  string after converting.
 Caveats: The maximum characters of name in a LFN entry is 13
         (defined by FAT File System).
 */

char *fat_name_2_last_lfn(fat_lfnent_t *lfn, char *name, uint32_t name_len)
  {
  uint8_t *dst;
  char *src = name;
  uint32_t i,
    len = 0;


  for(dst = lfn->name0_4, i = 0; i < LFN_NAME_CHARS_0_4; i++, dst += LFN_CHAR_SIZE)
    if(len++ <= name_len)
      {
      UINT16_2_ARR8(dst, *src);
      src++;
      }
    else
      UINT16_2_ARR8(dst, LFN_NAME_DEFAULT_CHAR);
  for(dst = lfn->name5_10, i = 0; i < LFN_NAME_CHARS_5_10; i++, dst += LFN_CHAR_SIZE)
    if(len++ <= name_len)
      {
      UINT16_2_ARR8(dst, *src);
      src++;
      }
    else
      UINT16_2_ARR8(dst, LFN_NAME_DEFAULT_CHAR);
  for(dst = lfn->name11_12, i = 0; i < LFN_NAME_CHARS_11_12; i++, dst += LFN_CHAR_SIZE)
    if(len++ <= name_len)
      {
      UINT16_2_ARR8(dst, *src);
      src++;
      }
    else
      UINT16_2_ARR8(dst, LFN_NAME_DEFAULT_CHAR);

  return name - LFN_NAME_CHARS;
  }

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

