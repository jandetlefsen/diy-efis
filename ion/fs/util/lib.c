/* FILE: ion_lib.c */
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

#include "../global/ion_global.h"




/*-----------------------------------------------------------------------------
 DEFINE GLOBAL VARIABLES
-----------------------------------------------------------------------------*/

static const uint32_t rand_multiplier =  0x015a4e35L;
static uint32_t rand_seed = 0x015a4e35L;
static const uint32_t rand_rncrement = 1;





/*-----------------------------------------------------------------------------
 DEFINE FUNTIONS
-----------------------------------------------------------------------------*/

/*
 Name: lib_rand
 Desc: Get the random value.
 Params: None.
 Returns:
   uint32_t  >0 Always. The value returned is a random value.
 Caveats: None.
*/

uint32_t lib_rand( void )
{
   rand_seed = (rand_seed * rand_multiplier) + rand_rncrement;
   return rand_seed;
}




/*
 Name: lib_srand
 Desc: Before getting a random value, set a based value.
 Params:
   - seed: The based value for getting a random value.
 Returns: None.
 Caveats: None.
*/

void lib_srand( uint32_t seed )
{
   rand_seed = seed;
   lib_rand();
}




/*
 Name: lib_memcpy
 Desc: Copies characters between buffers.
 Params:
   - dst: New buffer which copies chracters.
   - src: Buffer to copy from
   - len: Number of characters to copy.
 Returns: None.
 Caveats: None.
*/

void lib_memcpy( void *dst, const void *src, uint32_t len )
{
   uint8_t *s1 = (uint8_t *) dst;
   const uint8_t *s2 = (const uint8_t *) src;

   while ( len-- )
      *s1++ = *s2++;
}




/*
 Name: lib_memcpy8_16
 Desc: Characters changes the 8-bit characters with the 16-bit characters.
       & Copies the characters.
 Params:
   - dst: New buffer which copies chracters.
   - src: Buffer to copy from
   - len: Number of characters to copy.
 Returns: None.
 Caveats: None.
*/

void lib_memcpy8_16( uint8_t *dst, uint16_t *src, uint32_t len16 )
{
   while ( len16-- )
      *dst++ = (uint8_t) *src++;
}




/*
 Name: lib_memcpy16
 Desc: Copies 16-bit characters between buffers.
 Params:
   - dst: New buffer which copies chracters.
   - src: Buffer to copy from
   - len: Number of characters to copy.
 Returns: None.
 Caveats: None.
*/

void lib_memcpy16( uint16_t *dst, uint16_t *src, uint32_t len16 )
{
   while ( len16-- )
      *dst++ = (uint8_t) *src++;
}




/*
 Name: lib_memset
 Desc: Sets buffers to a specified character.
 Params:
   - dst: Pointer to destination.
   - src: Character to set.
   - len: Number of characters.
 Returns: None.
 Caveats: None.
*/

void lib_memset( void *dst, uint8_t byte, uint32_t len )
{
   uint8_t *s1 = (uint8_t *) dst;

   while ( len-- )
      *s1++ = byte;
}




/*
 Name: lib_memset16
 Desc: Sets buffers to a specified 16-bit character.
 Params:
   - dst: Pointer to destination.
   - word: Character to set.
   - len16: Number of characters.
 Returns: None.
 Caveats: None.
*/

void lib_memset16( void *dst, uint16_t word, uint32_t len16 )
{
   uint16_t *s1 = (uint16_t *) dst;

   while ( len16-- )
      *s1++ = word;
}




/*
 Name: lib_memset32
 Desc: Sets buffers to a specified 32-bit character.
 Params:
   - dst: Pointer to destination.
   - dword: Character to set.
   - len32: Number of characters.
 Returns: None.
 Caveats: None.
*/

void lib_memset32( void *dst, uint32_t dword, uint32_t len32 )
{
   uint32_t *s1 = (uint32_t *) dst;

   while ( len32-- )
      *s1++ = dword;
}




/*
 Name: lib_memcmp
 Desc: Compare characters in two buffers.
 Params:
   - s1: First buffer.
   - s2: Second buffer.
   - len: Number of characters.
 Returns:
   uint32_t  <0 It means that buf1 is less than buf2.
             =0 It means that buf1 is equal to buf2.
             >0 It means that buf1 greater than buf2.
 Caveats: None.
*/

