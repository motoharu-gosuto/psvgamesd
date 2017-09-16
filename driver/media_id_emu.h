#pragma once

#include "mbr_types.h"

int write_media_id(const MBR* mbr, int sector, char* buffer, int nSectors);

int read_media_id(const MBR* mbr, int sector, char* buffer, int nSectors);