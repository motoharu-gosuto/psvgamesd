#include "psvgamesd.h"

#include <psp2kern/types.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/suspend.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/net/net.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <taihen.h>
#include <module.h>

#include "defines.h"

#include "sector_api.h"
#include "mbr_types.h"
#include "cmd56_key.h"
#include "mmc_emu.h"
#include "global_log.h"

//-------------

#define SceSdifForDriver_NID 0x96D306FA
#define SceIofilemgrForDriver_NID 0x40FD29C7
#define SceSblGcAuthMgrGcAuthForDriver_NID 0xC6627F5E
#define SceThreadmgrForDriver_NID 0xE2C40624
#define SceKernelUtilsForDriver_NID 0x496AD8B4

//-------------

sceKernelCreateCondForDriver_t* sceKernelCreateCondForDriver = 0;
sceKernelDeleteCondForDriver_t* sceKernelDeleteCondForDriver = 0;
sceKernelWaitCondForDriver_t* sceKernelWaitCondForDriver = 0;
sceKernelSignalCondForDriver_t* sceKernelSignalCondForDriver = 0;
sceKernelSha1DigestForDriver_t* sceKernelSha1DigestForDriver = 0;

insert_handler* sceSdifInsertHandler = 0;
remove_handler* sceSdifRemoveHandler = 0;

//-------------

char* iso_path = "ux0:iso/XXX.bin";

MBR mbr;

char sprintfBuffer[256];

SceUID readThreadId = -1;

SceUID req_lock = -1;
SceUID resp_lock = -1;

SceUID req_cond = -1;
SceUID resp_cond = -1;

SceUID dumpThreadId = -1;

SceUID removeInsertCardThreadId = -1;

//-------------

tai_hook_ref_t sd_read_hook_ref;
SceUID sd_read_hook_id = -1;

SceUID gen_init_2_patch_uid = -1; // patch of zero func in gen_init_2 function

SceUID hs_dis_patch1_uid = -1; //high speed disable patch 1
SceUID hs_dis_patch2_uid = -1; //high speed disable patch 2

tai_hook_ref_t init_sd_hook_ref;
SceUID init_sd_hook_id = -1; // hook of sd init function in Sdif

tai_hook_ref_t send_command_hook_ref;
SceUID send_command_hook_id = -1;

tai_hook_ref_t gc_cmd56_handshake_hook_ref;
SceUID gc_cmd56_handshake_hook_id = -1;

tai_hook_ref_t mmc_read_hook_ref;
SceUID mmc_read_hook_id = -1;

//-------------

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
  for(int i = 0; i < len; i++)
  {
    snprintf(sprintfBuffer, 256, "%02x", data[i]);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  FILE_GLOBAL_WRITE_LEN("\n");

  return 0;
}

//-------------

interrupt_argument* get_int_arg(int index)
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0) 
  {
    interrupt_argument* int_arg = 0;
    int res = module_get_offset(KERNEL_PID, sdstor_info.modid, 1, 0x1B20 + sizeof(interrupt_argument) * index, (uintptr_t*)&int_arg);
    if(res < 0)
    {
      FILE_GLOBAL_WRITE_LEN("get_int_arg failed\n");
      return 0;
    }
    else
    {
      return int_arg;
    }
  }
  else
  {
    FILE_GLOBAL_WRITE_LEN("get_int_arg failed\n");
    return 0;
  }
}

int insert_game_card()
{
  if(sceSdifInsertHandler > 0)
  {
    FILE_GLOBAL_WRITE_LEN("Signal insert\n");
    return sceSdifInsertHandler(0, get_int_arg(SCE_SDSTOR_SDIF1_INDEX));
  }
  else
  {
    FILE_GLOBAL_WRITE_LEN("Signal insert failed\n");
    return -1;
  }
}

int remove_game_card()
{
  if(sceSdifRemoveHandler > 0)
  {
    FILE_GLOBAL_WRITE_LEN("Signal remove\n");
    return sceSdifRemoveHandler(0, get_int_arg(SCE_SDSTOR_SDIF1_INDEX));
  }
  else
  {
    FILE_GLOBAL_WRITE_LEN("Signal remove failed\n");
    return -1;
  }
}