uint32_t lib_memcmp( const void *s1, const void *s2, uint32_t len )
{
   uint8_t result;
   const uint8_t *t1 = (const uint8_t *) s1;
   const uint8_t *t2 = (const uint8_t *) s2;

   while ( len-- )
      if ( (result = *t1++ - *t2++) != 0 )
         return result;

   return 0;
}




/*
 Name: lib_data32cmp
 Desc: Find 32-bit data from buffer.
 Params:
   - p: Null-terminated string to search.
   - data: 32-bit data to search for.
   - len32: Number of characters to find data.
 Returns:
   uint32_t  value the buffer pointer found.
             0     Not found.
 Caveats: None.
*/

uint32_t lib_data32cmp( const uint32_t *p, uint32_t data, uint32_t len32 )
{
   uint32_t i;

   for ( i = 0; i < len32; i++ ) {
      if ( data != *p )
         return (uint32_t) p;
      p++;
   }
   return 0;
}




/*
 Name: lib_strcat
 Desc: Append a string.
 Params:
   - dst: Null-terminated destination string.
   - src: Null-terminated source string.
 Returns:
   uint32_t  value returns the destination string.
 Caveats: None.
*/

uint32_t lib_strcat( char *dst, const char *src )
{
   char *d = dst;

   while ( *d )
      d++;

   while ( 0 != (*d++ = *src++) )
      ;

   return (uint32_t) dst;
}




/*
 Name: lib_strcmp
 Desc: Compare two strings.
 Params:
 - s1, s2: Null-terminated strings to compare.
 Returns:
   uint32_t  value The return value for each of these functions indicates the
                   lexicographic relation of s1 to s2.
 Caveats: None.
*/

uint32_t lib_strcmp( const char *s1, const char *s2 )
{
   uint32_t   result;

   while ( ( result = *s1 -*s2++ ) == 0 && *s1++ )
      ;

   return result;
}




/*
 Name: lib_stricmp
 Desc: Perform a lowercase comparison of strings.
 Params:
 - s1, s2: Null-terminated strings to compare.
 Returns:
   uint32_t  <0 s1 less than s2.
             =0 s1 identical to s2.
             >0 s1 greater than s2.
 Caveats: None.
*/

uint32_t lib_stricmp( const char *s1, const char *s2 )
{
   uint32_t   result;

   while ( ( result = lib_tolower( *s1 ) - lib_tolower( *s2 ) ) == 0 && *s1++ )
      s2++;

   return result;
}




/*
 Name: lib_strlen
 Desc: Get the length of a string.
 Params:
   - str: Null-terminated string.
 Returns:
   uint32_t >0 the length of a string.
 Caveats: This function returns the number of characters in string, excluding
          the terminal NULL.
*/

uint32_t lib_strlen( const char *str )
{
   uint32_t len;

   for ( len = 0; *str++; len++ )
      ;
   return len;
}




/*
 Name: lib_lstrcpy
 Desc: Copies a string to a buffer.
 Params:
   - dst: Pointer to a buffer to receive the contents of the string pointed to
          by the src parameter. The buffer must be large enough to contain the
          string, including the terminating null character.
   - src: Pointer to the null-terminated string to be copied.
 Returns:
   int32_t value The return value is the length of copied.
 Caveats: None.
*/

int32_t lib_lstrcpy( char *dst, const char *src )
{
   const char *ori_dst = dst;

   while ( (*dst++ = *src++ ) != 0 )
      ;

   return (int32_t)(dst-ori_dst-1/*'\0'*/);
}

#if 0
int32_t lib_lstrcpy( char *dst, const char *src )
{
   const char *ori_dst = dst;

   while ( (*dst++ = *src++ ) != 0 )
      ;

#if defined (UINCODE)
   return (int32_t)((dst-ori_dst-1)>>1/*'\0'*/);
#else
   return (int32_t)(dst-ori_dst-1/*'\0'*/);
#endif
}

#endif


/*
 Name: lib_lstrncpy
 Desc: Copies a string to a buffer.
 Params:
   - dst: Pointer to a buffer to receive the contents of the string pointed to
          by the src parameter. The buffer must be large enough to contain the
          string, including the terminating null character.
   - src: Pointer to the null-terminated string to be copied.
   - len: Specifies the number of char values to be copied from the string
          pointed to by src into the buffer pointed to by dst, including a
          terminating null character.
 Returns: length of copied
 Caveats: None.
*/

int32_t lib_lstrncpy( char *dst, const char *src, int32_t len )
{
   const char *ori_dst = dst;

   while ( len ) {
      len--;

      if ( 0 == (*dst = *src) )
         break;
      dst++;
      src++;
   }

   return (int32_t)(dst-ori_dst);
}




