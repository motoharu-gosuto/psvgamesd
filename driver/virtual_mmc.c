#include "virtual_mmc.h"

#include <psp2kern/kernel/threadmgr.h>

#include <taihen.h>
#include <module.h>

#include "hook_ids.h"
#include "functions.h"
#include "reader.h"
#include "global_log.h"
#include "cmd56_key.h"
#include "sector_api.h"
#include "mmc_emu.h"
#include "ins_rem_card.h"

//redirect read operations to separate thread
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
    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
}

//emulate mmc command
int send_command_emu_hook(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    //can add debug code here

    int res = emulate_mmc_command(ctx, cmd_data1, cmd_data2, nIter, num);
    
    //can add debug code here

    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);
    return res;
  }
}

//override cmd56 handshake with dumped keys
//this function is called only for game cards in mmc mode
int gc_cmd56_handshake_override_hook(int param0)
{
  //get data from iso
  char data_5018_buffer[0x34];
  get_cmd56_data(data_5018_buffer);

  //set data in gc memory
  set_5018_data(data_5018_buffer);

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("override cmd56 handshake\n");
  #endif

  return 0;
}

int initialize_hooks_virtual_mmc()
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0) 
  {
    //override cmd56 handshake with dumped keys
    gc_cmd56_handshake_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &gc_cmd56_handshake_hook_ref, "SceSdstor", SceSblGcAuthMgrGcAuthForDriver_NID, 0x68781760, gc_cmd56_handshake_override_hook);

    //redirect read operations to separate thread
    mmc_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &mmc_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0x6f8d529b, mmc_read_hook_threaded);
  }

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_emu_hook);
  }

  initialize_ins_rem();

  return 0;
}

int deinitialize_hooks_virtual_mmc()
{
  if(gc_cmd56_handshake_hook_id >= 0)
  {
    taiHookReleaseForKernel(gc_cmd56_handshake_hook_id, gc_cmd56_handshake_hook_ref);
    gc_cmd56_handshake_hook_id = -1;
  }

  if(mmc_read_hook_id >= 0)
  {
    taiHookReleaseForKernel(mmc_read_hook_id, mmc_read_hook_ref);
    mmc_read_hook_id = -1;
  }

  if(send_command_hook_id >= 0)
  {
    taiHookReleaseForKernel(send_command_hook_id, send_command_hook_ref);
    send_command_hook_id = -1;
  }

  deinitialize_ins_rem();

  return 0;
}