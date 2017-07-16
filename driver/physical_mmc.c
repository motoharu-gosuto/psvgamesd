#include "physical_mmc.h"

#include "hook_ids.h"
#include "global_log.h"
#include "cmd56_key.h"
#include "sector_api.h"

#include <taihen.h>
#include <module.h>

int mmc_read_hook_through(void* ctx_part, int	sector,	char* buffer, int nSectors)
{
  //make sure that only mmc operations are redirected
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ((sd_context_part_base*)ctx_part)->gctx_ptr)
  {
    //can add debug code here

    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, mmc_read_hook_ref, ctx_part, sector, buffer, nSectors);
    return res;
  }
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

int send_command_debug_hook(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    //there is a timing check for cmd56 in gcauthmgr so can not do much logging here (since it is slow)
    //i know where this check is but it is not worth patching right now since cmd56 auth can be bypassed
    //commands except cmd56 can be logged

    //can add debug code here

    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);

    //can add debug code here

    return res;
  }
  else
  {
    int res = TAI_CONTINUE(int, send_command_hook_ref, ctx, cmd_data1, cmd_data2, nIter, num);
    return res;
  }
}

//this is a hook of GcAuth function that performs cmd56 handshake
int gc_cmd56_handshake_hook(int param0)
{
  int res = TAI_CONTINUE(int, gc_cmd56_handshake_hook_ref, param0);

  get_5018_data();
  
  return res;
}
 
int initialize_hooks_physical_mmc()
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0) 
  {
    //cmd56 handshake hook is used to dump cmd56 handshake data (keys)
    gc_cmd56_handshake_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &gc_cmd56_handshake_hook_ref, "SceSdstor", SceSblGcAuthMgrGcAuthForDriver_NID, 0x68781760, gc_cmd56_handshake_hook);

    //read hook can be used for some debugging
    mmc_read_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &mmc_read_hook_ref, "SceSdstor", SceSdifForDriver_NID, 0x6f8d529b, mmc_read_hook_through);
  }

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    //mmc command hook can be used for some debugging
    send_command_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &send_command_hook_ref, sdif_info.modid, 0, 0x17E8, 1, send_command_debug_hook);
  }

  return 0;
}

int deinitialize_hooks_physical_mmc()
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

  return 0;
}