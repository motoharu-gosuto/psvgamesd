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