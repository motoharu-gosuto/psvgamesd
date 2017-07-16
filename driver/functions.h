#pragma once

#include <psp2kern/types.h>

typedef struct SceKernelCondOptParam 
{
	SceSize size;
} SceKernelCondOptParam;

typedef SceUID (sceKernelCreateCondForDriver_t)(const char* name, SceUInt attr, SceUID mutexId, const SceKernelCondOptParam* option);
typedef int (sceKernelDeleteCondForDriver_t)(SceUID cid);
typedef int (sceKernelWaitCondForDriver_t)(SceUID condId, unsigned int *timeout);
typedef int (sceKernelSignalCondForDriver_t)(SceUID condId);
typedef int (sceKernelSha1DigestForDriver_t)(char* in, int size, char* digest);

extern sceKernelCreateCondForDriver_t* sceKernelCreateCondForDriver;
extern sceKernelDeleteCondForDriver_t* sceKernelDeleteCondForDriver;
extern sceKernelWaitCondForDriver_t* sceKernelWaitCondForDriver;
extern sceKernelSignalCondForDriver_t* sceKernelSignalCondForDriver;
extern sceKernelSha1DigestForDriver_t* sceKernelSha1DigestForDriver;

int initialize_functions();