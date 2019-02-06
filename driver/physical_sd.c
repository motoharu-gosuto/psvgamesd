/* physical_sd.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "physical_sd.h"

#include "hook_ids.h"
#include "global_log.h"
#include "cmd56_key.h"
#include "sector_api.h"
#include "utils.h"
#include "reader.h"
#include "psv_types.h"
#include "media_id_emu.h"
#include "sd_emu.h"
#include "defines.h"

#include <taihen.h>

#include <stdio.h>
#include <string.h>

// ======= img header and mbr init for physical mode =======

char g_img_header_sd_raw_data[SD_DEFAULT_SECTOR_SIZE] = {0};
psv_file_header_v1* g_img_header_sd = 0;

char g_mbr_sd_raw_data[SD_DEFAULT_SECTOR_SIZE] = {0};
MBR* g_mbr_sd = 0;

int initialize_img_header()
{
  if(g_img_header_sd > 0)
    return 0;

  sd_context_part_sd* sd_ctx = ksceSdifGetSdContextPartValidateSd(SCE_SDIF_DEV_GAME_CARD);
  if(sd_ctx <= 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Failed to get context part\n");
    #endif

    return -1;
  }

  //when image header is not initialized - data will be read from real 0 sector
  int res = ksceSdifReadSectorSd(sd_ctx, 0, g_img_header_sd_raw_data, 1);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Failed to read sector\n");
    #endif

    return -1;
  }

  psv_file_header_base* header_base = (psv_file_header_base*)g_img_header_sd_raw_data;

  if(header_base->magic != PSV_MAGIC || header_base->version != PSV_VERSION_V1)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("ISO magic or version is invalid\n");
    #endif

    return -1;
  }

  g_img_header_sd = (psv_file_header_v1*)g_img_header_sd_raw_data;

  return 0;
}

int deinitialize_img_header()
{
  memset(g_img_header_sd_raw_data, 0, SD_DEFAULT_SECTOR_SIZE);

  g_img_header_sd = 0;
  return 0;
}

int initialize_mbr_header()
{
  if(g_img_header_sd == 0)
    return -1;

  if(g_mbr_sd > 0)
    return 0;

  sd_context_part_sd* sd_ctx = ksceSdifGetSdContextPartValidateSd(SCE_SDIF_DEV_GAME_CARD);
  if(sd_ctx <= 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Failed to get context part\n");
    #endif

    return -1;
  }

  //when image header is initialized data will be read from g_img_header_sd->image_offset_sector
  int res = ksceSdifReadSectorSd(sd_ctx, 0, g_mbr_sd_raw_data, 1);
  if(res < 0)
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Failed to read sector\n");
    #endif

    return -1;
  }

  g_mbr_sd = (MBR*)g_mbr_sd_raw_data;

  return 0;
}

int deinitialize_mbr_header()
{
  memset(g_mbr_sd_raw_data, 0, SD_DEFAULT_SECTOR_SIZE);

  g_mbr_sd = 0;
  return 0;
}

// ======= other code =======

//sd read operation hook that redirects directly to sd card (physical read)
int sd_read_hook_through(void* ctx_part, int sector, char* buffer, int nSectors)
{
  //make sure that only sd operations are redirected
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ((sd_context_part_base*)ctx_part)->gctx_ptr)
  {
    //first (internal) read that will be requested will initialize image header - at this moment we dont have mbr initialied
    if(g_img_header_sd > 0)
    {
      //second (internal) read that will be requested will initialize mbr - at this point we can start emulating mediaid
      if(g_mbr_sd > 0)
      {
        //check if media-id read is requested
        int media_id_res = read_media_id(g_mbr_sd, sector, buffer, nSectors);
        if(media_id_res > 0)
          return 0;
      }
    }

    //this code may cause deadlocks so moved under comment
    /*
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "enter sd read sector %x nSectors %x\n", sector, nSectors);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    */

    //can add debug code here
    int res = TAI_CONTINUE(int, sd_read_hook_ref, ctx_part, sector, buffer, nSectors);

    //this code may cause deadlocks so moved under comment
    /*
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "exit sd read sector %x nSectors %x\n", sector, nSectors);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif
    */

    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, sd_read_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
}

