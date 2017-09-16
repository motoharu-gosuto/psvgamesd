#include "dumper.h"

#include <psp2kern/types.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/suspend.h>
#include <psp2kern/kernel/threadmgr.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "global_log.h"
#include "mbr_types.h"
#include "cmd56_key.h"
#include "psv_types.h"
#include "functions.h"
#include "reader.h"
#include "defines.h"

#define DUMP_STATE_START 1
#define DUMP_STATE_STOP 0

typedef struct dump_args
{
  char* dump_path;
} dump_args;

SceUID g_dumpThreadId = -1;

SceUID g_dumpPollThreadId = -1;

SceUID dump_req_lock = -1;
SceUID dump_resp_lock = -1;

SceUID dump_req_cond = -1;
SceUID dump_resp_cond = -1;

int g_dump_state = 0;
char g_dump_path[256] = {0};

//---------------

SceUID g_total_sectors_mutex_id = -1;

uint32_t g_total_sectors = 0;

uint32_t get_total_sectors()
{
  ksceKernelLockMutex(g_total_sectors_mutex_id, 1, 0);
  uint32_t temp = g_total_sectors;
  ksceKernelUnlockMutex(g_total_sectors_mutex_id, 1);
  return temp;
}

void set_total_sectors(uint32_t value)
{
  ksceKernelLockMutex(g_total_sectors_mutex_id, 1, 0);
  g_total_sectors = value;
  ksceKernelUnlockMutex(g_total_sectors_mutex_id, 1);
}

//---------------

SceUID g_progress_sectors_mutex_id = -1;

uint32_t g_progress_sectors = 0;

uint32_t get_progress_sectors()
{
  ksceKernelLockMutex(g_progress_sectors_mutex_id, 1, 0);
  uint32_t temp = g_progress_sectors;
  ksceKernelUnlockMutex(g_progress_sectors_mutex_id, 1);
  return temp;
}

void set_progress_sectors(uint32_t value)
{
  ksceKernelLockMutex(g_progress_sectors_mutex_id, 1, 0);
  g_progress_sectors = value;
  ksceKernelUnlockMutex(g_progress_sectors_mutex_id, 1);
}

//---------------

SceUID g_running_state_mutex_id = -1;

uint32_t g_running_state = 0;

uint32_t get_running_state()
{
  ksceKernelLockMutex(g_running_state_mutex_id, 1, 0);
  uint32_t temp = g_running_state;
  ksceKernelUnlockMutex(g_running_state_mutex_id, 1);
  return temp;
}

void set_running_state(uint32_t value)
{
  ksceKernelLockMutex(g_running_state_mutex_id, 1, 0);
  g_running_state = value;
  ksceKernelUnlockMutex(g_running_state_mutex_id, 1);
}

//---------------

//number of blocks per copy operation
#define DUMP_BLOCK_SIZE 0x10

//#define DUMP_BLOCK_TICK_SIZE 0x1000
#define DUMP_BLOCK_TICK_SIZE 0x100

char dump_buffer[SD_DEFAULT_SECTOR_SIZE * DUMP_BLOCK_SIZE];

int dump_img(SceUID dev_fd, SceUID out_fd, const MBR* dump_mbr)
{
  set_total_sectors(dump_mbr->sizeInBlocks);
  set_progress_sectors(0);

  //dump sectors - main part
  SceSize nBlocks = dump_mbr->sizeInBlocks / DUMP_BLOCK_SIZE;
  for(int i = 0; i < nBlocks; i++)
  {
    if((i % DUMP_BLOCK_TICK_SIZE) == 0)
    {
      #ifdef ENABLE_DEBUG_LOG
      //snprintf(sprintfBuffer, 256, "%x from %x\n", i, nBlocks);
      //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
      #endif

      //make sure vita does not go to sleep
      ksceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);

      //report number of sectors that are dumped
      set_progress_sectors(i * DUMP_BLOCK_SIZE);

      //check dump cancel request 
      //maybe it can be done on each iteration
      //but I think it will be slow
      uint32_t rn_state = get_running_state();
      if(rn_state == DUMP_STATE_STOP)
      {
        set_total_sectors(0);
        set_progress_sectors(0);
        return 0;
      }
    }

    ksceIoRead(dev_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * DUMP_BLOCK_SIZE);

    ksceIoWrite(out_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * DUMP_BLOCK_SIZE);
  }

  //dump sectors - tail
  SceSize nTail = dump_mbr->sizeInBlocks % DUMP_BLOCK_SIZE;
  if(nTail > 0)
  {
    ksceIoRead(dev_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * nTail);

    ksceIoWrite(out_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * nTail);
  }

  //report number of sectors that are dumped
  set_progress_sectors(dump_mbr->sizeInBlocks);

  return 0;
}

