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

typedef struct dump_args
{
  char* dump_path;
} dump_args;

SceUID dumpThreadId = -1;

//number of blocks per copy operation
#define DUMP_BLOCK_SIZE 0x10

char dump_buffer[SD_DEFAULT_SECTOR_SIZE * DUMP_BLOCK_SIZE];

int dump_thread(SceSize args, void* argp)
{
  FILE_GLOBAL_WRITE_LEN("Started Dump Thread\n");

  dump_args* da = (dump_args*)argp;
  if(da <= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Invalid arguments in dump thread\n");
    return -1;
  }

  SceUID dev_fd = ksceIoOpen("sdstor0:gcd-lp-ign-entire", SCE_O_RDONLY, 0777);
  if(dev_fd < 0)
  {
    FILE_GLOBAL_WRITE_LEN("Failed to open sd dev\n");
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("Opened sd dev\n");

  SceUID out_fd = ksceIoOpen(da->dump_path, SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

  if(out_fd < 0)
  {
    FILE_GLOBAL_WRITE_LEN("Failed to open output file\n");

    ksceIoClose(dev_fd);
    return -1; 
  }

  FILE_GLOBAL_WRITE_LEN("Opened output file\n");

  //get mbr data
  MBR dump_mbr;

  ksceIoRead(dev_fd, &dump_mbr, sizeof(MBR));

  if(memcmp(dump_mbr.header, SCEHeader, 0x20) != 0)
  {
    FILE_GLOBAL_WRITE_LEN("SCE header is invalid\n");

    ksceIoClose(out_fd);
    ksceIoClose(dev_fd);
    return -1;
  }

  snprintf(sprintfBuffer, 256, "max sector in sd dev: %x\n", dump_mbr.sizeInBlocks);
  FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

  //seek to beginning
  ksceIoLseek(dev_fd, 0, SEEK_SET);

  //dump sectors - main part
  SceSize nBlocks = dump_mbr.sizeInBlocks / DUMP_BLOCK_SIZE;
  for(int i = 0; i < nBlocks; i++)
  {
    if((i % 0x1000) == 0)
    {
      snprintf(sprintfBuffer, 256, "%x from %x\n", i, nBlocks);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

      ksceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
    }

    ksceIoRead(dev_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * DUMP_BLOCK_SIZE);

    ksceIoWrite(out_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * DUMP_BLOCK_SIZE);
  }

  //dump sectors - tail
  SceSize nTail = dump_mbr.sizeInBlocks % DUMP_BLOCK_SIZE;
  if(nTail > 0)
  {
    ksceIoRead(dev_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * nTail);

    ksceIoWrite(out_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * nTail);
  }

  FILE_GLOBAL_WRITE_LEN("Dump finished\n");

  ksceIoClose(out_fd);
  ksceIoClose(dev_fd);
  
  return 0;
} 

dump_args da_inst;

int initialize_dump_threading(const char* dump_path)
{
  dumpThreadId = ksceKernelCreateThread("DumpThread", &dump_thread, 0x64, 0x1000, 0, 0, 0);
  
  if(dumpThreadId >= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Created Dump Thread\n");

    memset(da_inst.dump_path, 0, 256);
    strncpy(da_inst.dump_path, dump_path, 256);

    int res = ksceKernelStartThread(dumpThreadId, sizeof(dump_args), &da_inst);
  }

  return 0;
}

int deinitialize_dump_threading()
{
  if(dumpThreadId >= 0)
  {
    int waitRet = 0;
    ksceKernelWaitThreadEnd(dumpThreadId, &waitRet, 0);
    
    int delret = ksceKernelDeleteThread(dumpThreadId);
  }

  return 0;
}