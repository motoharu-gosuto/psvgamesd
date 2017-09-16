#include "reader.h"

#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "global_log.h"
#include "mbr_types.h"
#include "functions.h"
#include "defines.h"

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

MBR g_mbr;

const MBR* get_mbr_ptr()
{
  return &g_mbr;
}

int get_mbr(const char* path)
{
  if(strnlen(path, 256) > 0)
  {
    SceUID iso_fd = ksceIoOpen(path, SCE_O_RDONLY, 0777);

    if(iso_fd >= 0)
    {
      #ifdef ENABLE_DEBUG_LOG
      FILE_GLOBAL_WRITE_LEN("Opened iso\n");
      #endif

      //read base header

      psv_file_header_base header_base;
      ksceIoRead(iso_fd, &header_base, sizeof(psv_file_header_base));

      if(header_base.magic != PSV_MAGIC || header_base.version != PSV_VERSION_V1)
      {
        #ifdef ENABLE_DEBUG_LOG
        FILE_GLOBAL_WRITE_LEN("Iso magic or version is invalid\n");
        #endif

        ksceIoClose(iso_fd);
        return -1;
      }

      //read version header

      ksceIoLseek(iso_fd, 0, SEEK_SET);

      psv_file_header_v1 header_ver;

      ksceIoRead(iso_fd, &header_ver, sizeof(psv_file_header_v1));

      //read mbr

      //DO NOT REMOVE THE CASTS!
      SceOff mbr_offset = (SceOff)header_ver.image_offset_sector * (SceOff)SD_DEFAULT_SECTOR_SIZE;

      ksceIoLseek(iso_fd, mbr_offset, SEEK_SET);

      ksceIoRead(iso_fd, &g_mbr, sizeof(MBR));

      #ifdef ENABLE_DEBUG_LOG
      snprintf(sprintfBuffer, 256, "max sector: %x\n", g_mbr.sizeInBlocks);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
      #endif

      ksceIoClose(iso_fd);
    }
    else
    {
      #ifdef ENABLE_DEBUG_LOG
      FILE_GLOBAL_WRITE_LEN("Failed to open iso\n");
      #endif
      return -1;
    }
  }

  return -1;
}

psv_file_header_v1 g_img_header;

int get_img_header(const char* path)
{
  if(strnlen(path, 256) > 0)
  {
    SceUID iso_fd = ksceIoOpen(path, SCE_O_RDONLY, 0777);

    if(iso_fd >= 0)
    {
      #ifdef ENABLE_DEBUG_LOG
      FILE_GLOBAL_WRITE_LEN("Opened iso\n");
      #endif

      //read base header

      psv_file_header_base header_base;
      ksceIoRead(iso_fd, &header_base, sizeof(psv_file_header_base));

      if(header_base.magic != PSV_MAGIC || header_base.version != PSV_VERSION_V1)
      {
        #ifdef ENABLE_DEBUG_LOG
        FILE_GLOBAL_WRITE_LEN("ISO magic or version is invalid\n");
        #endif

        ksceIoClose(iso_fd);
        return -1;
      }

      //read version header

      ksceIoLseek(iso_fd, 0, SEEK_SET);

      ksceIoRead(iso_fd, &g_img_header, sizeof(psv_file_header_v1));

      ksceIoClose(iso_fd);
    }
    else
    {
      #ifdef ENABLE_DEBUG_LOG
      FILE_GLOBAL_WRITE_LEN("Failed to open iso\n");
      #endif
      return -1;
    }
  }

  return -1;
}

int get_cmd56_data_base(psv_file_header_v1* ih, char* buffer)
{
  memcpy(buffer, ih->key1, 0x10);
  memcpy(buffer + 0x10, ih->key2, 0x10);
  memcpy(buffer + 0x20, ih->signature, 0x14);
  
  return 0;
}

int get_cmd56_data(char* buffer)
{
  return get_cmd56_data_base(&g_img_header, buffer);
}

char iso_path[256] = {0};

int set_reader_iso_path(const char* path)
{
  strncpy(iso_path, path, 256);

  get_img_header(iso_path);

  get_mbr(iso_path);

  return 0;
}

