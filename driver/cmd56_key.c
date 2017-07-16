#include "cmd56_key.h"

#include <psp2kern/types.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <taihen.h>
#include <module.h>

#define CMD56_KEY_FILE_PATH "ux0:iso/cmd56_key.bin"

//this function sets all sensitive data in GcAuthMgr 
int set_5018_data()
{
  tai_module_info_t gc_info;
  gc_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSblGcAuthMgr", &gc_info) >= 0)
  {
    uintptr_t addr = 0;
    int ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x5018, &addr);
    if(ofstRes == 0)
    {
      char data_5018_buffer[0x34] = {0};

      SceUID fd = ksceIoOpen(CMD56_KEY_FILE_PATH, SCE_O_RDONLY, 0777);
      if(fd >= 0)
      {
        ksceIoRead(fd, data_5018_buffer, 0x34);
        ksceIoClose(fd);
      }

      memcpy((char*)addr, data_5018_buffer, 0x34);
    }
  }

  return 0;
}

//this function gets all sensitive data that is cleaned up by GcAuthMgr function 0xBB451E83
int get_5018_data()
{
  tai_module_info_t gc_info;
  gc_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSblGcAuthMgr", &gc_info) >= 0)
  {
    uintptr_t addr = 0;
    int ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x5018, &addr);
    if(ofstRes == 0)
    {
      char data_5018_buffer[0x34] = {0};

      memcpy(data_5018_buffer, (char*)addr, 0x34);

      SceUID fd = ksceIoOpen(CMD56_KEY_FILE_PATH, SCE_O_CREAT | SCE_O_APPEND | SCE_O_WRONLY, 0777);

      if(fd >= 0)
      {
        ksceIoWrite(fd, data_5018_buffer, 0x34);
        ksceIoClose(fd);
      }
    }
  }

  return 0;
}