#pragma once

int set_iso_path(const char* path);

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