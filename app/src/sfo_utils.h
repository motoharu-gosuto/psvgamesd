#pragma once

#include <stdint.h>

#define SFO_CONTENT_ID_KEY "CONTENT_ID"
#define SFO_TITLE_ID_KEY "TITLE_ID"
#define SFO_GC_RO_SIZE_KEY "GC_RO_SIZE"

#define SFO_TABLE_ENTRIES_N 256
#define SFO_MAX_KEY_LEN 512
#define SFO_MAX_STR_VALUE_LEN 512

int init_sfo_structures(const char* path);

int get_utf8_value(const char* path, const char* key, char* value, uint32_t max_value_len);

int get_int32_value(const char* path, const char* key, int32_t* value);

int is_sfo_structures_initialized(const char* path);