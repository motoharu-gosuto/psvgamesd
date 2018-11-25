/* physical_mmc.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "physical_mmc.h"

#include "hook_ids.h"
#include "global_log.h"
#include "sector_api.h"
#include "utils.h"
#include "defines.h"

#include <taihen.h>

int mmc_read_hook_through(void* ctx_part, int	sector,	char* buffer, int nSectors)
{
  //make sure that only mmc operations are redirected
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ((sd_context_part_base*)ctx_part)->gctx_ptr)
  {
    //can add debug code here

    #ifdef ENABLE_DEBUG_LOG
    // this log will cause deadlock when game is started
    /*
    snprintf(sprintfBuffer, 256, "enter mmc read sector %x nSectors %x\n", sector, nSectors);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    */
    #endif

    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector, buffer, nSectors);

    #ifdef ENABLE_DEBUG_LOG
    // this log will cause deadlock when game is started
    /*
    snprintf(sprintfBuffer, 256, "exit mmc read sector %x nSectors %x\n", sector, nSectors);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    */
    #endif

    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
}

int send_command_debug_hook(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    //can add debug code here

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


    //can add debug code here

    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);
    return res;
  }
}

//originally this function clears cmd56 handshake data
//we need to leave the data as is in order to be able to copy it later
//we do it by returning 0 without calling TAI_CONTINUE
int clear_sensitive_data_hook()
{
  return 0;
}

//aka Blackfin patch - patches timing check during CMD56 handshake
//the only place where time function is used is cmd56 handshake
//instead of patching handshake code it is easier to return 0 from time function
//this wil mean that time gap is 0
//this is usefull for debugging CMD56 handshake
//this patch is only relevant to physical mmc mode
int64_t sys_wide_time_hook()
{
  return 0;
}

int initialize_hooks_physical_mmc()
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0)
  {
    //read hook can be used for some debugging
    mmc_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &mmc_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0x6f8d529b, mmc_read_hook_through);

    #ifdef ENABLE_DEBUG_LOG
    if(mmc_read_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init mmc_read_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init mmc_read_hook\n");
    #endif
  }

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    //mmc command hook can be used for some debugging
    send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_debug_hook);

    #ifdef ENABLE_DEBUG_LOG
    if(send_command_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init send_command_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init send_command_hook\n");
    #endif
  }

  tai_module_info_t gc_info;
  gc_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSblGcAuthMgr", &gc_info) >= 0)
  {
    clear_sensitive_data_hook_id = taiHookFunctionExportForKernel(KERNEL_PID, &clear_sensitive_data_hook_ref, "SceSblGcAuthMgr", SceSblGcAuthMgrDrmBBForDriver_NID, 0xBB451E83, clear_sensitive_data_hook);

    #ifdef ENABLE_DEBUG_LOG
    if(clear_sensitive_data_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init clear_sensitive_data_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init clear_sensitive_data_hook\n");
    #endif

    sys_wide_time_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &sys_wide_time_hook_ref, "SceSblGcAuthMgr", 0xE2C40624, 0xF4EE4FA9, sys_wide_time_hook);

    #ifdef ENABLE_DEBUG_LOG
    if(sys_wide_time_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init sys_wide_time_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init sys_wide_time_hook\n");
    #endif
  }

  return 0;
}

int deinitialize_hooks_physical_mmc()
{
  if(mmc_read_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(mmc_read_hook_id, mmc_read_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit mmc_read_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit mmc_read_hook\n");
    #endif

    mmc_read_hook_id = -1;
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

  if(sys_wide_time_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(sys_wide_time_hook_id, sys_wide_time_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit sys_wide_time_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit sys_wide_time_hook\n");
    #endif

    sys_wide_time_hook_id = -1;
  }

  if(clear_sensitive_data_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(clear_sensitive_data_hook_id, clear_sensitive_data_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit clear_sensitive_data_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit clear_sensitive_data_hook\n");
    #endif

    clear_sensitive_data_hook_id = -1;
  }

  return 0;
}
