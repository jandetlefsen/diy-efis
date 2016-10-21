#include "osd.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

handle_t sm_fat;
handle_t sm_ofile;
handle_t sm_dpath;
handle_t sm_dcache;

void os_zinit_osd(void)
  {
  }

result_t os_init_sm(void)
  {
  mutex_create(&sm_fat);
  mutex_create(&sm_ofile);
  mutex_create(&sm_dpath);
  mutex_create(&sm_dcache);
  
  return s_ok;
  }

result_t os_terminate_sm(void)
  {
  mutex_delete(sm_fat);
  mutex_delete(sm_ofile);
  mutex_delete(sm_dpath);
  mutex_delete(sm_dcache);
  
  return s_ok;
  }

bool set_safe_mode(bool is_safe)
  {
  return true;
  }
/*
 Name: os_break
 Desc: When exceptions occur, this function is called.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void os_break(void)
  {
  // TODO: implement this
  while(1);
  }

/*
 Name: os_assert
 Desc: Cause the assertion.
 Params:
   - condition: Specify the condition of assertion.
 Returns: None.
 Caveats: None.
 */

void os_assert(bool condition)
  {
  if(false == condition)
    os_break();
  }

/*
 Name: os_exit
 Desc: When exceptions occur, this function is called.
 Params:
   - exit_code: The exit code number.
 Returns: None.
 Caveats: None.
 */

void os_exit(uint32_t exit_code)
  {
  // TODO: figure out
  os_break();
  }
