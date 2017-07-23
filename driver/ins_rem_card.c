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

insert_handler* sceSdifInsertHandler = 0;
remove_handler* sceSdifRemoveHandler = 0; 

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

//calls original insert interrupt handler
int insert_game_card()
{
  if(sceSdifInsertHandler > 0)
  {
    FILE_GLOBAL_WRITE_LEN("Signal insert\n");

    g_gc_inserted = 1;
    return sceSdifInsertHandler(0, get_int_arg(SCE_SDSTOR_SDIF1_INDEX));
  }
  else
  {
    FILE_GLOBAL_WRITE_LEN("Signal insert failed\n");
    return -1;
  }
}

//calls original remove interrupt handler
int remove_game_card()
{
  if(sceSdifRemoveHandler > 0)
  {
    FILE_GLOBAL_WRITE_LEN("Signal remove\n");

    g_gc_inserted = 0;
    return sceSdifRemoveHandler(0, get_int_arg(SCE_SDSTOR_SDIF1_INDEX));
  }
  else
  {
    FILE_GLOBAL_WRITE_LEN("Signal remove failed\n");
    return -1;
  }
}

//does the same functionality as insert interrupt handler and emulates the interrupt
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
    //initialize card insert handler
    module_get_offset(KERNEL_PID, sdstor_info.modid, 0, 0x3BD5, (uintptr_t*)&sceSdifInsertHandler);

    //get card remove handler
    module_get_offset(KERNEL_PID, sdstor_info.modid, 0, 0x3BC9, (uintptr_t*)&sceSdifRemoveHandler);
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
  if(get_insert_state_hook_id >= 0)
  {
    taiHookReleaseForKernel(get_insert_state_hook_id, get_insert_state_hook_ref);
    get_insert_state_hook_id = -1;
  }

  return 0;
}