int dump_header(SceUID dev_fd, SceUID out_fd, const MBR* dump_mbr)
{
  //get data from gc memory
  char data_5018_buffer[CMD56_DATA_SIZE];
  get_5018_data(data_5018_buffer);

  //construct header
  psv_file_header_v1 img_header;
  img_header.magic = PSV_MAGIC;
  img_header.version = PSV_VERSION_V1;
  img_header.flags = 0;
  memcpy(img_header.key1, data_5018_buffer, 0x10);
  memcpy(img_header.key2, data_5018_buffer + 0x10, 0x10);
  memcpy(img_header.signature, data_5018_buffer + 0x20, 0x14);
  memset(img_header.hash, 0, 0x20);
  img_header.image_size = dump_mbr->sizeInBlocks * SD_DEFAULT_SECTOR_SIZE;
  img_header.image_offset_sector = 1;

  //write data
  ksceIoWrite(out_fd, &img_header, sizeof(psv_file_header_v1));

  //write padding
  int padding_size = SD_DEFAULT_SECTOR_SIZE - sizeof(psv_file_header_v1);
  char padding_data[SD_DEFAULT_SECTOR_SIZE];
  memset(padding_data, 0, SD_DEFAULT_SECTOR_SIZE);

  ksceIoWrite(out_fd, padding_data, padding_size);
  
  return 0;
}

int dump_core(SceUID dev_fd, SceUID out_fd)
{
  //get mbr data
  MBR dump_mbr;

  ksceIoRead(dev_fd, &dump_mbr, sizeof(MBR));

  if(memcmp(dump_mbr.header, SCEHeader, 0x20) != 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("SCE header is invalid\n");
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  snprintf(sprintfBuffer, 256, "max sector in sd dev: %x\n", dump_mbr.sizeInBlocks);
  FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  #endif

  //seek to beginning
  ksceIoLseek(dev_fd, 0, SEEK_SET);

  //write header info
  dump_header(dev_fd, out_fd, &dump_mbr);

  //dump image itself
  dump_img(dev_fd, out_fd, &dump_mbr);

  return 0;
}

int dump_thread_internal(SceSize args, void* argp)
{
  dump_args* da = (dump_args*)argp;
  if(da <= 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Invalid arguments in dump thread\n");
    #endif
    return -1;
  }

  SceUID dev_fd = ksceIoOpen("sdstor0:gcd-lp-ign-entire", SCE_O_RDONLY, 0777);
  if(dev_fd < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Failed to open sd dev\n");
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Opened sd dev\n");
  #endif

  SceUID out_fd = ksceIoOpen(da->dump_path, SCE_O_CREAT | SCE_O_TRUNC | SCE_O_WRONLY, 0777);

  if(out_fd < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Failed to open output file\n");
    #endif

    ksceIoClose(dev_fd);
    return -1; 
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Opened output file\n");
  #endif

  dump_core(dev_fd, out_fd);

  ksceIoClose(out_fd);
  ksceIoClose(dev_fd);

  return 0;
}

int dump_thread(SceSize args, void* argp)
{
  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Started Dump Thread\n");
  #endif

  //indicate that dumping process has started
  set_running_state(DUMP_STATE_START);

  dump_thread_internal(args, argp);

  //indicate that dumping process has finished
  set_running_state(DUMP_STATE_STOP);

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Dump finished\n");
  #endif

  return 0;
} 

dump_args da_inst;
char da_inst_dump_path[256] = {0};

int initialize_dump_thread(const char* dump_path)
{
  g_dumpThreadId = ksceKernelCreateThread("DumpThread", &dump_thread, 0x64, 0x10000, 0, 0, 0);
  
  if(g_dumpThreadId >= 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Created Dump Thread\n");
    #endif

    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "path %s\n", dump_path);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);     
    #endif

    //copying the path to yet another variable to be able to clear g_dump_path that is used for requests
    memset(da_inst_dump_path, 0, 256);
    strncpy(da_inst_dump_path, dump_path, 256);
    da_inst.dump_path = da_inst_dump_path;

    int res = ksceKernelStartThread(g_dumpThreadId, sizeof(dump_args), &da_inst);
  }
  else
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "Failed to create Dump Thread: %x\n", g_dumpThreadId);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
  }

  return 0;
}

