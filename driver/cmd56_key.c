#include "cmd56_key.h"

#include <psp2kern/types.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <taihen.h>
#include <module.h>

#include "reader.h"

//this function sets all sensitive data in GcAuthMgr 
int set_5018_data(const char* data_5018_buffer)
{
  tai_module_info_t gc_info;
  gc_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSblGcAuthMgr", &gc_info) >= 0)
  {
    uintptr_t addr = 0;
    int ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x5018, &addr);
    if(ofstRes == 0)
    {
      memcpy((char*)addr, data_5018_buffer, CMD56_DATA_SIZE);
    }
  }

  return 0;
}

//this function gets all sensitive data that is cleaned up by GcAuthMgr function 0xBB451E83
int get_5018_data(char* data_5018_buffer)
{
  tai_module_info_t gc_info;
  gc_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSblGcAuthMgr", &gc_info) >= 0)
  {
    uintptr_t addr = 0;
    int ofstRes = module_get_offset(KERNEL_PID, gc_info.modid, 1, 0x5018, &addr);
    if(ofstRes == 0)
    {
      memcpy(data_5018_buffer, (char*)addr, CMD56_DATA_SIZE);
    }
  }

  return 0;
}