int insert_game_card_emu()
{
  interrupt_argument* ia = get_int_arg(SCE_SDSTOR_SDIF1_INDEX);
  if(ia <= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Signal insert failed\n");
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("Signal insert\n");
  return ksceKernelSetEventFlag(ia->SceSdstorRequest_evid, 0x10);
}

int remove_game_card_emu()
{
  interrupt_argument* ia = get_int_arg(SCE_SDSTOR_SDIF1_INDEX);
  if(ia <= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Signal remove failed\n");
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("Signal remove\n");
  return ksceKernelSetEventFlag(ia->SceSdstorRequest_evid, 0x1);
}

int remove_insert_card_thread(SceSize args, void *argp)
{
  FILE_GLOBAL_WRITE_LEN("Started RemoveInsertCard Thread\n");

  ksceKernelDelayThread(1000000 * 60);
  remove_game_card();

  //ksceKernelDelayThread(1000 * 500);
  //insert_game_card();

  return 0;
}

//-------------

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

int emulate_read_sd(void* ctx_part, int sector, char* buffer, int nSectors)
{
  int res = TAI_CONTINUE(int, sd_read_hook_ref, ctx_part, sector, buffer, nSectors);

  //snprintf(sprintfBuffer, 256, "sector: %x nSectors: %x result: %x\n", sector, nSectors, res);
  //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

  return res;
}

int emulate_read_mmc(void* ctx_part, int sector, char* buffer, int nSectors)
{
  int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector,	buffer, nSectors);

  //snprintf(sprintfBuffer, 256, "sector: %x nSectors: %x result: %x\n", sector, nSectors, res);
  //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

  return res;
}

void* g_ctx_part = 0;
int g_sector = 0;
char* g_buffer = 0;
int g_nSectors = 0;
int g_res = 0;

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

    /*
    char sha1[0x14];
    sceKernelSha1DigestForDriver(g_buffer, g_nSectors * SD_DEFAULT_SECTOR_SIZE, sha1);
    */

    //g_res = emulate_read_sd(g_ctx_part, g_sector, g_buffer, g_nSectors);

    //g_res = emulate_read_mmc(g_ctx_part, g_sector, g_buffer, g_nSectors);

    /*
    char sha2[0x14];
    sceKernelSha1DigestForDriver(g_buffer, g_nSectors * SD_DEFAULT_SECTOR_SIZE, sha2);

    if(memcmp(sha1, sha2, 0x14) != 0)
    {
      snprintf(sprintfBuffer, 256, "Invalid read at sector: %x nSectors: %x\n", g_sector, g_nSectors);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    */

    //return response
    sceKernelSignalCondForDriver(resp_cond);
  }
  
  return 0;  
}

#define DUMP_BLOCK_SIZE 0x10

char dump_buffer[SD_DEFAULT_SECTOR_SIZE * DUMP_BLOCK_SIZE];

