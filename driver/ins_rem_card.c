/* ins_rem_card.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "ins_rem_card.h"

#include <psp2kern/kernel/threadmgr.h>

#include <taihen.h>

#include "hook_ids.h"
#include "functions.h"
#include "reader.h"
#include "global_log.h"
#include "cmd56_key.h"
#include "sector_api.h"
#include "mmc_emu.h" 
#include "defines.h"

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
      #ifdef ENABLE_DEBUG_LOG
      FILE_GLOBAL_WRITE_LEN("get_int_arg failed\n");
      #endif
      return 0;
    }
    else
    {
      return int_arg;
    }
  }
  else
  {
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("get_int_arg failed\n");
    #endif
    return 0;
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
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Signal insert failed\n");
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Signal insert\n");
  #endif

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
    #ifdef ENABLE_DEBUG_LOG
    FILE_GLOBAL_WRITE_LEN("Signal remove failed\n");
    #endif
    return -1;
  }

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("Signal remove\n");
  #endif

  g_gc_inserted = 0;
  return ksceKernelSetEventFlag(ia->SceSdstorRequest_evid, CARD_REMOVE_SDSTOR_REQUEST_EVENT_FLAG);
}

//block physical insertion for game card in virtual mode
int insert_handler_hook(int unk, interrupt_argument* arg)
{
  if(arg->intr_table_index == SCE_SDSTOR_SDIF1_INDEX)
  {
    //you shoud NOT use any file i/o for logging inside this handler
    //using file i/o will cause deadlock
    return 0;
  }
  else
  {
    interrupt_argument* ia = get_int_arg(arg->intr_table_index);
    return ksceKernelSetEventFlag(ia->SceSdstorRequest_evid, CARD_INSERT_SDSTOR_REQUEST_EVENT_FLAG);
  }
}

//block physical removal for game card in virtual mode
int remove_handler_hook(int unk, interrupt_argument* arg)
{
  if(arg->intr_table_index == SCE_SDSTOR_SDIF1_INDEX)
  {
    //you shoud NOT use any file i/o for logging inside this handler
    //using file i/o will cause deadlock
    return 0;
  }
  else
  {
    interrupt_argument* ia = get_int_arg(arg->intr_table_index);
    return ksceKernelSetEventFlag(ia->SceSdstorRequest_evid, CARD_REMOVE_SDSTOR_REQUEST_EVENT_FLAG);
  }
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
    void* insert_handler_pointer = &insert_handler_hook;
    char* ins_data = (char*)(&insert_handler_pointer);
    char far_jump_ins_patch[8] = {0xDF, 0xF8, 0x00, 0xF0, ins_data[0], ins_data[1], ins_data[2], ins_data[3]}; // LDR.W PC, off_ where off_ is next 4 bytes
    insert_handler_patch_id = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x3BD4, far_jump_ins_patch, 8);

    #ifdef ENABLE_DEBUG_LOG
    if(insert_handler_patch_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init insert_handler_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init insert_handler_patch\n");
    #endif

    void* remove_handler_pointer = &remove_handler_hook;
    char* rem_data = (char*)(&remove_handler_pointer);
    char far_jump_rem_patch[8] = {0xDF, 0xF8, 0x00, 0xF0, rem_data[0], rem_data[1], rem_data[2], rem_data[3]}; // LDR.W PC, off_ where off_ is next 4 bytes
    remove_handler_patch_id = taiInjectDataForKernel(KERNEL_PID, sdstor_info.modid, 0, 0x3BC8, far_jump_rem_patch, 8);

    #ifdef ENABLE_DEBUG_LOG
    if(remove_handler_patch_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init remove_handler_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init remove_handler_patch\n");
    #endif
  }

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    get_insert_state_hook_id = taiHookFunctionOffsetForKernel(KERNEL_PID, &get_insert_state_hook_ref, sdif_info.modid, 0, 0xC84, 1, get_insert_state_hook);

    #ifdef ENABLE_DEBUG_LOG
    if(get_insert_state_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init get_insert_state_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init get_insert_state_hook\n");
    #endif
  }

  return 0;
}

int deinitialize_ins_rem()
{
  if(insert_handler_patch_id >= 0)
  {
    int res = taiInjectReleaseForKernel(insert_handler_patch_id);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit insert_handler_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit insert_handler_patch\n");
    #endif

    insert_handler_patch_id = -1;
  }

  if(remove_handler_patch_id >= 0)
  {
    int res = taiInjectReleaseForKernel(remove_handler_patch_id);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit remove_handler_patch\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit remove_handler_patch\n");
    #endif

    remove_handler_patch_id = -1;
  }

  if(get_insert_state_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(get_insert_state_hook_id, get_insert_state_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit get_insert_state_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit get_insert_state_hook\n");
    #endif

    get_insert_state_hook_id = -1;
  }

  return 0;
}
