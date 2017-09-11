#pragma once

#include "sector_api.h"

void CMD_BIN_LOG(char* data, int size);

int print_bytes(const char* data, int len);

int print_cmd(cmd_input* cmd_data, int n,  char* when);

int print_SceSdif1_lock_info(SceUID mutex);

int dump_sdif_data();