int clear_reader_iso_path()
{
  memset(iso_path, 0, 256);

  return 0;
}

int emulate_read(int sector, char* buffer, int nSectors)
{
  int res = 0;

  //DO NOT REMOVE THE CASTS!
  SceOff offset = (SceOff)g_img_header.image_offset_sector * (SceOff)SD_DEFAULT_SECTOR_SIZE;
  offset = offset + (SceOff)sector * (SceOff)SD_DEFAULT_SECTOR_SIZE;

  SceSize size = nSectors * SD_DEFAULT_SECTOR_SIZE;

  if(sector >= g_mbr.sizeInBlocks)
  {
    //handling trimmed image
    if((g_img_header.flags & FLAG_TRIMMED) > 0)
    {
      memset(buffer, 0, size);
      res = 0;
    }
    else
    {
      memset(buffer, 0, size);
      res = SD_UNKNOWN_READ_WRITE_ERROR; 
    }
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
        res = SD_UNKNOWN_READ_WRITE_ERROR;
      }
      else
      {
        int nbytes = ksceIoRead(iso_fd, buffer, size);
        if(nbytes != size)
          res = SD_UNKNOWN_READ_WRITE_ERROR;
        else
          res = 0;
      }

      ksceIoClose(iso_fd);
    }
    else
    {
      memset(buffer, 0, size);
      res = SD_UNKNOWN_READ_WRITE_ERROR;
    }
  }

  #ifdef ENABLE_DEBUG_LOG
  //snprintf(sprintfBuffer, 256, "sector: %x nSectors: %x result: %x\n", sector, nSectors, res);
  //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  #endif

  return res;
}

int read_thread(SceSize args, void *argp)
{
  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Started Read Thread\n");
  #endif

  while(1)
  {
    //lock mutex
    int res = ksceKernelLockMutex(req_lock, 1, 0);
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to ksceKernelLockMutex req_lock : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif

    //wait for request
    res = sceKernelWaitCondForDriver(req_cond, 0);
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to sceKernelWaitCondForDriver req_cond : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif

    //unlock mutex
    res = ksceKernelUnlockMutex(req_lock, 1);
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to ksceKernelUnlockMutex req_lock : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif
    
    g_res = emulate_read(g_sector, g_buffer, g_nSectors);

    //return response
    sceKernelSignalCondForDriver(resp_cond);
  }
  
  return 0;  
} 

int initialize_read_threading()
{
  req_lock = ksceKernelCreateMutex("req_lock", 0, 0, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(req_lock >= 0)
    FILE_GLOBAL_WRITE_LEN("Created req_lock\n");
  #endif

  req_cond = sceKernelCreateCondForDriver("req_cond", 0, req_lock, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(req_cond >= 0)
    FILE_GLOBAL_WRITE_LEN("Created req_cond\n");
  #endif

  resp_lock = ksceKernelCreateMutex("resp_lock", 0, 0, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(resp_lock >= 0)
    FILE_GLOBAL_WRITE_LEN("Created resp_lock\n");
  #endif

  resp_cond = sceKernelCreateCondForDriver("resp_cond", 0, resp_lock, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(resp_cond >= 0)
    FILE_GLOBAL_WRITE_LEN("Created resp_cond\n");
  #endif
  
  readThreadId = ksceKernelCreateThread("ReadThread", &read_thread, 0x64, 0x1000, 0, 0, 0);

  if(readThreadId >= 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Created Read Thread\n");
    #endif

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
    readThreadId = -1;
  }

  if(req_cond >= 0)
  {
    sceKernelDeleteCondForDriver(req_cond);
    req_cond = -1;
  }
  
  if(resp_cond >= 0)
  {
    sceKernelDeleteCondForDriver(resp_cond);
    resp_cond = -1;
  }

  if(req_lock >= 0)
  {
    ksceKernelDeleteMutex(req_lock);
    req_lock = -1;
  }

  if(resp_lock >= 0)
  {
    ksceKernelDeleteMutex(resp_lock);
    resp_lock = -1;
  }

  return 0;
}