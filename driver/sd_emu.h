#pragma once 

#include "sector_api.h"

#define READ_SINGLE_BLOCK 17
#define READ_MULTIPLE_BLOCK 18

#define WRITE_BLOCK 24
#define WRITE_MULTIPLE_BLOCK 25

int emulate_sd_command(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num);