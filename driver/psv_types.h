// https://gist.github.com/yifanlu/d546e687f751f951b1109ffc8dd8d903

#pragma once

#pragma pack(push, 1)

typedef struct psv_file_header_base
{
  uint32_t magic;
  uint32_t version;
}psv_file_header_base;

typedef struct psv_file_header_v1
{
  uint32_t magic;
  uint32_t version;
  uint8_t key1[0x10]; // key used to decrypt klicensee
  uint8_t key2[0x10]; // key used to decrypt klicensee
  uint8_t signature[0x14]; // signature that should match signature in rif
  uint64_t image_size;
} psv_file_header_v1;
  
#pragma pack(pop)

#define PSV_MAGIC (0x00565350)

#define PSV_VERSION_V1 1