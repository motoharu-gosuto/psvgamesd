 #include "virtual_sd.h"

 #include <psp2kern/kernel/threadmgr.h>

 #include "hook_ids.h"
 #include "functions.h"
 #include "reader.h"
 #include "global_log.h"

//TODO: this implementation is not finished

//sd read operation can be redirected to file only in separate thread
//it looks like file i/o api causes some internal locks/conflicts
//when called from deep inside of Sdif driver subroutines
int sd_read_hook_threaded(void* ctx_part, int sector, char* buffer, int nSectors)
{
  g_ctx_part = ctx_part;
  g_sector = sector;
  g_buffer = buffer;
  g_nSectors = nSectors;

  //send request
  sceKernelSignalCondForDriver(req_cond);

  //lock mutex
  int res = ksceKernelLockMutex(resp_lock, 1, 0);
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to ksceKernelLockMutex resp_lock : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  //wait for response
  res = sceKernelWaitCondForDriver(resp_cond, 0);
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to sceKernelWaitCondForDriver resp_cond : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  //unlock mutex
  res = ksceKernelUnlockMutex(resp_lock, 1);
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to ksceKernelUnlockMutex resp_lock : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  return g_res;
}

int initialize_hooks_virtual_sd()
{
  return 0;
}

int deinitialize_hooks_virtual_sd()
{
  return 0;
}