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

typedef struct dump_args
{
  char* dump_path;
} dump_args;

SceUID dumpThreadId = -1;

//number of blocks per copy operation
#define DUMP_BLOCK_SIZE 0x10

char dump_buffer[SD_DEFAULT_SECTOR_SIZE * DUMP_BLOCK_SIZE];

int dump_img(SceUID dev_fd, SceUID out_fd, const MBR* dump_mbr)
{
  //dump sectors - main part
  SceSize nBlocks = dump_mbr->sizeInBlocks / DUMP_BLOCK_SIZE;
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
  SceSize nTail = dump_mbr->sizeInBlocks % DUMP_BLOCK_SIZE;
  if(nTail > 0)
  {
    ksceIoRead(dev_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * nTail);

    ksceIoWrite(out_fd, dump_buffer, SD_DEFAULT_SECTOR_SIZE * nTail);
  }

  return 0;
}

int dump_header(SceUID dev_fd, SceUID out_fd, const MBR* dump_mbr)
{
  //get data from gc memory
  char data_5018_buffer[0x34];
  get_5018_data(data_5018_buffer);

  //construct header
  psv_file_header_v1 img_header;
  img_header.magic = PSV_MAGIC;
  img_header.version = PSV_VERSION_V1;
  memcpy(img_header.key1, data_5018_buffer, 0x10);
  memcpy(img_header.key2, data_5018_buffer + 0x10, 0x10);
  memcpy(img_header.signature, data_5018_buffer + 0x20, 0x14);
  img_header.image_size = dump_mbr->sizeInBlocks * SD_DEFAULT_SECTOR_SIZE;

  //write data
  ksceIoWrite(out_fd, &img_header, sizeof(psv_file_header_v1));

  return 0;
}

int dump_core(SceUID dev_fd, SceUID out_fd)
{
  //get mbr data
  MBR dump_mbr;

  ksceIoRead(dev_fd, &dump_mbr, sizeof(MBR));

  if(memcmp(dump_mbr.header, SCEHeader, 0x20) != 0)
  {
    FILE_GLOBAL_WRITE_LEN("SCE header is invalid\n");
    return -1;
  }

  snprintf(sprintfBuffer, 256, "max sector in sd dev: %x\n", dump_mbr.sizeInBlocks);
  FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

  //seek to beginning
  ksceIoLseek(dev_fd, 0, SEEK_SET);

  //write header info
  dump_header(dev_fd, out_fd, &dump_mbr);

  //dump image itself
  //dump_img(dev_fd, out_fd, &dump_mbr);

  return 0;
}

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

  dump_core(dev_fd, out_fd);

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
  //TODO: need to implement cancel functionality

  if(dumpThreadId >= 0)
  {
    int waitRet = 0;
    ksceKernelWaitThreadEnd(dumpThreadId, &waitRet, 0);
    
    int delret = ksceKernelDeleteThread(dumpThreadId);
  }

  return 0;
}