#include "utils.h"
#include "global_log.h"

#include <psp2kern/types.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

void CMD_BIN_LOG(char* data, int size)
{
  SceUID global_log_fd = ksceIoOpen("ux0:dump/cmd_log.bin", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

  if(global_log_fd >= 0)
  {
    ksceIoWrite(global_log_fd, data, size);
    ksceIoClose(global_log_fd);
  }  
}

int print_bytes(const char* data, int len)
{
  for(int i = 0; i < len; i++)
  {
    snprintf(sprintfBuffer, 256, "%02x", data[i]);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
  }

  FILE_GLOBAL_WRITE_LEN("\n");

  return 0;
} 

int print_cmd(cmd_input* cmd_data, int n,  char* when)
{
    snprintf(sprintfBuffer, 256, "--- CMD%d (%d) %s ---\n", cmd_data->command, n, when);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "cmd1: %x\n", cmd_data);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "argument: %x\n", cmd_data->argument);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "buffer: %x\n", cmd_data->buffer);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "state: %x\n", cmd_data->state_flags);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "error_code: %x\n", cmd_data->error_code);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    
    snprintf(sprintfBuffer, 256, "unk_64: %x\n", cmd_data->unk_64);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "resp_block_size_24: %x\n", cmd_data->resp_block_size_24);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "resp_n_blocks_26: %x\n", cmd_data->resp_n_blocks_26);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "base_198: %x\n", cmd_data->base_198);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "offset_19C: %x\n", cmd_data->offset_19C);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "size_1A0: %x\n", cmd_data->size_1A0);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    snprintf(sprintfBuffer, 256, "size_1A4: %x\n", cmd_data->size_1A4);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);

    if((((int)cmd_data->state_flags) << 0x15) < 0)
    {
      FILE_GLOBAL_WRITE_LEN("INVALIDATE\n");
      //print_bytes(cmd_data->base_198, 0x200);

      //print_bytes(cmd_data->vaddr_1C0, 0x10);
      //print_bytes(cmd_data->vaddr_200, 0x10);

      //print_bytes(cmd_data->base_198 + cmd_data->offset_19C, cmd_data->size_1A4);
    }

    if(((0x801 << 9) & cmd_data->state_flags) != 0)
      FILE_GLOBAL_WRITE_LEN("SKIP INVALIDATE\n");

    if((((int)cmd_data->state_flags) << 0xB) < 0)
     FILE_GLOBAL_WRITE_LEN("FREE mem_188\n");

    //print_bytes(cmd_data->response, 0x10);

    /*
    if(cmd_data->buffer > 0)
       print_bytes(cmd_data->buffer, cmd_data->b_size);
    */

    //print_bytes(cmd_data->vaddr_80, 0x10);

    return 0;
}