int dump_thread(SceSize args, void *argp)
{
  FILE_GLOBAL_WRITE_LEN("Started Dump Thread\n");

  SceUID dev_fd = ksceIoOpen("sdstor0:gcd-lp-ign-entire", SCE_O_RDONLY, 0777);

  if(dev_fd >= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Opened sd dev\n");

    SceUID out_fd = ksceIoOpen("ux0:iso/SAO_Hollow_Fragment.bin", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

    if(out_fd >= 0)
    {
      FILE_GLOBAL_WRITE_LEN("Opened output file\n");

      //get mbr data
      MBR dump_mbr;

      ksceIoRead(dev_fd, &dump_mbr, sizeof(MBR));

      if(memcmp(dump_mbr.header, SCEHeader, 0x20) == 0)
      {
        snprintf(sprintfBuffer, 256, "max sector in sd dev: %x\n", dump_mbr.sizeInBlocks);
        FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

        //seek to beginning
        ksceIoLseek(dev_fd, 0, SEEK_SET);

        //dump sectors
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

        FILE_GLOBAL_WRITE_LEN("Dump finished\n");
      }
      else
      {
        FILE_GLOBAL_WRITE_LEN("SCE header is invalid\n");
      }

      ksceIoClose(out_fd);
    }
    else
    {
      FILE_GLOBAL_WRITE_LEN("Failed to open output file\n");
    }

    ksceIoClose(dev_fd);
  }
  else
  {
    FILE_GLOBAL_WRITE_LEN("Failed to open sd dev\n");
  }

  return 0;
}

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

//sd read operation hook that redirects directly to file (without separate thread)
int sd_read_hook(void* ctx_part, int sector, char* buffer, int nSectors)
{
  return emulate_read(sector, buffer, nSectors);
}

//sd read operation hook that redirects directly to sd card (physical read)
int sd_read_hook_through(void* ctx_part, int sector, char* buffer, int nSectors)
{
  int res = TAI_CONTINUE(int, sd_read_hook_ref, ctx_part, sector, buffer, nSectors);
  return res;
}

int set_5018_data()
{
  tai_module_info_t gc_info;
  gc_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSblGcAuthMgr", &gc_info) >= 0)
  {
    uintptr_t addr = 0;
    int ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x5018, &addr);
    if(ofstRes == 0)
    {
      memcpy((char*)addr, data_5018, 0x34);
    }
  }

  return 0;
}

//this is a hook for sd init operation (mmc init operation is different)
//we need to imitate cmd56 handshake after init
//this is done by writing last cmd56 packet to correct location of GcAuthMgr module
int init_sd_hook(int sd_ctx_index, void** ctx_part)
{
  int res = TAI_CONTINUE(int, init_sd_hook_ref, sd_ctx_index, ctx_part);
  
  set_5018_data();
  
  return res;
}

//by some unknown reason real sectors on the sd card start from 0x8000
//TODO: I need to figure out this later
#define ADDRESS_OFFSET 0x8000

//this hook modifies offset to data that is sent to the card
//this is done only for game card device by checking ctx pointer with sd api
//this is done only for commands CMD17 (read) and CMD18 (write)
int send_command_hook(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    if(cmd_data1->command == 17 || cmd_data1->command == 18)
    {
      cmd_data1->argument = cmd_data1->argument + ADDRESS_OFFSET; //fixup address. I have no idea why I should do it
    }
  }

  int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);

  return res;
}

int print_cmd(cmd_input* cmd_data, int n,  char* when)
{
    snprintf(sprintfBuffer, 256, "--- CMD%d (%d) %s ---\n", cmd_data->command, n, when);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "cmd1: %x\n", cmd_data);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "argument: %x\n", cmd_data->argument);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

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

    if((((int)cmd_data->state_flags) << 0x15) < 0)
    {
      FILE_GLOBAL_WRITE_LEN("INVALIDATE\n");
      //print_bytes(cmd_data->base_198, 0x200);

      //print_bytes(cmd_data->vaddr_1C0, 0x10);
      //print_bytes(cmd_data->vaddr_200, 0x10);

      //print_bytes(cmd_data->base_198 + cmd_data->offset_19C, cmd_data->size_1A4);
    }

    if(((0x801 << 9) & cmd_data->state_flags) != 0)
      FILE_GLOBAL_WRITE_LEN("SKIP INVALIDATE\n");

    if((((int)cmd_data->state_flags) << 0xB) < 0)
     FILE_GLOBAL_WRITE_LEN("FREE mem_188\n");

    //print_bytes(cmd_data->response, 0x10);

    /*
    if(cmd_data->buffer > 0)
       print_bytes(cmd_data->buffer, cmd_data->b_size);
    */

    //print_bytes(cmd_data->vaddr_80, 0x10);

    return 0;
}

