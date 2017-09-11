#include "global_hooks.h"

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/types.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/sysmem.h>

#include <taihen.h>
#include <module.h>

#include <string.h>

#include "hook_ids.h"
#include "defines.h"
#include "sector_api.h"
#include "global_log.h"

SceUID SceSdif1_lock = -1;

fast_mutex* get_gc_mutex()
{
  sd_context_global* ctx_g = ksceSdifGetSdContextGlobal(SCE_SDIF_DEV_GAME_CARD);
  if(ctx_g <= 0)
    return 0;

  fast_mutex* ctx_mutex = &ctx_g->ctx_data.sdif_fast_mutex;
  return ctx_mutex;
}

int fast_mutex_lock_hook(fast_mutex* mutex)
{
  if(mutex == get_gc_mutex())
  {
    int res = ksceKernelLockMutex(SceSdif1_lock, 1, 0);
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to LOCK %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    return 0;

    /*
    FILE_GLOBAL_WRITE_LEN("LOCK GC MUTEX (enter)\n");

    //int res = TAI_CONTINUE(int, fast_mutex_lock_hook_ref, mutex);
    int res = ksceKernelLockMutex(SceSdif1_lock, 1, 0);

    snprintf(sprintfBuffer, 256, "LOCK GC MUTEX (exit) %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    return res;
    */
  }
  else
  {
    int res = TAI_CONTINUE(int, fast_mutex_lock_hook_ref, mutex);
    return res;
  }
}

int fast_mutex_unlock_hook(fast_mutex* mutex)
{
  if(mutex == get_gc_mutex())
  {
    int res = ksceKernelUnlockMutex(SceSdif1_lock, 1);
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "failed to UNLOCK %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    return 0;

    /*
    FILE_GLOBAL_WRITE_LEN("UNLOCK GC MUTEX (enter)\n");

    //int res = TAI_CONTINUE(int, fast_mutex_unlock_hook_ref, mutex);
    int res = ksceKernelUnlockMutex(SceSdif1_lock, 1);

    snprintf(sprintfBuffer, 256, "UNLOCK GC MUTEX (exit) %x\n", res);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    return res;
    */
  }
  else
  {
    int res = TAI_CONTINUE(int, fast_mutex_unlock_hook_ref, mutex);
    return res;
  }
}

int init_global_threading()
{
  SceSdif1_lock = ksceKernelCreateMutex("SceSdif1_Emu", SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 0, 0);
  #ifdef ENABLE_DEBUG_LOG
  if(SceSdif1_lock >= 0)
    FILE_GLOBAL_WRITE_LEN("Created SceSdif1_Emu lock\n");
  #endif

  return 0;
}

int deinit_global_threading()
{
  if(SceSdif1_lock >= 0)
  {    
    int res = ksceKernelDeleteMutex(SceSdif1_lock);
    
    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
    {
      snprintf(sprintfBuffer, 256, "Failed to delete SceSdif1_lock %x\n", res);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    }
    else
    {
      FILE_GLOBAL_WRITE_LEN("Delete SceSdif1_lock\n");
    }
    #endif

    SceSdif1_lock = -1;
  }

  return 0;
}

int init_global_hooks()
{
  init_global_threading();

  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    fast_mutex_lock_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &fast_mutex_lock_hook_ref, "SceSdif", 0xE2C40624, 0x70627F3A, fast_mutex_lock_hook);

    #ifdef ENABLE_DEBUG_LOG
    if(fast_mutex_lock_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init fast_mutex_lock_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init fast_mutex_lock_hook\n");
    #endif

    fast_mutex_unlock_hook_id = taiHookFunctionImportForKernel(KERNEL_PID, &fast_mutex_unlock_hook_ref, "SceSdif", 0xE2C40624, 0xDB395782, fast_mutex_unlock_hook);

    #ifdef ENABLE_DEBUG_LOG
    if(fast_mutex_unlock_hook_id < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to init fast_mutex_unlock_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Init fast_mutex_unlock_hook\n");
    #endif
  }

  return 0;
}

