#pragma once

#include <stdio.h>

#define INV_16(value) ((((value) & 0x00FF) << 8) | (((value) & 0xFF00) >> 8))
#define INV_32(value) ((((value) & 0x000000FF) << 24) | (((value) & 0x0000FF00) << 8) | (((value) & 0x00FF0000) >> 8) | (((value) & 0xFF000000) >> 24))
#define INV_64(value) ((((value) & 0x00000000000000FF) << 56) | (((value) & 0x000000000000FF00) << 40) | (((value) & 0x0000000000FF0000) << 24) | (((value) & 0x00000000FF000000) << 8) | (((value) & 0x000000FF00000000) >> 8) | (((value) & 0x0000FF0000000000) >> 24) | (((value) & 0x00FF000000000000) >> 40) | (((value) & 0xFF00000000000000) >> 56))

int memcpy_inv(char* dest, const char* src, size_t size);