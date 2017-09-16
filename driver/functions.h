#pragma once

#include <psp2kern/types.h>
#include <psp2kern/kernel/threadmgr.h>

#include "sector_api.h"

#pragma pack(push, 1)

typedef struct SceKernelCondOptParam 
{
	SceSize size;
} SceKernelCondOptParam;

#pragma pack(pop)

typedef SceUID (sceKernelCreateCondForDriver_t)(const char* name, SceUInt attr, SceUID mutexId, const SceKernelCondOptParam* option);
typedef int (sceKernelDeleteCondForDriver_t)(SceUID cid);
typedef int (sceKernelWaitCondForDriver_t)(SceUID condId, unsigned int *timeout);
typedef int (sceKernelSignalCondForDriver_t)(SceUID condId);
typedef int (sceKernelSha1DigestForDriver_t)(char* in, int size, char* digest);

typedef int (sceKernelInitializeFastMutexForDriver_t)(fast_mutex* mutex, const char *name);
typedef int (sceKernelDeleteFastMutexForDriver_t)(fast_mutex* mutex);
typedef int (sceKernelGetMutexInfoForDriver_t)(SceUID mutexid, SceKernelMutexInfo *info);

#pragma pack(push, 1)

typedef struct sha256_ctx
{
  char data[0x94];
}sha256_ctx;

#pragma pack(pop)

typedef int (sceSha256BlockInitForDriver_t)(sha256_ctx* ctx);
typedef int (sceSha256BlockUpdateForDriver_t)(sha256_ctx* ctx, char* data, int size);
typedef int (sceSha256BlockResultForDriver_t)(sha256_ctx* ctx, char* digest);

extern sceKernelCreateCondForDriver_t* sceKernelCreateCondForDriver;
extern sceKernelDeleteCondForDriver_t* sceKernelDeleteCondForDriver;
extern sceKernelWaitCondForDriver_t* sceKernelWaitCondForDriver;
extern sceKernelSignalCondForDriver_t* sceKernelSignalCondForDriver;
extern sceKernelSha1DigestForDriver_t* sceKernelSha1DigestForDriver;

extern sceKernelInitializeFastMutexForDriver_t* sceKernelInitializeFastMutexForDriver;
extern sceKernelDeleteFastMutexForDriver_t* sceKernelDeleteFastMutexForDriver;
extern sceKernelGetMutexInfoForDriver_t* sceKernelGetMutexInfoForDriver;

extern sceSha256BlockInitForDriver_t* sceSha256BlockInitForDriver;
extern sceSha256BlockUpdateForDriver_t* sceSha256BlockUpdateForDriver;
extern sceSha256BlockResultForDriver_t* sceSha256BlockResultForDriver;

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