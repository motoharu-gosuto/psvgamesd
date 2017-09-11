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
 #include "utils.h"

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
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to ksceKernelLockMutex resp_lock : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif

    //wait for response
    res = sceKernelWaitCondForDriver(resp_cond, 0);
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to sceKernelWaitCondForDriver resp_cond : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif

    //unlock mutex
    res = ksceKernelUnlockMutex(resp_lock, 1);
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to ksceKernelUnlockMutex resp_lock : %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    #endif

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
    
    #ifdef ENABLE_COMMAND_DEBUG_LOG
    print_cmd(cmd_data1, 1, "before");
    #endif
    
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

    //get data from iso
    char data_5018_buffer[0x34];
    get_cmd56_data(data_5018_buffer);

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

int initialize_hooks_virtual_sd()
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0) 
  {
    //read hook to readirect read operations to iso
    sd_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sd_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0xb9593652, sd_read_hook_threaded);
    
    #ifdef ENABLE_DEBUG_LOG
    if(sd_read_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init sd_read_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init sd_read_hook\n");
    #endif
    
    //patch for proc_initialize_generic_2 - so that sd card type is not ignored
    char zeroCallOnePatch[4] = {0x01, 0x20, 0x00, 0xBF};
    gen_init_2_patch_uid = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x2498, zeroCallOnePatch, 4); //patch (BLX) to (MOVS R0, #1 ; NOP)

    #ifdef ENABLE_DEBUG_LOG
    if(gen_init_2_patch_uid < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init gen_init_2_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init gen_init_2_patch\n");
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
    init_sd_hook_id = taiHookFunctionExportForKernel(KERNEL_PID, &init_sd_hook_ref, "SceSdif", SceSdifForDriver_NID, 0xc1271539, init_sd_hook_virtual);

    #ifdef ENABLE_DEBUG_LOG
    if(init_sd_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init init_sd_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init init_sd_hook\n");
    #endif

    //this hooks command send function which is the main function for executing all commands that are sent from Vita to SD/MMC devices
    //this hook is used to emulate commands
    send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_hook_emu);

    #ifdef ENABLE_DEBUG_LOG
    if(send_command_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init send_command_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init send_command_hook\n");
    #endif
  }

  initialize_ins_rem();

  return 0;
}

int deinitialize_hooks_virtual_sd()
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

  deinitialize_ins_rem();
  
  return 0;
}