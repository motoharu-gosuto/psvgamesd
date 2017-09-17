/* global_log.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "global_log.h"

#include <psp2kern/types.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>

char sprintfBuffer[256];

void FILE_GLOBAL_WRITE_LEN(char* msg)
{
  SceUID global_log_fd = ksceIoOpen("ux0:dump/game_log.txt", SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

  if(global_log_fd >= 0)
  {
    ksceIoWrite(global_log_fd, msg, strlen(msg));
    ksceIoClose(global_log_fd);
  }  
} 