int sd_write_hook_physical(void* ctx_part, int sector, char* buffer, int nSectors)
{
  //make sure that only mmc operations are redirected
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ((sd_context_part_base*)ctx_part)->gctx_ptr)
  {
     //first (internal) read that will be requested will initialize image header - at this moment we dont have mbr initialied
     if(g_img_header_sd > 0)
     {
        //second (internal) read that will be requested will initialize mbr - at this point we can start emulating mediaid
        if(g_mbr_sd > 0)
        {
          int media_id_res = write_media_id(g_mbr_sd, sector, buffer, nSectors);
          if(media_id_res > 0)
            return 0;
        }
    }

    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Write operation is not supported\n");
    #endif

    memset(buffer, 0, nSectors * SD_DEFAULT_SECTOR_SIZE);
    return SD_UNKNOWN_READ_WRITE_ERROR;
  }
  else
  {
    int res = TAI_CONTINUE(int, sd_write_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
}

//this hook modifies offset to data that is sent to the card
//this is done only for game card device by checking ctx pointer with sd api
//this is done only for commands CMD17 (read) and CMD18 (read multiple)
//                               CMD24 (write) and CMD25 (write multiple)
int send_command_hook(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    if(cmd_data1->command == READ_SINGLE_BLOCK || cmd_data1->command == READ_MULTIPLE_BLOCK)
    {
      //add image offset if image header is initialized
      if(g_img_header_sd > 0)
      {
        cmd_data1->argument = cmd_data1->argument + g_img_header_sd->image_offset_sector;
      }
    }

    if(cmd_data1->command == WRITE_BLOCK || cmd_data1->command == WRITE_MULTIPLE_BLOCK)
    {
      //add image offset if image header is initialized
      if(g_img_header_sd > 0)
      {
        cmd_data1->argument = cmd_data1->argument + g_img_header_sd->image_offset_sector;
      }
    }

    #ifdef ENABLE_COMMAND_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "enter CMD%d \n", cmd_data1->command);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    print_cmd(cmd_data1, 1, "before");
    #endif

    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);

    #ifdef ENABLE_COMMAND_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "exit CMD%d \n", cmd_data1->command);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif

    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);
    return res;
  }
}

//this is a hook for sd init operation (mmc init operation is different)
//we need to imitate cmd56 handshake after init
//this is done by writing last cmd56 packet to correct location of GcAuthMgr module
//we hook this function instead of gc because gc funciton is not called
int init_sd_hook_physical(int sd_ctx_index, void** ctx_part)
{
  if(sd_ctx_index == SCE_SDIF_DEV_GAME_CARD)
  {
    int res = TAI_CONTINUE(int, init_sd_hook_ref, sd_ctx_index, ctx_part);

    //initialize img header after card is initialized and we can read it
    int img_res = initialize_img_header();
    #ifdef ENABLE_DEBUG_LOG
    if(img_res < 0)
    {
      FILE_GLOBAL_WRITE_LEN("Failed to initialize img\n");
    }
    #endif

    int mbr_res = initialize_mbr_header();
    #ifdef ENABLE_DEBUG_LOG
    if(mbr_res < 0)
    {
      FILE_GLOBAL_WRITE_LEN("Failed to initialize mbr\n");
    }
    #endif

    //get data from img header
    char data_5018_buffer[CMD56_DATA_SIZE];
    get_cmd56_data_base(g_img_header_sd, data_5018_buffer);

    //set data in gc memory
    set_5018_data(data_5018_buffer);

    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, init_sd_hook_ref, sd_ctx_index, ctx_part);
    return res;
  }
}

