#include "global_log.h"

void FILE_GLOBAL_WRITE_LEN(char* msg)
{
  SceUID global_log_fd = ksceIoOpen("ux0:dump/game_log.txt", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

  if(global_log_fd >= 0)
  {
    ksceIoWrite(global_log_fd, msg, strlen(msg));
    ksceIoClose(global_log_fd);
  }  
} 
