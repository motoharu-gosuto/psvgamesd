#include "ins_rem_card.h"

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

//this file is used to control insertion and removal of card in virtual mode

int g_gc_inserted = 0;

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

//block physical insertion for game card
int insert_handler_hook(int unk, interrupt_argument* arg)
{
  if(arg->intr_table_index == SCE_SDSTOR_SDIF1_INDEX)
  {
    return 0;
  }
  else
  {
    return TAI_CONTINUE(int, insert_handler_hook_ref, unk, arg);
  }
}

//block physical removal for game card
int remove_handler_hook(int unk, interrupt_argument* arg)
{
  if(arg->intr_table_index == SCE_SDSTOR_SDIF1_INDEX)
  {
    return 0;
  }
  else
  {
    return TAI_CONTINUE(int, remove_handler_hook_ref, unk, arg);
  }
}

//does the same functionality as insert interrupt handler and emulates the interrupt
//we need emulation because we block original insert / remove handlers that handle physical insertion
//this is to forbid physical insertion of card in virtual mode
int insert_game_card_emu()
{
  interrupt_argument* ia = get_int_arg(SCE_SDSTOR_SDIF1_INDEX);
  if(ia <= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Signal insert failed\n");
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("Signal insert\n");

  g_gc_inserted = 1;
  return ksceKernelSetEventFlag(ia->SceSdstorRequest_evid, CARD_INSERT_SDSTOR_REQUEST_EVENT_FLAG);
}

//does the same functionality as remove interrupt handler and emulates the interrupt
//we need emulation because we block original insert / remove handlers that handle physical insertion
//this is to forbid physical insertion of card in virtual mode
int remove_game_card_emu()
{
  interrupt_argument* ia = get_int_arg(SCE_SDSTOR_SDIF1_INDEX);
  if(ia <= 0)
  {
    FILE_GLOBAL_WRITE_LEN("Signal remove failed\n");
    return -1;
  }

  FILE_GLOBAL_WRITE_LEN("Signal remove\n");

  g_gc_inserted = 0;
  return ksceKernelSetEventFlag(ia->SceSdstorRequest_evid, CARD_REMOVE_SDSTOR_REQUEST_EVENT_FLAG);
}

//this function emulates physical insertion signal
//since card is not connected - reading insert state from DMA mapped region will always return 0
//that is why insertion is emulated with g_gc_inserted variable
int get_insert_state_hook(sd_context_global* ctx)
{
  if(ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD) == ctx)
  {
    //you shoud NOT use any file i/o for logging inside this handler
    //using file i/o will cause deadlock
    return g_gc_inserted;
  }
  else
  {
    return TAI_CONTINUE(int, get_insert_state_hook_ref, ctx);
  }
}

int initialize_ins_rem()
{
  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0) 
  {
    insert_handler_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &insert_handler_hook_ref, sdstor_info.modid, 0, 0x3BD4, 1, insert_handler_hook);

    remove_handler_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &remove_handler_hook_ref, sdstor_info.modid, 0, 0x3BC8, 1, remove_handler_hook);
  }

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    get_insert_state_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &get_insert_state_hook_ref, sdif_info.modid, 0, 0xC84, 1, get_insert_state_hook);
  }

  return 0;
}

int deinitialize_ins_rem()
{
  if(insert_handler_hook_id >= 0)
  {
    taiHookReleaseForKernel(insert_handler_hook_id, insert_handler_hook_ref);
    insert_handler_hook_id = -1;
  }

  if(remove_handler_hook_id >= 0)
  {
    taiHookReleaseForKernel(remove_handler_hook_id, remove_handler_hook_ref);
    remove_handler_hook_id = -1;
  }

  if(get_insert_state_hook_id >= 0)
  {
    taiHookReleaseForKernel(get_insert_state_hook_id, get_insert_state_hook_ref);
    get_insert_state_hook_id = -1;
  }

  return 0;
}