int initialize_hooks_physical_sd()
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0)
  {
    //read hook can be used for some debugging
    sd_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sd_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0xb9593652, sd_read_hook_through);

    #ifdef ENABLE_DEBUG_LOG
    if(sd_read_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init sd_read_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init sd_read_hook\n");
    #endif

    //write hook to emulate media-id partition
    sd_write_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sd_write_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0xe0781171, sd_write_hook_physical);

    #ifdef ENABLE_DEBUG_LOG
    if(sd_write_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init sd_write_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init sd_write_hook\n");
    #endif

    //patch for proc_initialize_generic_X - so that sd card type is not ignored
    char zeroCallOnePatch[4] = {0x01, 0x20, 0x00, 0xBF};

    //this patch enables initialization in partition table related subroutines
    gen_init_1_patch_uid = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x2022, zeroCallOnePatch, 4); //patch (BLX) to (MOVS R0, #1 ; NOP)

    #ifdef ENABLE_DEBUG_LOG
    if(gen_init_1_patch_uid < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init gen_init_1_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init gen_init_1_patch\n");
    #endif

    //this patch enables generic initialization on insert
    gen_init_2_patch_uid = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x2498, zeroCallOnePatch, 4); //patch (BLX) to (MOVS R0, #1 ; NOP)

    #ifdef ENABLE_DEBUG_LOG
    if(gen_init_2_patch_uid < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init gen_init_2_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init gen_init_2_patch\n");
    #endif

    //this patch enables initialization on resume
    gen_init_3_patch_uid = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x2940, zeroCallOnePatch, 4); //patch (BLX) to (MOVS R0, #1 ; NOP)

    #ifdef ENABLE_DEBUG_LOG
    if(gen_init_3_patch_uid < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init gen_init_3_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init gen_init_3_patch\n");
    #endif
  }

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    #ifdef ENABLE_SD_LOW_SPEED_PATCH
    //this patch modifies CMD6 argument to check for availability of low speed mode instead of high speed mode
    char lowSpeed_check[4] = {0xF0, 0xFF, 0xFF, 0x00};
    hs_dis_patch1_uid = taiInjectDataForKernel(KERNEL_PID, sdif_info.modid, 0, 0x6B34, lowSpeed_check, 4); //0x06, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xFF, 0x00

    #ifdef ENABLE_DEBUG_LOG
    if(hs_dis_patch1_uid < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init hs_dis_patch1\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init hs_dis_patch1\n");
    #endif

    //this patch modifies CMD6 argument to set low speed mode instead of high speed mode
    char lowSpeed_set[4] = {0xF0, 0xFF, 0xFF, 0x80};
    hs_dis_patch2_uid = taiInjectDataForKernel(KERNEL_PID, sdif_info.modid, 0, 0x6B54, lowSpeed_set, 4); //0x06, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xFF, 0x80

    #ifdef ENABLE_DEBUG_LOG
    if(hs_dis_patch2_uid < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init hs_dis_patch2\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init hs_dis_patch2\n");
    #endif

    #endif

    //this hooks sd init function (there is separate function for mmc init)
    //this hook is used to set cmd56 handshake data
    init_sd_hook_id = taiHookFunctionExportForKernel(KERNEL_PID, &init_sd_hook_ref, "SceSdif", SceSdifForDriver_NID, 0xc1271539, init_sd_hook_physical);

    #ifdef ENABLE_DEBUG_LOG
    if(init_sd_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init init_sd_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init init_sd_hook\n");
    #endif

    //this hooks command send function which is the main function for executing all commands that are sent from Vita to SD/MMC devices
    //this hook is used to fix cmd17/cmd18 sector offset
    send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_hook);

    #ifdef ENABLE_DEBUG_LOG
    if(send_command_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init send_command_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init send_command_hook\n");
    #endif
  }

  init_media_id_emu();

  return 0;
}

int deinitialize_hooks_physical_sd()
{
  if(sd_read_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(sd_read_hook_id, sd_read_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit sd_read_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit sd_read_hook\n");
    #endif

    sd_read_hook_id = -1;
  }

  if(sd_write_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(sd_write_hook_id, sd_write_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit sd_write_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit sd_write_hook\n");
    #endif

    sd_write_hook_id = -1;
  }

  if(gen_init_1_patch_uid >= 0)
  {
    int res = taiInjectReleaseForKernel(gen_init_1_patch_uid);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit gen_init_1_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit gen_init_1_patch\n");
    #endif

    gen_init_1_patch_uid = -1;
  }

  if(gen_init_2_patch_uid >= 0)
  {
    int res = taiInjectReleaseForKernel(gen_init_2_patch_uid);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit gen_init_2_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit gen_init_2_patch\n");
    #endif

    gen_init_2_patch_uid = -1;
  }

  if(gen_init_3_patch_uid >= 0)
  {
    int res = taiInjectReleaseForKernel(gen_init_3_patch_uid);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit gen_init_3_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit gen_init_3_patch\n");
    #endif

    gen_init_3_patch_uid = -1;
  }

  if(hs_dis_patch1_uid >= 0)
  {
    int res = taiInjectReleaseForKernel(hs_dis_patch1_uid);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit hs_dis_patch1\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit hs_dis_patch1\n");
    #endif

    hs_dis_patch1_uid = -1;
  }

  if(hs_dis_patch2_uid >= 0)
  {
    int res = taiInjectReleaseForKernel(hs_dis_patch2_uid);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit hs_dis_patch2\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit hs_dis_patch2\n");
    #endif

    hs_dis_patch2_uid = -1;
  }

  if(init_sd_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(init_sd_hook_id, init_sd_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit init_sd_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit init_sd_hook\n");
    #endif

    init_sd_hook_id = -1;
  }

  if(send_command_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(send_command_hook_id, send_command_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit send_command_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit send_command_hook\n");
    #endif

    send_command_hook_id = -1;
  }

  deinitialize_mbr_header();
  deinitialize_img_header();
  deinit_media_id_emu();

  return 0;
}
