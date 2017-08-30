#pragma once

#include <stdint.h>

int set_iso_path(const char* path);

int clear_iso_path();

int insert_card();

int remove_card();

int initialize_physical_mmc();

int deinitialize_physical_mmc();

int initialize_virtual_mmc();

int deinitialize_virtual_mmc();

int initialize_physical_sd();

int deinitialize_physical_sd();

int initialize_virtual_sd();

int deinitialize_virtual_sd();

int dump_mmc_card_start(const char* path);

int dump_mmc_card_cancel();

uint32_t dump_mmc_get_total_sectors();

uint32_t dump_mmc_get_progress_sectors();

int get_phys_ins_state();

#pragma pack(push, 1)

typedef struct psvgamesd_ctx
{
  char current_directory[256];
  char selected_iso[256];
  uint32_t driver_mode;
  uint32_t insertion_state;
}psvgamesd_ctx;

#pragma pack(pop)

int save_psvgamesd_state(const psvgamesd_ctx* state);

int load_psvgamesd_state(psvgamesd_ctx* state);