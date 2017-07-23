#include "physical_sd.h"

#include "hook_ids.h"
#include "global_log.h"
#include "cmd56_key.h"
#include "sector_api.h"
#include "utils.h"

#include "defines.h"

#include <taihen.h>
#include <module.h>

//sd read operation hook that redirects directly to sd card (physical read)
int sd_read_hook_through(void* ctx_part, int sector, char* buffer, int nSectors)
{
  //make sure that only sd operations are redirected
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ((sd_context_part_base*)ctx_part)->gctx_ptr)
  {
    //can add debug code here
    int res = TAI_CONTINUE(int, sd_read_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, sd_read_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
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

    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);
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
    set_5018_data();
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
    
    //patch for proc_initialize_generic_2 - so that sd card type is not ignored
    char zeroCallOnePatch[4] = {0x01, 0x20, 0x00, 0xBF};
    gen_init_2_patch_uid = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x2498, zeroCallOnePatch, 4); //patch (BLX) to (MOVS R0, #1 ; NOP)
  }

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    #ifdef ENABLE_SD_LOW_SPEED_PATCH
    //this patch modifies CMD6 argument to check for availability of low speed mode instead of high speed mode
    char lowSpeed_check[4] = {0xF0, 0xFF, 0xFF, 0x00};
    hs_dis_patch1_uid = taiInjectDataForKernel(KERNEL_PID, sdif_info.modid, 0, 0x6B34, lowSpeed_check, 4); //0x06, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xFF, 0x00

    //this patch modifies CMD6 argument to set low speed mode instead of high speed mode
    char lowSpeed_set[4] = {0xF0, 0xFF, 0xFF, 0x80};
    hs_dis_patch2_uid = taiInjectDataForKernel(KERNEL_PID, sdif_info.modid, 0, 0x6B54, lowSpeed_set, 4); //0x06, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xFF, 0x80
    #endif
    
    //this hooks sd init function (there is separate function for mmc init)
    //this hook is used to set cmd56 handshake data
    init_sd_hook_id = taiHookFunctionExportForKernel(KERNEL_PID, &init_sd_hook_ref, "SceSdif", SceSdifForDriver_NID, 0xc1271539, init_sd_hook_physical);

    //this hooks command send function which is the main function for executing all commands that are sent from Vita to SD/MMC devices
    //this hook is used to fix cmd17/cmd18 sector offset
    send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_hook);
  }

  return 0;
}

int deinitialize_hooks_physical_sd()
{
  if(sd_read_hook_id >= 0)
  {
   taiHookReleaseForKernel(sd_read_hook_id, sd_read_hook_ref);
   sd_read_hook_id = -1;
  }
  
  if(gen_init_2_patch_uid >= 0)
  {
    taiInjectReleaseForKernel(gen_init_2_patch_uid);
    gen_init_2_patch_uid = -1;
  }

  if(hs_dis_patch1_uid >= 0)
  {
    taiInjectReleaseForKernel(hs_dis_patch1_uid);
    hs_dis_patch1_uid = -1;
  }

  if(hs_dis_patch2_uid >= 0)
  {
    taiInjectReleaseForKernel(hs_dis_patch2_uid);
    hs_dis_patch2_uid = -1;
  }

  if(init_sd_hook_id >= 0)
  {
    taiHookReleaseForKernel(init_sd_hook_id, init_sd_hook_ref);
    init_sd_hook_id = -1;
  }

  if(send_command_hook_id >= 0)
  {
    taiHookReleaseForKernel(send_command_hook_id, send_command_hook_ref);
    send_command_hook_id = -1;
  }

  return 0;
}