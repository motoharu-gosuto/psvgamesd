#include "media_id_emu.h"

#include <taihen.h>
#include <module.h>

#include <string.h>

#include "reader.h"
#include "global_log.h"
#include "sector_api.h"
#include "utils.h"
#include "defines.h"

enum PartitionCodes block_dev_to_partition_code(const char* block_dev_name, int length)
{
  if(memcmp(block_dev_name, GC_MEDIA_ID_BLOCK_DEV, length) == 0)
  {
    return cardsExt;
  }
  else if(memcmp(block_dev_name, MC_MEDIA_ID_BLOCK_DEV, length) == 0)
  {
    return cardsExt;
  }
  else
  {
    return empty_c;
  }
}

const PartitionEntry* call_find_partition_entry(const MBR* mbr, const char* block_dev_name, int length)
{
  enum PartitionCodes pc = block_dev_to_partition_code(block_dev_name, length);

  for(int i = 0; i < MAX_MBR_PARTITIONS; i++)
  {
    const PartitionEntry* pe = &mbr->partitions[i];

    if(pe->partitionCode == pc)
      return pe;
  }

  return 0;
}

//this function is somehow not stable
/*
const PartitionEntry* call_find_partition_entry(const char* block_dev_name, int length)
{
  typedef const PartitionEntry* (find_partition_entry_t)(const char* block_dev_name, int length);

  tai_module_info_t sdstor_info;
  sdstor_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) >= 0) 
  {
    find_partition_entry_t* find_partition_entry = 0;
    int ofstRes = module_get_offset(KERNEL_PID, sdstor_info.modid, 0, 0x142D, (uintptr_t*)&find_partition_entry);
    if(ofstRes == 0)
    {
      return find_partition_entry(block_dev_name, length);
    }
    else
    {
      #ifdef ENABLE_DEBUG_LOG
      snprintf(sprintfBuffer, 256, "failed to get address of find_partition_entry %x\n", ofstRes);
      FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
      #endif
    }
  }

  return 0;
}
*/

char media_id[SD_DEFAULT_SECTOR_SIZE] = {0};

int media_id_sector_cached = -1;

//if result is -1 - error
//if result is 0 - not a media-id partition
//if result is 1 - media-id was written
int write_media_id(const MBR* mbr, int sector, char* buffer, int nSectors)
{
  if(media_id_sector_cached > 0 && sector != media_id_sector_cached)
    return 0;

  const PartitionEntry* pe = call_find_partition_entry(mbr, GC_MEDIA_ID_BLOCK_DEV, 0x12);
  if(pe > 0)
  {
    if(pe->partitionOffset == sector)
    {
      if(nSectors == 1)
      {
        #ifdef ENABLE_DEBUG_LOG
        FILE_GLOBAL_WRITE_LEN("write media id\n");
        #endif

        memcpy(media_id, buffer, SD_DEFAULT_SECTOR_SIZE);

        if(media_id_sector_cached < 0)
          media_id_sector_cached = sector;
        return 1;
      }
      else
      {
        #ifdef ENABLE_DEBUG_LOG
        FILE_GLOBAL_WRITE_LEN("too many sectors in media_id\n");
        #endif

        return -1;
      }
    }
    else
    {
      return 0;
    }
  }
  else
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to find partition entry on write %x\n", pe);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif

    return -1;
  }
}

//if result is -1 - error
//if result is 0 - not a media-id partition
//if result is 1 - media-id was written
int read_media_id(const MBR* mbr, int sector, char* buffer, int nSectors)
{
  if(media_id_sector_cached > 0 && sector != media_id_sector_cached)
    return 0;

  const PartitionEntry* pe = call_find_partition_entry(mbr, GC_MEDIA_ID_BLOCK_DEV, 0x12);
  if(pe > 0)
  {
    if(pe->partitionOffset == sector)
    {
      if(nSectors == 1)
      {
        #ifdef ENABLE_DEBUG_LOG
        FILE_GLOBAL_WRITE_LEN("read media id\n");
        #endif

        memcpy(buffer, media_id, SD_DEFAULT_SECTOR_SIZE);

        if(media_id_sector_cached < 0)
          media_id_sector_cached = sector;
        return 1;
      }
      else
      {
        #ifdef ENABLE_DEBUG_LOG
        FILE_GLOBAL_WRITE_LEN("too many sectors in media_id\n");
        #endif

        return -1;
      }
    }
    else
    {
      return 0;
    }
  }
  else
  {
    #ifdef ENABLE_DEBUG_LOG
    snprintf(sprintfBuffer, 256, "failed to find partition entry on read %x\n", pe);
    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
    #endif

    return -1;
  }
}

int init_media_id_emu()
{
  memset(media_id, 0, SD_DEFAULT_SECTOR_SIZE);
  media_id_sector_cached = -1;
  return 0;
}

int deinit_media_id_emu()
{
  memset(media_id, 0, SD_DEFAULT_SECTOR_SIZE);
  media_id_sector_cached = -1;
  return 0;
}

