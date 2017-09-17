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

sceKernelCreateCondForDriver_t* sceKernelCreateCondForDriver = 0;
sceKernelDeleteCondForDriver_t* sceKernelDeleteCondForDriver = 0;
sceKernelWaitCondForDriver_t* sceKernelWaitCondForDriver = 0;
sceKernelSignalCondForDriver_t* sceKernelSignalCondForDriver = 0;
sceKernelSha1DigestForDriver_t* sceKernelSha1DigestForDriver = 0;

sceKernelInitializeFastMutexForDriver_t* sceKernelInitializeFastMutexForDriver = 0;
sceKernelDeleteFastMutexForDriver_t* sceKernelDeleteFastMutexForDriver = 0;
sceKernelGetMutexInfoForDriver_t* sceKernelGetMutexInfoForDriver = 0;

sceSha256BlockInitForDriver_t* sceSha256BlockInitForDriver = 0;
sceSha256BlockUpdateForDriver_t* sceSha256BlockUpdateForDriver = 0;
sceSha256BlockResultForDriver_t* sceSha256BlockResultForDriver = 0;

int initialize_functions()
{
  int res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xDB6CD34A, (uintptr_t*)&sceKernelCreateCondForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to set sceKernelCreateCondForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelCreateCondForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xAEE0D27C, (uintptr_t*)&sceKernelDeleteCondForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to set sceKernelDeleteCondForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelDeleteCondForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xCC7E027D, (uintptr_t*)&sceKernelWaitCondForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to set sceKernelWaitCondForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelWaitCondForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xAC616150, (uintptr_t*)&sceKernelSignalCondForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to set sceKernelSignalCondForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelSignalCondForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceSysmem", SceKernelUtilsForDriver_NID, 0x87DC7F2F, (uintptr_t*)&sceKernelSha1DigestForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to set sceKernelSha1DigestForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelSha1DigestForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xaf8e1266, (uintptr_t*)&sceKernelInitializeFastMutexForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to get sceKernelInitializeFastMutexForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelInitializeFastMutexForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0x11fe84a1, (uintptr_t*)&sceKernelDeleteFastMutexForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to get sceKernelDeleteFastMutexForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelDeleteFastMutexForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0x69B78A12, (uintptr_t*)&sceKernelGetMutexInfoForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to get sceKernelGetMutexInfoForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceKernelGetMutexInfoForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceSysmem", SceKernelUtilsForDriver_NID, 0xd909fa2c, (uintptr_t*)&sceSha256BlockInitForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to get sceSha256BlockInitForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceSha256BlockInitForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceSysmem", SceKernelUtilsForDriver_NID, 0x236a9097, (uintptr_t*)&sceSha256BlockUpdateForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to get sceSha256BlockUpdateForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceSha256BlockUpdateForDriver\n");
  #endif

  res = module_get_export_func(KERNEL_PID, "SceSysmem", SceKernelUtilsForDriver_NID, 0x4899cd4b, (uintptr_t*)&sceSha256BlockResultForDriver);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to get sceSha256BlockResultForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("set sceSha256BlockResultForDriver\n");
  #endif

  return 0;
}