/*
 Name: lib_strncpy
 Desc: Copy characters of one string to another.
 Params:
   - dst: Destination string.
   - src: Source string.
   - count: Number of characters to be copied.
 Returns:
   char*  value The return value is destination string.
 Caveats: None.
*/

char *lib_strncpy( char *dst, const char *src, uint32_t len )
{
   char *d = dst;

   while ( len ) {
      len--;

      if ( ( *d++ = *src++ ) == 0 )
         break;
   }

   return dst;
}




/*
 Name: lib_strchr
 Desc: Find a character in a string.
 Params:
   - src: Null-terminated source string
   - ch: Character to be located
 Returns:
   void*  value on success. The return value is a pointer to the first
                occurrence of ch in string.
          NULL on fail. The ch in string isn't found.
 Caveats: None.
*/

const void *lib_strchr( const char *src, char ch )
{
   while ( *src )
      if ( *src++ == ch ) return src;

   return NULL;
}



/*
 Name: lib_strrchr
 Desc: Scan a string for the last occurrence of a character.
 Params:
   - src: Null-terminated string to search.
   - ch: Character to be located.
 Returns:
   void*  value The return value a pointer to the last occurrence of ch in
                string,
          NULL  if ch is not found.
 Caveats: None.
*/

const void *lib_strrchr( const char *src, char ch )
{
   uint32_t len = lib_strlen( src );
   const char * p = src + len;

   while ( p != src )
      if ( *p-- == ch ) return ++p;

   return NULL;
}



/*
 Name: lib_strnchr
 Desc: Scan a string for the last occurrence of a character.
 Params:
   - src: Null-terminated string to search.
   - ch: Character to be located.
   - n: Number of characters to scaned.
 Returns:
   void*  value The return value a pointer to the last occurrence of ch in
                string,
          NULL  if ch is not found.
 Caveats: None.
*/

const void *lib_strnchr( const char *src, const char ch, int n )
{
   while ( *src && n--)
      if ( *src++ == ch ) return src;

   return NULL;
}




/*
 Name: lib_wcscat
 Desc: Append a string.
 Params:
   - dst: Null-terminated destination string (string of 16-bit unit).
   - src: Null-terminated source string (string of 16-bit unit).
 Returns:
   uint32_t  >0 always. The return value is the destination string.
 Caveats: None.
*/

uint32_t lib_wcscat( uint16_t *dst, const uint16_t *src )
{
   uint16_t *d = dst;

   while ( *d )
      d++;

   while ( 0 != (*d++ = *src++) )
      ;

   return (uint32_t) dst;
}




/*
 Name: lib_wcscmp
 Desc: Compare strings.
 Params:
   - s1: Null-terminated strings to compare (string of 16-bit unit).
   - s2: Null-terminated strings to compare (string of 16-bit unit).
 Returns: -
   uint32_t  value The return value for each of these functions indicates the
                    lexicographic relation of s1 to s2.
 Caveats: None.
*/

uint32_t lib_wcscmp( const uint16_t *s1, const uint16_t *s2 )
{
   uint32_t   result;

   while ( ( result = *s1 -*s2++ ) == 0 && *s1++ )
      ;

   return result;
}




/*
 Name: lib_stricmp
 Desc: Perform a lowercase comparison of strings.
 Params:
 - s1, s2: Null-terminated strings to compare (string of 16-bit unit).
 Returns:
   uint32_t  <0 s1 less than s2.
             =0 s1 identical to s2.
             >0 s1 greater than s2.
 Caveats: None.
*/

uint32_t lib_wcsicmp( const uint16_t *s1, const uint16_t *s2 )
{
   uint32_t   result;

   while ( ( result = lib_tolower( *s1 ) - lib_tolower( *s2 ) ) == 0 && *s1++ )
      s2++;

   return result;
}




/*
 Name: lib_strlen
 Desc: Get the length of a string.
 Params:
   - str: Null-terminated string (string of 16-bit unit).
 Returns:
   uint32_t >0 the length of a string.
 Caveats: This function returns the number of characters in string, excluding
          the terminal NULL.
*/

uint32_t lib_wcslen( const uint16_t *str )
{
   uint32_t len;

   for ( len = 0; *str++; len++ )
      ;
   return len;
}




