#include <psp2kern/kernel/modulemgr.h>

#include "functions.h"
#include "reader.h"
#include "dumper.h"
#include "global_log.h"

#include "physical_mmc.h"

int module_start(SceSize argc, const void *args) 
{
  FILE_GLOBAL_WRITE_LEN("Startup iso driver\n");

  if(initialize_functions() >= 0)
  {
    initialize_read_threading();
  }

  initialize_dump_threading();

  initialize_hooks_physical_mmc(); //default state

  return SCE_KERNEL_START_SUCCESS;
}
 
//Alias to inhibit compiler warning
void _start() __attribute__ ((weak, alias ("module_start")));
 
int module_stop(SceSize argc, const void *args) 
{
  deinitialize_dump_threading();

  deinitialize_read_threading();
  
  return SCE_KERNEL_STOP_SUCCESS;
}