#pragma once 

#include "sector_api.h"

int emulate_sd_command(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num);