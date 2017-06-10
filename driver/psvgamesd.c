#include <psp2kern/types.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
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

//-------------

#define SceSdifForDriver_NID 0x96D306FA
#define SceIofilemgrForDriver_NID 0x40FD29C7
#define SceSblGcAuthMgrGcAuthForDriver_NID 0xC6627F5E

//-------------

char* iso_path = "ux0:iso/XXX.bin";

SceUID global_log_fd = -1;

MBR mbr;

char sprintfBuffer[256];

SceUID readThreadId = -1;

SceUID req_sema;
SceUID resp_sema;

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

//-------------

void FILE_GLOBAL_WRITE_LEN(char* msg)
{
  global_log_fd = ksceIoOpen("ux0:dump/game_log.txt", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

  if(global_log_fd >= 0)
  {
    ksceIoWrite(global_log_fd, msg, strlen(msg));
    ksceIoClose(global_log_fd);
  }  
}

//-------------

int emulate_read(int sector, char* buffer, int nSectors)
{
  int res = 0;

  //snprintf(sprintfBuffer, 256, "sector: %x nSectors: %x\n", sector, nSectors);
  //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

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

  //snprintf(sprintfBuffer, 256, "result: %x\n", res);
  //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

  return res;
}

int g_sector = 0;
char* g_buffer = 0;
int g_nSectors = 0;
int g_res = 0;

int read_thread(SceSize args, void *argp)
{
  FILE_GLOBAL_WRITE_LEN("Started Read Thread\n");

  while(1)
  {
    //wait for request
    ksceKernelWaitSema(req_sema, 1, NULL);

    g_res = emulate_read(g_sector, g_buffer, g_nSectors);

    //return response
    ksceKernelSignalSema(resp_sema, 1);
  }
  
  return 0;  
}

//sd read operation can be redirected to file only in separate thread
//it looks like file i/o api causes some internal locks/conflicts
//when called from deep inside of Sdif driver subroutines
int sd_read_hook_threaded(sd_context_part* ctx, int sector, char* buffer, int nSectors)
{
  g_sector = sector;
  g_buffer = buffer;
  g_nSectors = nSectors;

  //send request
  ksceKernelSignalSema(req_sema, 1);

  //wait for response
  ksceKernelWaitSema(resp_sema, 1, NULL);

  return g_res;
}

//sd read operation hook that redirects directly to file (without separate thread)
int sd_read_hook(sd_context_part* ctx, int sector, char* buffer, int nSectors)
{
  return emulate_read(sector, buffer, nSectors);
}

//sd read operation hook that redirects directly to sd card (physical read)
int sd_read_hook_through(sd_context_part* ctx, int sector, char* buffer, int nSectors)
{
  int res = TAI_CONTINUE(int, sd_read_hook_ref, ctx, sector, buffer, nSectors);
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
int init_sd_hook(int sd_ctx_index, sd_context_part** result)
{
  int res = TAI_CONTINUE(int, init_sd_hook_ref, sd_ctx_index, result);
  
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

    //this data is constant 0x02 and zeroes
    /*
    addr = 0;
    ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x544C, &addr);
    if(ofstRes == 0)
    {
      char data_544C_buffer[0x20] = {0};

      memcpy(data_544C_buffer, (char*)addr, 0x20);

      SceUID fd = ksceIoOpen("ux0:dump/cmd56_key.bin", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

      if(fd >= 0)
      {
        ksceIoWrite(fd,data_544C_buffer, 0x20);
        ksceIoClose(fd);
      } 
    }
    */

    //this data is not constant
    /*
    addr = 0;
    ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x4BC4, &addr);
    if(ofstRes == 0)
    {
      char data_4BC4_buffer[0x30] = {0};

      memcpy(data_4BC4_buffer, (char*)addr, 0x30);

      SceUID fd = ksceIoOpen("ux0:dump/cmd56_key.bin", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

      if(fd >= 0)
      {
        ksceIoWrite(fd,data_4BC4_buffer, 0x30);
        ksceIoClose(fd);
      } 
    }
    */

    //this data is constant
    /*
    addr = 0;
    ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x4FF8, &addr);
    if(ofstRes == 0)
    {
      char data_4FF8_buffer[0x20] = {0};

      memcpy(data_4FF8_buffer, (char*)addr, 0x20);

      SceUID fd = ksceIoOpen("ux0:dump/cmd56_key.bin", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

      if(fd >= 0)
      {
        ksceIoWrite(fd,data_4FF8_buffer, 0x20);
        ksceIoClose(fd);
      } 
    }
    */
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
    #endif
    sd_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sd_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0xb9593652, sd_read_hook);
    #endif
    
    //patch for proc_initialize_generic_2 - so that sd card type is not ignored
    char zeroCallOnePatch[4] = {0x01, 0x20, 0x00, 0xBF};
    gen_init_2_patch_uid = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x2498, zeroCallOnePatch, 4); //patch (BLX) to (MOVS R0, #1 ; NOP)

    #endif

    gc_cmd56_handshake_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &gc_cmd56_handshake_hook_ref, "SceSdstor", SceSblGcAuthMgrGcAuthForDriver_NID, 0x68781760, gc_cmd56_handshake_hook);
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
    
  return 0;
}

int module_start(SceSize argc, const void *args) 
{
  FILE_GLOBAL_WRITE_LEN("Startup iso driver\n");

  req_sema = ksceKernelCreateSema("req_sema", 0, 0, 1, NULL);
  if(req_sema >= 0)
    FILE_GLOBAL_WRITE_LEN("Created req sema\n");

  resp_sema = ksceKernelCreateSema("resp_sema", 0, 0, 1, NULL);
  if(resp_sema >= 0)
    FILE_GLOBAL_WRITE_LEN("Created resp sema\n");
  
  readThreadId = ksceKernelCreateThread("ReadThread", &read_thread, 0x64, 0x1000, 0, 0, 0);

  if(readThreadId >=0)
  {
    FILE_GLOBAL_WRITE_LEN("Created Read Thread\n");

    int ret = ksceKernelStartThread(readThreadId, 0, 0);
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

  if(readThreadId >= 0)
  {
    int waitRet = 0;
    ksceKernelWaitThreadEnd(readThreadId, &waitRet, 0);
    
    int delret = ksceKernelDeleteThread(readThreadId);
  }

  if(req_sema >= 0)
    ksceKernelDeleteMutex(req_sema);

  if(resp_sema >= 0)
    ksceKernelDeleteMutex(resp_sema);
  
  return SCE_KERNEL_STOP_SUCCESS;
}