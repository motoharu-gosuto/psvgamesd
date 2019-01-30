#pragma once

#include <psp2kern/types.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>

#include <taihen.h>

#include "sector_api.h"

typedef int (sceKernelGetModuleInfoForKernel_t)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);
extern sceKernelGetModuleInfoForKernel_t* sceKernelGetModuleInfoForKernel;

int initialize_functions();

#define SCE_KERNEL_SYS_EVENT_SUSPEND 0
#define SCE_KERNEL_SYS_EVENT_RESUME 1

#pragma pack(push, 1)

typedef struct
{
  uint32_t size; // 24
  uint32_t unk1;
  uint32_t unk2;
  uint32_t unk3;
  uint32_t unk4;
  uint32_t unk5;
} sysevent_args_t;

typedef struct
{
	uint32_t unk_0;
	uint8_t unk_4;
	uint8_t unk_5;
	uint8_t unk_6;
	uint8_t unk_7;
}sysevent_opt_t;

#pragma pack(pop)

typedef int (sysevent_callback_t)(int resume, int eventid, sysevent_args_t* args, sysevent_opt_t* opt);

SceUID ksceKernelRegisterSysEventHandler(const char *name, sysevent_callback_t *callback_func, void *opt);

int ksceKernelUnregisterSysEventHandler(SceUID evid);

int module_get_by_name_nid(SceUID pid, const char *name, uint32_t nid, tai_module_info_t *info);
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int module_get_import_func(SceUID pid, const char *modname, uint32_t target_libnid, uint32_t funcnid, uintptr_t *stub);