int emulate_mmc_command_debug(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  //there is a timing check for cmd56 in gcauthmgr so can not do much logging here (since it is slow)
  //i know where this check is but it is not worth patching right now since cmd56 auth can be bypassed
  if(cmd_data1->command != 56)
  {
    print_cmd(cmd_data1, 1, "before");

    CMD_BIN_LOG((char*)cmd_data1, sizeof(cmd_input));

    if(cmd_data2 > 0)
    {
      print_cmd(cmd_data2, 2, "before");

      CMD_BIN_LOG((char*)cmd_data2, sizeof(cmd_input));
    }
  }

  int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);

  /*
  if(cmd_data1->command == 8)
  {
    if(cmd_data1->buffer > 0)
    {
      CMD_BIN_LOG(cmd_data1->buffer, 0x200);
    }
  }
  */

  if(cmd_data1->command != 56)
  {
    print_cmd(cmd_data1, 1, "after");

    CMD_BIN_LOG((char*)cmd_data1, sizeof(cmd_input));

    if(cmd_data2 > 0)
    {
      print_cmd(cmd_data2, 2, "after");

      CMD_BIN_LOG((char*)cmd_data2, sizeof(cmd_input));
    }
  }

  if(cmd_data1->command != 56)
  {
    FILE_GLOBAL_WRITE_LEN("------\n");
    snprintf(sprintfBuffer, 256, "result %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  return res;
}

int send_command_debug_hook(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    return emulate_mmc_command_debug(ctx, cmd_data1, cmd_data2, nIter, num);
  }
  else
  {
    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);
    return res;
  }
}

int send_command_emu_hook(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    /*
    if(cmd_data1->command != 56)
    {
      print_cmd(cmd_data1, 1, "before");

      //CMD_BIN_LOG((char*)cmd_data1, sizeof(cmd_input));

      if(cmd_data2 > 0)
      {
        print_cmd(cmd_data2, 2, "before");

        //CMD_BIN_LOG((char*)cmd_data2, sizeof(cmd_input));
      }
    }
    */

    int res = emulate_mmc_command(ctx, cmd_data1, cmd_data2, nIter, num);
    
    /*
    if(cmd_data1->command != 56)
    {
      print_cmd(cmd_data1, 1, "after");

      //CMD_BIN_LOG((char*)cmd_data1, sizeof(cmd_input));

      if(cmd_data2 > 0)
      {
        print_cmd(cmd_data2, 2, "after");

        //CMD_BIN_LOG((char*)cmd_data2, sizeof(cmd_input));
      }
    }

    if(cmd_data1->command != 56)
    {
      FILE_GLOBAL_WRITE_LEN("------\n");
      snprintf(sprintfBuffer, 256, "result %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    */

    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);
    return res;
  }
}

//this hook gets all sensitive data that is cleaned up by GcAuthMgr function 0xBB451E83
int get_5018_data()
{
  tai_module_info_t gc_info;
  gc_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSblGcAuthMgr", &gc_info) >= 0)
  {
    uintptr_t addr = 0;
    int ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x5018, &addr);
    if(ofstRes == 0)
    {
      char data_5018_buffer[0x34] = {0};

      memcpy(data_5018_buffer, (char*)addr, 0x34);

      SceUID fd = ksceIoOpen("ux0:dump/cmd56_key.bin", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

      if(fd >= 0)
      {
        ksceIoWrite(fd,data_5018_buffer, 0x34);
        ksceIoClose(fd);
      }
    }
  }

  return 0;
}

//this is a hook of GcAuth function that performs cmd56 handshake
int gc_cmd56_handshake_hook(int param0)
{
  int res = TAI_CONTINUE(int, gc_cmd56_handshake_hook_ref, param0);

  get_5018_data();
  
  return res;
}

int gc_cmd56_handshake_override_hook(int param0)
{
  set_5018_data();

  FILE_GLOBAL_WRITE_LEN("override cmd56 handshake\n");

  ksceKernelDelayThread(2000000); //2 seconds

  return 0;
}

