#include "functions.h"

#include "hook_ids.h"
#include "global_log.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

sceKernelCreateCondForDriver_t* sceKernelCreateCondForDriver = 0;
sceKernelDeleteCondForDriver_t* sceKernelDeleteCondForDriver = 0;
sceKernelWaitCondForDriver_t* sceKernelWaitCondForDriver = 0;
sceKernelSignalCondForDriver_t* sceKernelSignalCondForDriver = 0;
sceKernelSha1DigestForDriver_t* sceKernelSha1DigestForDriver = 0;

int initialize_functions()
{
  int res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xDB6CD34A, (uintptr_t*)&sceKernelCreateCondForDriver);
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to set sceKernelCreateCondForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("set sceKernelCreateCondForDriver\n");

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xAEE0D27C, (uintptr_t*)&sceKernelDeleteCondForDriver);
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to set sceKernelDeleteCondForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("set sceKernelDeleteCondForDriver\n");

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xCC7E027D, (uintptr_t*)&sceKernelWaitCondForDriver);
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to set sceKernelWaitCondForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("set sceKernelWaitCondForDriver\n");

  res = module_get_export_func(KERNEL_PID, "SceKernelThreadMgr", SceThreadmgrForDriver_NID, 0xAC616150, (uintptr_t*)&sceKernelSignalCondForDriver);
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to set sceKernelSignalCondForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("set sceKernelSignalCondForDriver\n");

  res = module_get_export_func(KERNEL_PID, "SceSysmem", SceKernelUtilsForDriver_NID, 0x87DC7F2F, (uintptr_t*)&sceKernelSha1DigestForDriver);
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to set sceKernelSha1DigestForDriver : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("set sceKernelSha1DigestForDriver\n");

  return 0;
}