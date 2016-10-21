#include "dpath.h"



/*-----------------------------------------------------------------------------
 DEFINE GLOVAL VARIABLES & DEFINITIONS
-----------------------------------------------------------------------------*/

#if defined( FORBID_CHAR )
/* The file name forbids the following characters. */
char forbid_chars[] = {':', '*', '?', '\"', '<', '>', '|'};
/* Check the forbidden characters. */
#define IS_FORBID_CHAR(ch)  (((ch)==':') || ((ch)=='*') || ((ch)=='?') || \
                            ((ch)=='\"') || ((ch)=='<') || ((ch)=='>') || \
                            ((ch)=='|') )
#endif
/* Check the characters for path name. */
#define IS_PATH_CHAR(ch)  (((ch)=='\\') || ((ch)=='/'))




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: fsm_parse_path
 Desc: Parsing the string by a delimiter character('\\' or '/') in given path,
       then put each parsed string to fsm_arg_t structure.
 Params:
   - buf: The string buffer will be filled with NULL character instead of one
          of delimeter in Path.
   - arg: Pointer to the data structure in which the elements parsed are placed.
   - path: Pointer to the buffer to be parsed.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fsm_parse_path(char *buf, fsm_arg_t *arg, const char *path)
  {
  const char *src;
  char *dst;
  int32_t argc, i;


  src = path;
  dst = buf;
  argc = i = 0;

  while (1)
    {
    while (IS_PATH_CHAR(*src))
      src++;

    if ('\0' == *src) break;

    dst += i;
    i = 0;
    arg->argv[argc++] = dst;

    if (ALLPATH_ELEMENTS_MAX < argc)
      return set_errno(e_name_too_long);

    while (1)
      {
      if ('\0' == *src)
        {
        dst[i] = '\0';

        /* Ignore the case which finishes with dot or space character. */
        while (i && ((SPACE_CHAR == dst[i - 1]) || (DOT_CHAR == dst[i - 1])))
          dst[--i] = '\0';
        if (0 == i) return set_errno(e_path);
        goto End;
        }

      if (IS_PATH_CHAR(*src))
        break;

#if defined( FORBID_CHAR )
      if ((SPACE_CHAR > *src) || IS_FORBID_CHAR(*src))
        return set_errno(e_path);
#else
      if ((SPACE_CHAR > *src))
        return set_errno(e_path);
#endif

      dst[i++] = *src++;

      if ((ALLPATH_LEN_MAX - 2/*volume length*/) < ((int32_t) src - (int32_t) path))
        return set_errno(e_name_too_long);
      }


    /* Ignore the case which finishes with dot or space character. */
    while (i && ((SPACE_CHAR == dst[i - 1]) || (DOT_CHAR == dst[i - 1])))
      dst[--i] = '\0';

    dst[i++] = '\0';

    if (0 == i) return set_errno(e_path);
    }

End:
  arg->argc = argc;

  return s_ok;
  }

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

