/* psvgamesd_api.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "psvgamesd_api.h" 

#include <psp2kern/kernel/sysmem.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "reader.h"

#include "defines.h"

#include "global_log.h"

#include "physical_sd.h"
#include "physical_mmc.h"
#include "virtual_mmc.h" 
#include "virtual_sd.h"
#include "ins_rem_card.h"
#include "dumper.h"
#include "sector_api.h"

int set_iso_path(const char* path)
{
  char path_kernel[256];
  memset(path_kernel, 0, 256);
  ksceKernelStrncpyUserToKernel(path_kernel, (uintptr_t)path, 256);

  #ifdef ENABLE_DEBUG_LOG
  snprintf(sprintfBuffer, 256, "set_iso_path %s\n", path_kernel);
  FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  #endif

  set_reader_iso_path(path_kernel);

  return 0;
}

int clear_iso_path()
{
  clear_reader_iso_path();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("clear_iso_path\n");
  #endif
  return 0;
}

int insert_card()
{
  insert_game_card_emu();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("insert_card\n");
  #endif
  return 0;
}

int remove_card()
{
  remove_game_card_emu();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("remove_card\n");
  #endif
  return 0;
}

int initialize_physical_mmc()
{
  initialize_hooks_physical_mmc();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("initialize_physical_mmc\n");
  #endif
  return 0;
}

int deinitialize_physical_mmc()
{
  deinitialize_hooks_physical_mmc();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("deinitialize_physical_mmc\n");
  #endif
  return 0;
}

int initialize_virtual_mmc()
{
  initialize_hooks_virtual_mmc();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("initialize_virtual_mmc\n");
  #endif
  return 0;
}

int deinitialize_virtual_mmc()
{
  deinitialize_hooks_virtual_mmc();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("deinitialize_virtual_mmc\n");
  #endif
  return 0;
}

int initialize_physical_sd()
{
  initialize_hooks_physical_sd();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("initialize_physical_sd\n");
  #endif
  return 0;
}

int deinitialize_physical_sd()
{
  deinitialize_hooks_physical_sd();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("deinitialize_physical_sd\n");
  #endif
  return 0;
}

int initialize_virtual_sd()
{
  initialize_hooks_virtual_sd();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("initialize_virtual_sd\n");
  #endif
  return 0;
}

int deinitialize_virtual_sd()
{
  deinitialize_hooks_virtual_sd();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("deinitialize_virtual_sd\n");
  #endif
  return 0;
}

int dump_mmc_card_start(const char* path)
{
  char path_kernel[256];
  memset(path_kernel, 0, 256);
  ksceKernelStrncpyUserToKernel(path_kernel, (uintptr_t)path, 256);

  #ifdef ENABLE_DEBUG_LOG
  snprintf(sprintfBuffer, 256, "dump_mmc_card_start %s\n", path_kernel);
  FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  #endif

  dump_mmc_card_start_internal(path_kernel);

  return 0;
}

int dump_mmc_card_cancel()
{
  dump_mmc_card_stop_internal();

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("dump_mmc_card_cancel\n");
  #endif
  return 0;
}

uint32_t dump_mmc_get_total_sectors()
{
  uint32_t value = get_total_sectors();

  #ifdef ENABLE_DEBUG_LOG
  snprintf(sprintfBuffer, 256, "dump_mmc_get_total_sectors %x\n", value);
  FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  #endif

  return value;
}

uint32_t dump_mmc_get_progress_sectors()
{
  uint32_t value = get_progress_sectors();

  #ifdef ENABLE_DEBUG_LOG
  snprintf(sprintfBuffer, 256, "dump_mmc_get_progress_sectors %x\n", value);
  FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  #endif

  return value;
}

int get_phys_ins_state()
{
  int res = ksceSdifGetCardInsertState1(SCE_SDIF_DEV_GAME_CARD);

  #ifdef ENABLE_DEBUG_LOG
  snprintf(sprintfBuffer, 256, "get_phys_ins_state %x\n", res);
  FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  #endif

  return res;
}

static int g_is_driver_state_saved = 0;
static psvgamesd_ctx g_driver_state;

int save_psvgamesd_state(const psvgamesd_ctx* state)
{
  ksceKernelMemcpyUserToKernel(&g_driver_state, (uintptr_t)state, sizeof(psvgamesd_ctx));
  
  g_is_driver_state_saved = 1;

  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("save_psvgamesd_state\n");
  #endif
  return 0;
}

int load_psvgamesd_state(psvgamesd_ctx* state)
{
  if(g_is_driver_state_saved == 0)
    return -1;

  ksceKernelMemcpyKernelToUser((uintptr_t)state, &g_driver_state, sizeof(psvgamesd_ctx));
    
  #ifdef ENABLE_DEBUG_LOG
  FILE_GLOBAL_WRITE_LEN("load_psvgamesd_state\n");
  #endif
  return 0;
}
