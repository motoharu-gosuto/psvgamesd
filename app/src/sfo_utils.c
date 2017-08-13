#include "sfo_utils.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <psp2/io/fcntl.h>

#pragma pack(push, 1)

//http://www.psdevwiki.com/ps3/PARAM.SFO
//SFO stands for PSP Game Parameters File

typedef struct sfo_header
{
   uint32_t magic;            // Always PSF
   uint32_t version;          // Usually 1.1
   uint32_t key_table_start;  // Start offset of key_table
   uint32_t data_table_start; // Start offset of data_table
   uint32_t tables_entries;   // Number of entries in all tables
}sfo_header;

//table entry data formats
#define SFO_TE_DF_UTF8S 0x0004
#define SFO_TE_DF_UTF8  0x0204
#define SFO_TE_DF_INT32 0x0404

typedef struct sfo_index_table_entry
{
   uint16_t key_offset;   // param_key offset (relative to start offset of key_table)
   uint16_t data_fmt; // param_data data type
   uint32_t data_len;     // param_data used bytes
   uint32_t data_max_len; // param_data total bytes
   uint32_t data_offset;  // param_data offset (relative to start offset of data_table)
}sfo_index_table_entry;

#pragma pack(pop)

#define PSF_MAGIC 0x46535000

sfo_header g_header;

sfo_index_table_entry g_table_entries[SFO_TABLE_ENTRIES_N];

char g_sfo_cached_path[256];

int init_sfo_structures(const char* path)
{
  memset(g_sfo_cached_path, 0, 256);
  strncpy(g_sfo_cached_path, path, 256);

  SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
  if(fd < 0)
    return -1;

  memset(&g_header, 0, sizeof(sfo_header));
  int read_res = sceIoRead(fd, &g_header, sizeof(sfo_header));
  if(read_res < 0)
    return -1;

  if (g_header.magic != PSF_MAGIC)
    return -1;

  memset(g_table_entries, 0, sizeof(sfo_index_table_entry) * SFO_TABLE_ENTRIES_N);
  for (uint32_t i = 0; (i < g_header.tables_entries && i < SFO_TABLE_ENTRIES_N); i++)
  {
    sfo_index_table_entry* ce = g_table_entries + i;
    read_res = sceIoRead(fd, ce, sizeof(sfo_index_table_entry));
    if(read_res < 0)
      return -1;
  }

  sceIoClose(fd);

  return 0;
}

int is_sfo_structures_initialized(const char* path)
{
  return strncmp(g_sfo_cached_path, path, 256) == 0;
}

int read_null_term_utf8_string(SceUID fd, char* key_buffer, uint32_t max_len)
{
  memset(key_buffer, 0, max_len);

  for(int i = 0; i < max_len; i++)
  {
    char letter = 0;

    int read_res = sceIoRead(fd, &letter, sizeof(char));
    if(read_res < 0)
    return -1;

    key_buffer[i] = letter;

    if(letter == 0)
        break;
  }

  if(key_buffer[max_len - 1] != 0)
  {
    key_buffer[max_len - 1] = 0;
    return -1; //not enough buffer length
  }

  return 0;
}

char g_key_buffer[SFO_MAX_KEY_LEN];

int get_utf8_value(const char* path, const char* key, char* value, uint32_t max_value_len)
{
  if(is_sfo_structures_initialized(path) == 0)
    return -1;

  memset(value, 0, max_value_len);

  SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
  if(fd < 0)
    return -1;

  for (uint32_t i = 0; (i < g_header.tables_entries && i < SFO_TABLE_ENTRIES_N); i++)
  {
    sfo_index_table_entry* te = g_table_entries + i;

    if(te->data_fmt != SFO_TE_DF_UTF8)
        continue;

    SceOff seek_res = sceIoLseek(fd, g_header.key_table_start + te->key_offset, SCE_SEEK_SET);
    if(seek_res < 0)
      return -1;

    int read_key_res = read_null_term_utf8_string(fd, g_key_buffer, SFO_MAX_KEY_LEN);
    if(read_key_res < 0)
      return -1;

    if(strncmp(g_key_buffer, key, SFO_MAX_KEY_LEN) == 0)
    {
      if(te->data_max_len > max_value_len)
        return -1; //not enough buffer length

      seek_res = sceIoLseek(fd, g_header.data_table_start + te->data_offset, SCE_SEEK_SET);
      if(seek_res < 0)
        return -1;

      int read_res = sceIoRead(fd, value, te->data_max_len);
      if(read_res < 0)
        return -1;
    
      break;
    }
  }

  sceIoClose(fd);

  return 0;
}

int get_int32_value(const char* path, const char* key, int32_t* value)
{
  if(is_sfo_structures_initialized(path) == 0)
    return -1;

  *value = 0;

  SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
  if(fd < 0)
    return -1;

  for (uint32_t i = 0; (i < g_header.tables_entries && i < SFO_TABLE_ENTRIES_N); i++)
  {
    sfo_index_table_entry* te = g_table_entries + i;

    if(te->data_fmt != SFO_TE_DF_INT32)
      continue;

    SceOff seek_res = sceIoLseek(fd, g_header.key_table_start + te->key_offset, SCE_SEEK_SET);
    if(seek_res < 0)
      return -1;

    int read_key_res = read_null_term_utf8_string(fd, g_key_buffer, SFO_MAX_KEY_LEN);
    if(read_key_res < 0)
      return -1;

    if(strncmp(g_key_buffer, key, SFO_MAX_KEY_LEN) == 0)
    {
      if(te->data_max_len != sizeof(int))
        return -1; //not enough buffer length

      seek_res = sceIoLseek(fd, g_header.data_table_start + te->data_offset, SCE_SEEK_SET);
      if(seek_res < 0)
        return -1;

      int read_res = sceIoRead(fd, value, te->data_max_len);
      if(read_res < 0)
        return -1;
    
      break;
    }
  }

  sceIoClose(fd);

  return 0;
}