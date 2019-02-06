/* functions.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "functions.h"

#include "hook_ids.h"
#include "global_log.h"
#include "defines.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

sceKernelGetModuleInfoForKernel_t* sceKernelGetModuleInfoForKernel = 0;

extern size_t data_buffer_offset;

int initialize_functions()
{
  // 3.60-3.61
  int res = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", SceModulemgrForKernel_360_NID, 0xD269F915, (uintptr_t*)&sceKernelGetModuleInfoForKernel);
  if(res < 0)
  {
    // 3.63-3.70
    res = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", SceModulemgrForKernel_363_NID, 0xDAA90093, (uintptr_t*)&sceKernelGetModuleInfoForKernel);
    if(res < 0)
    {
      #ifdef ENABLE_DEBUG_LOG
      snprintf(sprintfBuffer, 256, "failed to get sceKernelGetModuleInfoForKernel : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
      #endif
      return -1;
    }
    else
    {
      data_buffer_offset = SceSblGcAuthMgr_363_BUF_OFFSET;
    }
  }
  else
  {
    data_buffer_offset = SceSblGcAuthMgr_360_BUF_OFFSET;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelGetModuleInfoForKernel\n");
  #endif

  return 0;
}