int mmc_read_hook_through(void* ctx_part, int	sector,	char* buffer, int nSectors)
{
  //make sure that only mmc operations are redirected
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ((sd_context_part_base*)ctx_part)->gctx_ptr)
  {
    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector,	buffer, nSectors);
    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector,	buffer, nSectors);
    return res;
  }
}

int mmc_read_hook(void* ctx_part, int	sector,	char* buffer, int nSectors)
{
  //make sure that only mmc operations are redirected
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ((sd_context_part_base*)ctx_part)->gctx_ptr)
  {
    return emulate_read(sector, buffer, nSectors);
  }
  else
  {
    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector,	buffer, nSectors);
    return res;
  }
}

int mmc_read_hook_threaded(void* ctx_part, int	sector,	char* buffer, int nSectors)
{
  //make sure that only mmc operations are redirected
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ((sd_context_part_base*)ctx_part)->gctx_ptr)
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
  else
  {
    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector,	buffer, nSectors);
    return res;
  }
}

int initialize_all_hooks()
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0) 
  {
    #ifdef ENABLE_SD_PATCHES

    #ifdef ENABLE_SEPARATE_READ_THREAD
      sd_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sd_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0xb9593652, sd_read_hook_threaded);
    #else
      #ifdef ENABLE_READ_THROUGH
      sd_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sd_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0xb9593652, sd_read_hook_through);
      #else
      sd_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sd_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0xb9593652, sd_read_hook);
      #endif
    #endif
    
    //patch for proc_initialize_generic_2 - so that sd card type is not ignored
    char zeroCallOnePatch[4] = {0x01, 0x20, 0x00, 0xBF};
    gen_init_2_patch_uid = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x2498, zeroCallOnePatch, 4); //patch (BLX) to (MOVS R0, #1 ; NOP)

    #endif

    #ifdef OVERRIDE_CMD56_HANDSHAKE
    gc_cmd56_handshake_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &gc_cmd56_handshake_hook_ref, "SceSdstor", SceSblGcAuthMgrGcAuthForDriver_NID, 0x68781760, gc_cmd56_handshake_override_hook);
    #else
    gc_cmd56_handshake_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &gc_cmd56_handshake_hook_ref, "SceSdstor", SceSblGcAuthMgrGcAuthForDriver_NID, 0x68781760, gc_cmd56_handshake_hook);
    #endif

    #ifdef ENABLE_MMC_READ

    #ifdef ENABLE_MMC_SEPARATE_READ_THREAD
      mmc_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &mmc_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0x6f8d529b, mmc_read_hook_threaded);
    #else
      #ifdef ENABLE_MMC_READ_THROUGH
      mmc_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &mmc_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0x6f8d529b, mmc_read_hook_through);
      #else
      mmc_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &mmc_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0x6f8d529b, mmc_read_hook);
      #endif
    #endif

    #endif

    //initialize card insert / remove handlers
    module_get_offset(KERNEL_PID, sdstor_info.modid, 0, 0x3BD5, (uintptr_t*)&sceSdifInsertHandler);

    module_get_offset(KERNEL_PID, sdstor_info.modid, 0, 0x3BC9, (uintptr_t*)&sceSdifRemoveHandler);
  }

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    #ifdef ENABLE_SD_PATCHES

    #ifdef ENABLE_SD_LOW_SPEED_PATCH
    //this patch modifies CMD6 argument to check for availability of low speed mode instead of high speed mode
    char lowSpeed_check[4] = {0xF0, 0xFF, 0xFF, 0x00};
    hs_dis_patch1_uid = taiInjectDataForKernel(KERNEL_PID, sdif_info.modid, 0, 0x6B34, lowSpeed_check, 4); //0x06, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xFF, 0x00

    //this patch modifies CMD6 argument to set low speed mode instead of high speed mode
    char lowSpeed_set[4] = {0xF0, 0xFF, 0xFF, 0x80};
    hs_dis_patch2_uid = taiInjectDataForKernel(KERNEL_PID, sdif_info.modid, 0, 0x6B54, lowSpeed_set, 4); //0x06, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xFF, 0x80
    #endif
    
    //this hooks sd init function (there is separate function for mmc init)
    init_sd_hook_id = taiHookFunctionExportForKernel(KERNEL_PID, &init_sd_hook_ref, "SceSdif", SceSdifForDriver_NID, 0xc1271539, init_sd_hook);

    //this hooks command send functoin which is the main function for executing all commands that are sent from Vita to SD/MMC devices
    send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_hook);

    #endif

    #ifdef OVERRIDE_COMMANDS_DEBUG
      send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_debug_hook);
    #else
      #ifdef OVERRIDE_COMMANDS_EMU
      send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_emu_hook);
      #endif
    #endif
  }

  return 0;
}