int deinitialize_dump_thread()
{
  //wait till thread finishes and do a cleanup

  if(g_dumpThreadId >= 0)
  {  
    int waitRet = 0;
    ksceKernelWaitThreadEnd(g_dumpThreadId, &waitRet, 0);
    
    int delret = ksceKernelDeleteThread(g_dumpThreadId);
    g_dumpThreadId = -1; 
  }

  return 0;
}

int handle_dump_request(int dump_state, const char* dump_path)
{
  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("handle_dump_request\n");
  #endif

  switch(dump_state)
  {
    case DUMP_STATE_START:
    {
      //dont allow to enter dumping state if already dumping

      uint32_t rn_state = get_running_state();

      if(rn_state != DUMP_STATE_START)
      {
        //if previous dump operation was not canceled - dump thread will not be deinitialized
        deinitialize_dump_thread();

        initialize_dump_thread(dump_path);
      }

      break;
    }
    case DUMP_STATE_STOP:
    {
      //dont allow to enter cancel state if not yet dumping

      uint32_t rn_state = get_running_state();

      if(rn_state != DUMP_STATE_STOP)
      {
        //indicate that we are entering cancel state (this will stop dumping thread)
        set_running_state(DUMP_STATE_STOP);

        deinitialize_dump_thread();

        #ifdef ENABLE_DEBUG_LOG
        FILE_GLOBAL_WRITE_LEN("Dump canceled\n");
        #endif
      }

      break;
    }
    default:
    {
      #ifdef ENABLE_DEBUG_LOG
      FILE_GLOBAL_WRITE_LEN("Unknown dump state\n");
      #endif
      break;
    }
  }

  return 0;
}

int dump_poll_thread(SceSize args, void* argp)
{
  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Started Dump Poll Thread\n");
  #endif
  
  while(1)
  {
    //lock mutex
    int res = ksceKernelLockMutex(dump_req_lock, 1, 0);
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to ksceKernelLockMutex dump_req_lock : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif

    //wait for request
    res = sceKernelWaitCondForDriver(dump_req_cond, 0);
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to sceKernelWaitCondForDriver dump_req_cond : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif

    //unlock mutex
    res = ksceKernelUnlockMutex(dump_req_lock, 1);
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to ksceKernelUnlockMutex dump_req_lock : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif

    handle_dump_request(g_dump_state, g_dump_path);

    //return response
    sceKernelSignalCondForDriver(dump_resp_cond);
  }

  return 0;
}

