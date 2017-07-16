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
