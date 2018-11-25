/* psvgamesd.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2kern/kernel/modulemgr.h>

#include "functions.h"
#include "reader.h"
#include "dumper.h"
#include "global_log.h"
#include "defines.h"
#include "global_hooks.h"

#include "physical_mmc.h"

int module_start(SceSize argc, const void *args)
{
  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Startup iso driver\n");
  #endif

  initialize_read_threading();

  initialize_dump_threading();

  init_global_hooks();

  initialize_hooks_physical_mmc(); //default state

  return SCE_KERNEL_START_SUCCESS;
}

//Alias to inhibit compiler warning
void _start() __attribute__ ((weak, alias ("module_start")));

int module_stop(SceSize argc, const void *args)
{
  deinit_global_hooks();

  deinitialize_dump_threading();

  deinitialize_read_threading();

  return SCE_KERNEL_STOP_SUCCESS;
}
