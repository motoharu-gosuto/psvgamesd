 #include "virtual_sd.h"

 #include <psp2kern/kernel/threadmgr.h>

 #include "hook_ids.h"
 #include "functions.h"
 #include "reader.h"
 #include "global_log.h"
 #include "sector_api.h"
 #include "sd_emu.h"
 #include "cmd56_key.h"
 #include "ins_rem_card.h"

 #include "defines.h"

//sd read operation can be redirected to file only in separate thread
//it looks like file i/o api causes some internal locks/conflicts
//when called from deep inside of Sdif driver subroutines
int sd_read_hook_threaded(void* ctx_part, int sector, char* buffer, int nSectors)
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
    int res = TAI_CONTINUE(int, sd_read_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
}

int send_command_hook_emu(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    //can add debug code here
    
    int res = emulate_sd_command(ctx, cmd_data1, cmd_data2, nIter, num);

    //can add debug code here

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
int init_sd_hook_virtual(int sd_ctx_index, void** ctx_part)
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

int initialize_hooks_virtual_sd()
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0) 
  {
    //read hook to readirect read operations to iso
    sd_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sd_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0xb9593652, sd_read_hook_threaded);
    
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
    init_sd_hook_id = taiHookFunctionExportForKernel(KERNEL_PID, &init_sd_hook_ref, "SceSdif", SceSdifForDriver_NID, 0xc1271539, init_sd_hook_virtual);

    //this hooks command send function which is the main function for executing all commands that are sent from Vita to SD/MMC devices
    //this hook is used to emulate commands
    send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_hook_emu);
  }

  initialize_ins_rem();

  return 0;
}

int deinitialize_hooks_virtual_sd()
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

  deinitialize_ins_rem();
  
  return 0;
}