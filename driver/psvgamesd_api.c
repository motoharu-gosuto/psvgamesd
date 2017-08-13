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

int set_iso_path(const char* path)
{
  char path_kernel[256];
  memset(path_kernel, 0, 256);
  ksceKernelStrncpyUserToKernel(path_kernel, (uintptr_t)path, 256);

  //snprintf(sprintfBuffer, 256, "set_iso_path %s\n", path_kernel);
  //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

  set_reader_iso_path(path_kernel);

  return 0;
}

int clear_iso_path()
{
  clear_reader_iso_path();

  return 0;
}

int insert_card()
{
  #ifdef ENABLE_INSERT_EMU
  insert_game_card_emu();
  #else
  insert_game_card();
  #endif

  //FILE_GLOBAL_WRITE_LEN("insert_card\n");
  return 0;
}

int remove_card()
{
  #ifdef ENABLE_INSERT_EMU
  remove_game_card_emu();
  #else
  remove_game_card();
  #endif

  //FILE_GLOBAL_WRITE_LEN("remove_card\n");
  return 0;
}

int initialize_physical_mmc()
{
  initialize_hooks_physical_mmc();

  //FILE_GLOBAL_WRITE_LEN("initialize_physical_mmc\n");
  return 0;
}

int deinitialize_physical_mmc()
{
  deinitialize_hooks_physical_mmc();

  //FILE_GLOBAL_WRITE_LEN("deinitialize_physical_mmc\n");
  return 0;
}

int initialize_virtual_mmc()
{
  initialize_hooks_virtual_mmc();

  //FILE_GLOBAL_WRITE_LEN("initialize_virtual_mmc\n");
  return 0;
}

int deinitialize_virtual_mmc()
{
  deinitialize_hooks_virtual_mmc();

  //FILE_GLOBAL_WRITE_LEN("deinitialize_virtual_mmc\n");
  return 0;
}

int initialize_physical_sd()
{
  initialize_hooks_physical_sd();

  //FILE_GLOBAL_WRITE_LEN("initialize_physical_sd\n");
  return 0;
}

int deinitialize_physical_sd()
{
  deinitialize_hooks_physical_sd();

  //FILE_GLOBAL_WRITE_LEN("deinitialize_physical_sd\n");
  return 0;
}

int initialize_virtual_sd()
{
  initialize_hooks_virtual_sd();

  //FILE_GLOBAL_WRITE_LEN("initialize_virtual_sd\n");
  return 0;
}

int deinitialize_virtual_sd()
{
  deinitialize_hooks_virtual_sd();

  //FILE_GLOBAL_WRITE_LEN("deinitialize_virtual_sd\n");
  return 0;
}

int dump_mmc_card_start(const char* path)
{
  char path_kernel[256];
  memset(path_kernel, 0, 256);
  ksceKernelStrncpyUserToKernel(path_kernel, (uintptr_t)path, 256);

  //snprintf(sprintfBuffer, 256, "dump_mmc_card_start %s\n", path_kernel);
  //FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

  initialize_dump_threading(path_kernel);

  return 0;
}

int dump_mmc_card_cancel()
{
  deinitialize_dump_threading();

  return 0;
}