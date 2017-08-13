#pragma once

 #include <psp2kern/types.h>

extern SceUID req_lock;
extern SceUID resp_lock;

extern SceUID req_cond;
extern SceUID resp_cond;

extern void* g_ctx_part;
extern int g_sector;
extern char* g_buffer;
extern int g_nSectors; 
extern int g_res;

int set_reader_iso_path(const char* path);
int clear_reader_iso_path();

int get_cmd56_data(char* buffer);

int initialize_read_threading();
int deinitialize_read_threading();