int deinit_global_hooks()
{
  if(fast_mutex_lock_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(fast_mutex_lock_hook_id, fast_mutex_lock_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit fast_mutex_lock_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit fast_mutex_lock_hook\n");
    #endif

    fast_mutex_lock_hook_id = -1;
  }

  if(fast_mutex_unlock_hook_id >= 0)
  {
    int res = taiHookReleaseForKernel(fast_mutex_unlock_hook_id, fast_mutex_unlock_hook_ref);

    #ifdef ENABLE_DEBUG_LOG
    if(res < 0)
      FILE_GLOBAL_WRITE_LEN("Failed to deinit fast_mutex_unlock_hook\n");
    else
      FILE_GLOBAL_WRITE_LEN("Deinit fast_mutex_unlock_hook\n");
    #endif

    fast_mutex_unlock_hook_id = -1;
  }

  deinit_global_threading();

  return 0;
}

uintptr_t get_sdif_data_offset(size_t ofst)
{
  tai_module_info_t sdif_info;
  sdif_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdif", &sdif_info) >= 0)
  {
    uintptr_t addr = 0;
    int ofstRes = module_get_offset(KERNEL_PID, sdif_info.modid, 1, ofst, &addr);
    if(ofstRes == 0)
    {
      return addr;
    }
  }

  return -1;
}

//this is a cleanup function that in theory can be used to restore Sdif data section to a default state
//(without recreating uid objects)
//not sure if this function can be usefull. it looks like transitions from sd to mmc mode go smoothly
int cleanup_sdif()
{
  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("cleanup_sdif\n");
  #endif

  sd_context_global* gc = (sd_context_global*)get_sdif_data_offset(0x2500);
  if(gc > 0)
  {
    cmd_input* commands = gc->commands;

    //clean command cyclic buffer

    memset(commands, 0, sizeof(cmd_input) * COMMAND_CYCLIC_BUFFER_SIZE);

    for(int i = 0; i < COMMAND_CYCLIC_BUFFER_SIZE; i++)
    {
      commands->next_cmd = commands + 1;
      commands->array_index = i;

      int res1 = ksceKernelGetPaddr(commands->vaddr_80, (uintptr_t*)&commands->paddr_184);
      int res2 = ksceKernelGetPaddr(commands->vaddr_1C0, (uintptr_t*)&commands->paddr_1A8);
      int res3 = ksceKernelGetPaddr(commands->vaddr_200, (uintptr_t*)&commands->paddr_1AC);

      #ifdef ENABLE_DEBUG_LOG
      if(res1 < 0)
      {
        FILE_GLOBAL_WRITE_LEN("failed to init paddr_184\n");
      }
      #endif

      #ifdef ENABLE_DEBUG_LOG
      if(res2 < 0)
      {
        FILE_GLOBAL_WRITE_LEN("failed to init paddr_1A8\n");
      }
      #endif

      #ifdef ENABLE_DEBUG_LOG
      if(res3 < 0)
      {
        FILE_GLOBAL_WRITE_LEN("failed to init paddr_1AC\n");
      }
      #endif

      commands++;
    }

    //last pointer should be 0
    gc->commands[COMMAND_CYCLIC_BUFFER_SIZE - 1].next_cmd = 0;

    //clean context

    sd_context_data* gc_ctxd = &gc->ctx_data;

    //make temp copy
    sd_context_data gc_ctxd_copy;
    memcpy(&gc_ctxd_copy, gc_ctxd, sizeof(sd_context_data));

    //clean ctx
    memset(gc_ctxd, 0, sizeof(sd_context_data));

    //init ctx
    gc_ctxd->cmd_ptr = gc->commands;
    gc_ctxd->cmd_ptr_next = gc->commands + COMMAND_CYCLIC_BUFFER_SIZE - 1;
    gc_ctxd->unk_18 = 0x00300000; // ????
    gc_ctxd->array_idx = 1;
    gc_ctxd->membase_1000 = gc_ctxd_copy.membase_1000;
    gc_ctxd->unk_34 = 0x00000003; // ????
    gc_ctxd->unk_38 = 0x0000000E; // ????
    gc_ctxd->uid_1000 = gc_ctxd_copy.uid_1000;
    gc_ctxd->evid = gc_ctxd_copy.evid;
    
    memcpy(&gc_ctxd->sdif_fast_mutex, &gc_ctxd_copy.sdif_fast_mutex, sizeof(fast_mutex));
    
    //doing this instead of memcpy may cause crashes ?
    //maybe this is because fast_mutex is already broken so it is not deleted properly?
    //sceKernelDeleteFastMutexForDriver(&gc_ctxd_copy.sdif_fast_mutex);
    //sceKernelInitializeFastMutexForDriver(&gc_ctxd->sdif_fast_mutex, "SceSdif1");

    gc_ctxd->uid_10000 = gc_ctxd_copy.uid_10000;
    gc_ctxd->membase_10000 = gc_ctxd_copy.membase_10000;
  }

  //clean mmc context

  sd_context_part_mmc* cp_mmc = (sd_context_part_mmc*)get_sdif_data_offset(0x7218); //can be completely cleared
  if(cp_mmc > 0)
  {
    memset(cp_mmc, 0, sizeof(sd_context_part_mmc)); //clears 0x398 bytes
  }

  //clean sd context

  sd_context_part_sd* cp_sd = (sd_context_part_sd*)get_sdif_data_offset(0x7670);
  if(cp_sd > 0)
  {
    memset(cp_sd, 0, sizeof(sd_context_part_sd)); //clears 0xC0 bytes
  }

  return 0;
}