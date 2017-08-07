#include "reg_common.h"

int memcpy_inv(char* dest, const char* src, size_t size)
{
    for(int i = 0, j = size - 1; i < size ; i++, j--)
       dest[i] = src[j];
    return 0;
} 
