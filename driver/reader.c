#include "reader.h"

#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "global_log.h"
#include "mbr_types.h"
#include "functions.h"

SceUID readThreadId = -1;

SceUID req_lock = -1;
SceUID resp_lock = -1;

SceUID req_cond = -1;
SceUID resp_cond = -1;

void* g_ctx_part = 0;
int g_sector = 0;
char* g_buffer = 0;
int g_nSectors = 0;
int g_res = 0;

MBR mbr;

int get_mbr(const char* path)
{
  SceUID iso_fd = ksceIoOpen(path, SCE_O_RDONLY, 0777);

  if(iso_fd >= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Opened iso\n");

    ksceIoRead(iso_fd, &mbr, sizeof(MBR));

    snprintf(sprintfBuffer, 256, "max sector: %x\n", mbr.sizeInBlocks);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    ksceIoClose(iso_fd);
  }
  else
  {
    FILE_GLOBAL_WRITE_LEN("Failed to open iso\n");
  }

  return 0;
}

char iso_path[256] = {0};

int set_reader_iso_path(const char* path)
{
  strncpy(iso_path, path, 256);

  get_mbr(iso_path);

  return 0;
}

int emulate_read(int sector, char* buffer, int nSectors)
{
  int res = 0;

  SceOff offset = (SceOff)sector * (SceOff)SD_DEFAULT_SECTOR_SIZE; //DO NOT REMOVE THE CASTS!
  SceSize size = nSectors * SD_DEFAULT_SECTOR_SIZE;

  if(sector >= mbr.sizeInBlocks)
  {
    memset(buffer, 0, size);
    res = -2;
  }
  else
  {
    SceUID iso_fd = ksceIoOpen(iso_path, SCE_O_RDONLY, 0777);
    if(iso_fd > 0)
    {
      SceOff newPos = ksceIoLseek(iso_fd, offset, SEEK_SET);
      if(newPos != offset)
      {
        memset(buffer, 0, size);
        res = -3;
      }
      else
      {
        int nbytes = ksceIoRead(iso_fd, buffer, size);
        if(nbytes != size)
          res = -4;
        else
          res = 0;
      }

      ksceIoClose(iso_fd);
    }
    else
    {
      memset(buffer, 0, size);
      res = -1;
    }
  }

  //snprintf(sprintfBuffer, 256, "sector: %x nSectors: %x result: %x\n", sector, nSectors, res);
  //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

  return res;
}

int read_thread(SceSize args, void *argp)
{
  FILE_GLOBAL_WRITE_LEN("Started Read Thread\n");

  while(1)
  {
    //lock mutex
    int res = ksceKernelLockMutex(req_lock, 1, 0);
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to ksceKernelLockMutex req_lock : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }

    //wait for request
    res = sceKernelWaitCondForDriver(req_cond, 0);
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to sceKernelWaitCondForDriver req_cond : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }

    //unlock mutex
    res = ksceKernelUnlockMutex(req_lock, 1);
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to ksceKernelUnlockMutex req_lock : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    
    g_res = emulate_read(g_sector, g_buffer, g_nSectors);

    //return response
    sceKernelSignalCondForDriver(resp_cond);
  }
  
  return 0;  
} 

int initialize_read_threading()
{
  req_lock = ksceKernelCreateMutex("req_lock", 0, 0, 0);
  if(req_lock >= 0)
    FILE_GLOBAL_WRITE_LEN("Created req_lock\n");

  req_cond = sceKernelCreateCondForDriver("req_cond", 0, req_lock, 0);
  if(req_cond >= 0)
    FILE_GLOBAL_WRITE_LEN("Created req_cond\n");

  resp_lock = ksceKernelCreateMutex("resp_lock", 0, 0, 0);
  if(resp_lock >= 0)
    FILE_GLOBAL_WRITE_LEN("Created resp_lock\n");

  resp_cond = sceKernelCreateCondForDriver("resp_cond", 0, resp_lock, 0);
  if(resp_cond >= 0)
    FILE_GLOBAL_WRITE_LEN("Created resp_cond\n");
  
  readThreadId = ksceKernelCreateThread("ReadThread", &read_thread, 0x64, 0x1000, 0, 0, 0);

  if(readThreadId >= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Created Read Thread\n");

    int res = ksceKernelStartThread(readThreadId, 0, 0);
  }

  return 0;
}

int deinitialize_read_threading()
{
  if(readThreadId >= 0)
  {
    int waitRet = 0;
    ksceKernelWaitThreadEnd(readThreadId, &waitRet, 0);
    
    int delret = ksceKernelDeleteThread(readThreadId);
  }

  sceKernelDeleteCondForDriver(req_cond);
  sceKernelDeleteCondForDriver(resp_cond);

  ksceKernelDeleteMutex(req_lock);
  ksceKernelDeleteMutex(resp_lock);

  return 0;
}