int deinitialize_all_hooks()
{
  if(sd_read_hook_id >= 0)
   taiHookReleaseForKernel(sd_read_hook_id, sd_read_hook_ref);
  
  if(gen_init_2_patch_uid >= 0)
    taiInjectReleaseForKernel(gen_init_2_patch_uid);

  if(hs_dis_patch1_uid >= 0)
    taiInjectReleaseForKernel(hs_dis_patch1_uid);

  if(hs_dis_patch2_uid >= 0)
    taiInjectReleaseForKernel(hs_dis_patch2_uid);

  if(init_sd_hook_id >= 0)
    taiHookReleaseForKernel(init_sd_hook_id, init_sd_hook_ref);

  if(send_command_hook_id >= 0)
    taiHookReleaseForKernel(send_command_hook_id, send_command_hook_ref);

  if(gc_cmd56_handshake_hook_id >= 0)
    taiHookReleaseForKernel(gc_cmd56_handshake_hook_id, gc_cmd56_handshake_hook_ref);

  if(mmc_read_hook_id >= 0)
    taiHookReleaseForKernel(mmc_read_hook_id, mmc_read_hook_ref);
    
  return 0;
}

int initialize_threading()
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

  #ifdef ENABLE_DUMP_THREAD

  dumpThreadId = ksceKernelCreateThread("DumpThread", &dump_thread, 0x64, 0x1000, 0, 0, 0);
  
  if(dumpThreadId >= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Created Dump Thread\n");

    int res = ksceKernelStartThread(dumpThreadId, 0, 0);
  }

  #endif

  removeInsertCardThreadId = ksceKernelCreateThread("RemoveInsertCardThread", &remove_insert_card_thread, 0x64, 0x1000, 0, 0, 0);

  if(removeInsertCardThreadId >= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Created RemoveInsertCard Thread\n");

    int res = ksceKernelStartThread(removeInsertCardThreadId, 0, 0);
  }

  return 0;
}

int deinitialize_threading()
{
  if(removeInsertCardThreadId >= 0)
  {
    int waitRet = 0;
    ksceKernelWaitThreadEnd(removeInsertCardThreadId, &waitRet, 0);
    
    int delret = ksceKernelDeleteThread(removeInsertCardThreadId);
  }

  #ifdef ENABLE_DUMP_THREAD
  
  if(dumpThreadId >= 0)
  {
    int waitRet = 0;
    ksceKernelWaitThreadEnd(dumpThreadId, &waitRet, 0);
    
    int delret = ksceKernelDeleteThread(dumpThreadId);
  }

  #endif

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

int module_start(SceSize argc, const void *args) 
{
  FILE_GLOBAL_WRITE_LEN("Startup iso driver\n");

  if(initialize_functions() >= 0)
  {
    initialize_threading();
  }

  SceUID iso_fd = ksceIoOpen(iso_path, SCE_O_RDONLY, 0777);

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

  initialize_all_hooks();

  return SCE_KERNEL_START_SUCCESS;
}
 
//Alias to inhibit compiler warning
void _start() __attribute__ ((weak, alias ("module_start")));
 
int module_stop(SceSize argc, const void *args) 
{
  deinitialize_all_hooks();

  deinitialize_threading();
  
  return SCE_KERNEL_STOP_SUCCESS;
}