/*
 Name: lib_lstrcpy
 Desc: Copies a string to a buffer.
 Params:
   - dst: Pointer to a buffer to receive the contents of the string pointed to
          by the src parameter. The buffer must be large enough to contain the
          string, including the terminating null character(string of 16-bit
          unit).
   - src: Pointer to the null-terminated string to be copied (string of 16-bit
          unit).
 Returns:
   int32_t value The return value is the length of copied.
 Caveats: None.
*/

int32_t lib_lwcscpy( uint16_t *dst, const uint16_t *src )
{
   const uint16_t *ori_dst = dst;

   while ( (*dst++ = *src++ ) != 0 )
      ;

   return (int32_t)(dst-ori_dst-1/*'\0'*/);
}




/*
 Name: lib_lstrncpy
 Desc: Copies a string to a buffer.
 Params:
   - dst: Pointer to a buffer to receive the contents of the string pointed to
          by the src parameter. The buffer must be large enough to contain the
          string, including the terminating null character (string of 16-bit
          unit).
   - src: Pointer to the null-terminated string to be copied (string of 16-bit
          unit).
   - len: Specifies the number of char values to be copied from the string
          pointed to by src into the buffer pointed to by dst, including a
          terminating null character.
 Returns: length of copied
 Caveats: None.
*/

int32_t lib_lwcsncpy( uint16_t *dst, const uint16_t *src, int32_t len )
{
   const uint16_t *ori_dst = dst;

   while ( len ) {
      len--;

      if ( 0 == (*dst = *src) )
         break;
      dst++;
      src++;
   }

   return (int32_t)(dst-ori_dst);
}




/*
 Name: lib_wcschr
 Desc:
 Params:
   - src:
   - ch:
 Returns:
   void*  value on success.
          NULL
 Caveats:
*/
/*
 Name: lib_wcschr
 Desc: Find a character in a string.
 Params:
   - src: Null-terminated source string (string of 16-bit unit).
   - ch: Character to be located (character of 16-bit unit).
 Returns:
   void*  value on success. The return value is a pointer to the first
                occurrence of ch in string.
          NULL on fail. The ch in string isn't found.
 Caveats: None.
*/


const void *lib_wcschr( const uint16_t *src, uint16_t ch )
{
   while ( *src )
      if ( *src++ == ch ) return src;

   return NULL;
}




/*
 Name: lib_strnchr
 Desc: Scan a string for the last occurrence of a character.
 Params:
   - src: Null-terminated string to search (string of 16-bit unit).
   - ch: Character to be located (string of 16-bit unit).
   - n: Number of characters to scaned.
 Returns:
   void*  value The return value a pointer to the last occurrence of ch in
                string,
          NULL  if ch is not found.
 Caveats: None.
*/

const void *lib_wcsnchr( const uint16_t *src, const uint16_t ch, int n )
{
   while ( *src && n--)
      if ( *src++ == ch ) return src;

   return NULL;
}




/*
 Name: lib_log2
 Desc: Calculates logarithms.
 Params:
   - val: Value whose logarithm is to be found.
 Returns:
   uint32_t  >0 Always. The return value is the logarithm of dwVal if
                successful.
 Caveats: None.
*/

uint32_t lib_log2( uint32_t val )
{
   uint32_t i = 0;

   while ( (val & 0x00000001) == 0 ) {
      val >>= 1;
      i++;
   }

   return i;
}




/*
 Name: lib_ucs2_2_utf8
 Desc: Convert UCS format to UTF8 format.
 Params:
   - ucs2: 16-bit character of UCS format
   - utf8: Pointer to buffer of UTF8 format.
 Returns:
   uint32_t  >0 Always. The return value is the length of characters in the
                parameter ucs2.
 Caveats: None.
*/

uint32_t lib_ucs2_2_utf8( const uint16_t ucs2, uint8_t *utf8 )
{
   uint32_t len;

   if ( ucs2 < 0x80 ) {
      len = 1;
      utf8[0] = (uint8_t) ucs2;
   } else if ( ucs2 < 0x0800 ) {
      len = 2;
      utf8[1] = (uint8_t) ((ucs2 & 0x3f) | 0x80);
      utf8[0] = (uint8_t) (((ucs2 << 2) & 0xcf00 | 0xc000) >> 8);
   } else {
      len = 3;
      utf8[2] = (uint8_t) ((ucs2 & 0x3f) | 0x80);
      utf8[1] = (uint8_t) (((ucs2 << 2) & 0x3f00 | 0x8000) >> 8);
      utf8[0] = (uint8_t) (((ucs2 << 4) & 0x3f0000 | 0xe00000) >> 16);
   }

   return len;
}

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

