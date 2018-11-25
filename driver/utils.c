/* utils.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "utils.h"

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/types.h>
#include <psp2kern/io/fcntl.h>

#include <taihen.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "global_log.h"
#include "defines.h"
#include "sector_api.h"
#include "functions.h"

void CMD_BIN_LOG(char* data, int size)
{
  SceUID global_log_fd = ksceIoOpen("ux0:dump/cmd_log.bin", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

  if(global_log_fd >= 0)
  {
    ksceIoWrite(global_log_fd, data, size);
    ksceIoClose(global_log_fd);
  }
}

int print_bytes(const char* data, int len)
{
  #ifdef ENABLE_DEBUG_LOG

  for(int i = 0; i < len; i++)
  {
    snprintf(sprintfBuffer, 256, "%02x", data[i]);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  FILE_GLOBAL_WRITE_LEN("\n");

  #endif

  return 0;
}

int print_cmd(cmd_input* cmd_data, int n,  char* when)
{
    #ifdef ENABLE_DEBUG_LOG

    snprintf(sprintfBuffer, 256, "--- CMD%d (%d) %s ---\n", cmd_data->command, n, when);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "cmd1: %x\n", cmd_data);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "argument: %x\n", cmd_data->argument);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    /*
    snprintf(sprintfBuffer, 256, "buffer: %x\n", cmd_data->buffer);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "state: %x\n", cmd_data->state_flags);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "error_code: %x\n", cmd_data->error_code);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "unk_64: %x\n", cmd_data->unk_64);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "resp_block_size_24: %x\n", cmd_data->resp_block_size_24);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "resp_n_blocks_26: %x\n", cmd_data->resp_n_blocks_26);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "base_198: %x\n", cmd_data->base_198);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "offset_19C: %x\n", cmd_data->offset_19C);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "size_1A0: %x\n", cmd_data->size_1A0);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "size_1A4: %x\n", cmd_data->size_1A4);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    */

    /*
    if((((int)cmd_data->state_flags) << 0x15) < 0)
    {
      FILE_GLOBAL_WRITE_LEN("INVALIDATE\n");
      //print_bytes(cmd_data->base_198, 0x200);

      //print_bytes(cmd_data->vaddr_1C0, 0x10);
      //print_bytes(cmd_data->vaddr_200, 0x10);

      //print_bytes(cmd_data->base_198 + cmd_data->offset_19C, cmd_data->size_1A4);
    }
    */

    /*
    if(((0x801 << 9) & cmd_data->state_flags) != 0)
      FILE_GLOBAL_WRITE_LEN("SKIP INVALIDATE\n");

    if((((int)cmd_data->state_flags) << 0xB) < 0)
     FILE_GLOBAL_WRITE_LEN("FREE mem_188\n");
    */

    //print_bytes(cmd_data->response, 0x10);

    /*
    if(cmd_data->buffer > 0)
       print_bytes(cmd_data->buffer, cmd_data->b_size);
    */

    //print_bytes(cmd_data->vaddr_80, 0x10);

    #endif

    return 0;
}

int print_SceSdif1_lock_info(SceUID mutex)
{
  if(mutex >= 0)
  {
    SceKernelMutexInfo info;
    memset(&info, 0, sizeof(SceKernelMutexInfo));
    info.size = sizeof(SceKernelMutexInfo);

    int res = ksceKernelGetMutexInfo(mutex, &info);

    #ifdef ENABLE_DEBUG_LOG
    if(res >= 0)
    {
      snprintf(sprintfBuffer, 256, "name: %s\n", info.name);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

      snprintf(sprintfBuffer, 256, "attr: %x\n", info.attr);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

      snprintf(sprintfBuffer, 256, "initCount: %x\n", info.initCount);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

      snprintf(sprintfBuffer, 256, "currentCount: %x\n", info.currentCount);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

      snprintf(sprintfBuffer, 256, "currentOwnerId: %x\n", info.currentOwnerId);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

      snprintf(sprintfBuffer, 256, "numWaitThreads: %x\n", info.numWaitThreads);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    else
    {
      snprintf(sprintfBuffer, 256, "Failed to get mutex info %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif
  }

  return 0;
}

int dumpSegment(SceKernelModuleInfo* minfo, int index)
{
  if(minfo->segments <= 0)
    return -1;

  if (minfo->segments[index].vaddr <= 0)
  {
    FILE_GLOBAL_WRITE_LEN("segment is empty\n");
    return -1;
  }

  {
    snprintf(sprintfBuffer, 256, "%d %x %x\n", index, minfo->segments[index].vaddr, minfo->segments[index].memsz);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  char filename[100] = {0};
  char moduleNameCopy[30] = {0};
  snprintf(moduleNameCopy, 30, minfo->module_name);
  snprintf(filename, 100, "ux0:dump/0x%08x_%s_%d.bin", (unsigned)minfo->segments[index].vaddr, moduleNameCopy, index);

  {
    snprintf(sprintfBuffer, 256, "%s\n", filename);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  SceUID fout = ksceIoOpen(filename, SCE_O_CREAT | SCE_O_TRUNC | SCE_O_WRONLY, 0777);

  if(fout < 0)
     return -1;

  if(minfo->segments[index].memsz > 0)
  {
    ksceIoWrite(fout, minfo->segments[index].vaddr, minfo->segments[index].memsz);
  }

  ksceIoClose(fout);

  return 0;
}

int dump_sdif_data()
{
  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    SceKernelModuleInfo minfo;
    minfo.size = sizeof(SceKernelModuleInfo);
    int ret = ksceKernelGetModuleInfo(KERNEL_PID, sdif_info.modid, &minfo);
    if(ret >= 0)
    {
      FILE_GLOBAL_WRITE_LEN("ready to dump sdif data seg\n");

      dumpSegment(&minfo, 1);
    }
    else
    {
      FILE_GLOBAL_WRITE_LEN("can not dump sdif data seg\n");
    }
  }

  return 0;
}
