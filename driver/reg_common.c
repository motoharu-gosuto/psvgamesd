/* reg_common.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "reg_common.h"

int memcpy_inv(char* dest, const char* src, size_t size)
{
    for(int i = 0, j = size - 1; i < size ; i++, j--)
       dest[i] = src[j];
    return 0;
} 