int initialize_dump_threading()
{
  g_total_sectors_mutex_id = ksceKernelCreateMutex("total_sectors_mutex", 0, 0, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(g_total_sectors_mutex_id >= 0)
    FILE_GLOBAL_WRITE_LEN("Created g_total_sectors_mutex\n");
  #endif

  g_progress_sectors_mutex_id = ksceKernelCreateMutex("progress_sectors_mutex", 0, 0, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(g_progress_sectors_mutex_id >= 0)
    FILE_GLOBAL_WRITE_LEN("Created g_progress_sectors_mutex\n");
  #endif

  g_running_state_mutex_id = ksceKernelCreateMutex("running_state_mutex", 0, 0, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(g_running_state_mutex_id >= 0)
    FILE_GLOBAL_WRITE_LEN("Created g_running_state_mutex\n");
  #endif

  dump_req_lock = ksceKernelCreateMutex("dump_req_lock", 0, 0, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(dump_req_lock >= 0)
    FILE_GLOBAL_WRITE_LEN("Created dump_req_lock\n");
  #endif

  dump_req_cond = sceKernelCreateCondForDriver("dump_req_cond", 0, dump_req_lock, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(dump_req_cond >= 0)
    FILE_GLOBAL_WRITE_LEN("Created dump_req_cond\n");
  #endif

  dump_resp_lock = ksceKernelCreateMutex("dump_resp_lock", 0, 0, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(dump_resp_lock >= 0)
    FILE_GLOBAL_WRITE_LEN("Created dump_resp_lock\n");
  #endif

  dump_resp_cond = sceKernelCreateCondForDriver("dump_resp_cond", 0, dump_resp_lock, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(dump_resp_cond >= 0)
    FILE_GLOBAL_WRITE_LEN("Created dump_resp_cond\n");
  #endif

  g_dumpPollThreadId = ksceKernelCreateThread("DumpPollThread", &dump_poll_thread, 0x64, 0x10000, 0, 0, 0);

  if(g_dumpPollThreadId >= 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Created Dump Poll Thread\n");
    #endif

    int res = ksceKernelStartThread(g_dumpPollThreadId, 0, 0);
  }
  else
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "Failed to create Dump Poll Thread: %x\n", g_dumpPollThreadId);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
  }

  return 0;
}

int deinitialize_dump_threading()
{
  //deinitialize dump thread if last dump was successfull
  //if it was canceled - it will be already deinitialized
  deinitialize_dump_thread();

  if(g_dumpPollThreadId >= 0)
  {
    int waitRet = 0;
    ksceKernelWaitThreadEnd(g_dumpPollThreadId, &waitRet, 0);
    
    int delret = ksceKernelDeleteThread(g_dumpPollThreadId);
    g_dumpPollThreadId = -1;
  }

  if(dump_req_cond >= 0)
  {
    sceKernelDeleteCondForDriver(dump_req_cond);
    dump_req_cond = -1;
  }

  if(dump_resp_cond >= 0)
  {
    sceKernelDeleteCondForDriver(dump_resp_cond);
    dump_resp_cond = -1;
  }

  if(dump_req_lock >= 0)
  {
    ksceKernelDeleteMutex(dump_req_lock);
    dump_req_lock = -1;
  }

  if(dump_resp_lock >= 0)
  {
    ksceKernelDeleteMutex(dump_resp_lock);
    dump_resp_lock = -1;
  }

  if(g_total_sectors_mutex_id >= 0)
  {
    ksceKernelDeleteMutex(g_total_sectors_mutex_id);
    g_total_sectors_mutex_id = -1;
  }

  if(g_progress_sectors_mutex_id >= 0)
  {
    ksceKernelDeleteMutex(g_progress_sectors_mutex_id);
    g_progress_sectors_mutex_id = -1;
  }

  if(g_running_state_mutex_id >= 0)
  {
    ksceKernelDeleteMutex(g_running_state_mutex_id);
    g_running_state_mutex_id = -1;
  }

  return 0;
}

//---------------

int dump_request_response_base()
{
  //send request
  sceKernelSignalCondForDriver(dump_req_cond);

  //lock mutex
  int res = ksceKernelLockMutex(dump_resp_lock, 1, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to ksceKernelLockMutex dump_resp_lock : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }
  #endif

  //wait for response
  res = sceKernelWaitCondForDriver(dump_resp_cond, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to sceKernelWaitCondForDriver dump_resp_cond : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }
  #endif

  //unlock mutex
  res = ksceKernelUnlockMutex(dump_resp_lock, 1);
  #ifdef ENABLE_DEBUG_LOG
  if(res < 0)
  {
    snprintf(sprintfBuffer, 256, "failed to ksceKernelUnlockMutex dump_resp_lock : %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }
  #endif

  return 0;
}

int dump_mmc_card_start_internal(const char* dump_path)
{
  g_dump_state = DUMP_STATE_START;
  memset(g_dump_path, 0, 256);
  strncpy(g_dump_path, dump_path, 256);

  dump_request_response_base();

  return 0;
}

int dump_mmc_card_stop_internal()
{
  g_dump_state = DUMP_STATE_STOP;
  memset(g_dump_path, 0, 256);
  
  dump_request_response_base();

  return 0;
}