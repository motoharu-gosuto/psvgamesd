/* sd_reg.h
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#pragma once

#include <stdint.h>

#include "reg_common.h"

#pragma pack(push, 1)

//========== CID ==========

typedef struct SD_CID
{
   uint8_t MID; //[127:120]
   uint8_t OID[2]; //[119:104]
   uint8_t PNM[5]; //[103:64]
   uint8_t PRV; //[63:56]
   uint32_t PSN; //[55:24]
   //Reserved [23:20]
   uint16_t MDT; //[19:8]
   uint8_t CRC7; //[7:0]
}SD_CID;

#define SD_CID_OID_SET(oid, value) strncpy((char*)oid, value, 2)

#define SD_CID_PNM_SET(pnm, value) strncpy((char*)pnm, value, 5)

#define SD_CID_PRV_N_GET(prv) (((prv) & 0xF0) >> 4)
#define SD_CID_PRV_M_GET(prv) ((prv) & 0x0F)
#define SD_CID_PRV_NM_SET(n, m) ((((n) & 0x0F) << 4) | ((m) & (0x0F)))

#define SD_CID_PSN_GET(psn) (INV_32(psn))
#define SD_CID_PSN_SET(psn, value) (INV_32(value))

#define SD_CID_MDT_JAN 1
#define SD_CID_MDT_FEB 2
#define SD_CID_MDT_MAR 3
#define SD_CID_MDT_APR 4
#define SD_CID_MDT_MAY 5
#define SD_CID_MDT_JUN 6
#define SD_CID_MDT_JUL 7
#define SD_CID_MDT_AUG 8
#define SD_CID_MDT_SEP 9
#define SD_CID_MDT_OCT 10
#define SD_CID_MDT_NOV 11
#define SD_CID_MDT_DEC 12

#define SD_CID_MDT_MIN_YEAR 2000
#define SD_CID_MDT_MAX_YEAR 2255

#define SD_CID_MDT_M_GET(mdt) ((INV_16(mdt)) & 0x000F)
#define SD_CID_MDT_Y_GET(mdt) ((((INV_16(mdt)) & 0x0FF0) >> 4) + SD_CID_MDT_MIN_YEAR)
#define SD_CID_MDT_Y_M_SET(y, m) (INV_16(((((y) - SD_CID_MDT_MIN_YEAR) & 0x00FF) << 4) | ((m) & 0x000F)))

//========== CSD ==========

//There are different versions of CSD register in SD standard
//layout of the register changes with each new standard of SD

//STRUCTURE

#define SD_CSD_STRUCTURE_MASK 0xC0
#define SD_CSD_STRUCTURE_GET(b0_structure) (((b0_structure) & SD_CSD_STRUCTURE_MASK) >> 6)
#define SD_CSD_STRUCTURE_SET(b0, b0_structure) ((b0) | (((b0_structure) & 3) << 6))

#define SD_CSD_STRUCTURE_VERSION_1_0 0
#define SD_CSD_STRUCTURE_VERSION_2_0 1

//========== CSD V1 ==========

typedef struct SD_CSD_V1
{
   union SD_CSD_V1_b0
   {
      uint8_t CSD_STRUCTURE; // [127:126]
      //Reserved             // [125:120]
   }b0;

   uint8_t TAAC;       // [119:112]
   uint8_t NSAC;       // [111:104]
   uint8_t TRAN_SPEED; // [103:96]

   union SD_CSD_V1_w4
   {
      uint16_t CCC;         // [95:84]
      uint16_t READ_BL_LEN; // [83:80]
   } w4;

   union SD_CSD_V1_qw6
   {
      uint64_t READ_BL_PARTIAL;    // [79:79]
      uint64_t WRITE_BLK_MISALIGN; // [78:78]
      uint64_t READ_BLK_MISALIGN;  // [77:77]
      uint64_t DSR_IMP;            // [76:76]
      // Reserved                     [75:74]
      uint64_t C_SIZE;             // [73:62]
      uint64_t VDD_R_CURR_MIN;     // [61:59]
      uint64_t VDD_R_CURR_MAX;     // [58:56]
      uint64_t VDD_W_CURR_MIN;     // [55:53]
      uint64_t VDD_W_CURR_MAX;     // [52:50]
      uint64_t C_SIZE_MULT;        // [49:47]

      uint64_t ERASE_BLK_EN;       // [46:46]
      uint64_t SECTOR_SIZE;        // [45:39]
      uint64_t WP_GRP_SIZE;        // [38:32]

      uint64_t WP_GRP_ENABLE;      // [31:31]

      //Reserved                      [30:29]

      uint64_t R2W_FACTOR;         // [28:26]
      uint64_t WRITE_BL_LEN;       // [25:22]
      uint64_t WRITE_BL_PARTIAL;   // [21:21]

      //Reserved                      [20:16]
   }qw6;

   union SD_CSD_V1_b14
   {
      uint8_t FILE_FORMAT_GRP;    // [15:15]
      uint8_t COPY;               // [14:14]
      uint8_t PERM_WRITE_PROTECT; // [13:13]
      uint8_t TMP_WRITE_PROTECT;  // [12:12]
      uint8_t FILE_FORMAT;        // [11:10]
      //Reserved                     [9:8]
   }b14;

   uint8_t CRC7; // [7:0]

} SD_CSD_V1;

//TAAC

#define SD_CSD_V1_TAAC_TU_MASK 7
#define SD_CSD_V1_TAAC_TU_GET(taac) ((taac) & SD_CSD_V1_TAAC_TU_MASK)
#define SD_CSD_V1_TAAC_TU_SET(taac, tu) ((taac) | ((tu) & 7))

#define SD_CSD_V1_TAAC_MF_MASK 0x78
#define SD_CSD_V1_TAAC_MF_GET(taac) (((taac) & SD_CSD_V1_TAAC_MF_MASK) >> 3)
#define SD_CSD_V1_TAAC_MF_SET(taac, mf) ((taac) | (((mf) & 0xF) << 3))

//time unit
#define SD_CSD_V1_TAAC_TU_1NS   0
#define SD_CSD_V1_TAAC_TU_10NS  1
#define SD_CSD_V1_TAAC_TU_100NS 2
#define SD_CSD_V1_TAAC_TU_1US   3
#define SD_CSD_V1_TAAC_TU_10US  4
#define SD_CSD_V1_TAAC_TU_100US 5
#define SD_CSD_V1_TAAC_TU_1MS   6
#define SD_CSD_V1_TAAC_TU_10MS  7

//multiplier factor
#define SD_CSD_V1_TAAC_MF_1_0 1
#define SD_CSD_V1_TAAC_MF_1_2 2
#define SD_CSD_V1_TAAC_MF_1_3 3
#define SD_CSD_V1_TAAC_MF_1_5 4
#define SD_CSD_V1_TAAC_MF_2_0 5
#define SD_CSD_V1_TAAC_MF_2_5 6
#define SD_CSD_V1_TAAC_MF_3_0 7
#define SD_CSD_V1_TAAC_MF_3_5 8
#define SD_CSD_V1_TAAC_MF_4_0 9
#define SD_CSD_V1_TAAC_MF_4_5 0xA
#define SD_CSD_V1_TAAC_MF_5_0 0xB
#define SD_CSD_V1_TAAC_MF_5_5 0xC
#define SD_CSD_V1_TAAC_MF_6_0 0xD
#define SD_CSD_V1_TAAC_MF_7_0 0xE
#define SD_CSD_V1_TAAC_MF_8_0 0xF

//TRAN_SPEED

#define SD_CSD_V1_TRAN_SPEED_TR_MASK 7
#define SD_CSD_V1_TRAN_SPEED_TR_GET(tran_speed) ((tran_speed) & SD_CSD_V1_TRAN_SPEED_TR_MASK)
#define SD_CSD_V1_TRAN_SPEED_TR_SET(tran_speed, tr) ((tran_speed) | ((tr) & 7))

#define SD_CSD_V1_TRAN_SPEED_TV_MASK 0x78
#define SD_CSD_V1_TRAN_SPEED_TV_GET(tran_speed) (((tran_speed) & SD_CSD_V1_TRAN_SPEED_TV_MASK) >> 3)
#define SD_CSD_V1_TRAN_SPEED_TV_SET(tran_speed, tv) ((tran_speed) | (((tv) & 0xF) << 3))

//frequency unit
#define SD_CSD_V1_TRAN_SPEED_TR_100KBITS 0
#define SD_CSD_V1_TRAN_SPEED_TR_1MBITS 1
#define SD_CSD_V1_TRAN_SPEED_TR_10MBITS 2
#define SD_CSD_V1_TRAN_SPEED_TR_100MBITS 3

//multiplier factor
#define SD_CSD_V1_TRAN_SPEED_TV_1_0 1
#define SD_CSD_V1_TRAN_SPEED_TV_1_2 2
#define SD_CSD_V1_TRAN_SPEED_TV_1_3 3
#define SD_CSD_V1_TRAN_SPEED_TV_1_5 4
#define SD_CSD_V1_TRAN_SPEED_TV_2_0 5
#define SD_CSD_V1_TRAN_SPEED_TV_2_5 6
#define SD_CSD_V1_TRAN_SPEED_TV_3_0 7
#define SD_CSD_V1_TRAN_SPEED_TV_3_5 8
#define SD_CSD_V1_TRAN_SPEED_TV_4_0 9
#define SD_CSD_V1_TRAN_SPEED_TV_4_5 0xA
#define SD_CSD_V1_TRAN_SPEED_TV_5_0 0xB
#define SD_CSD_V1_TRAN_SPEED_TV_5_5 0xC
#define SD_CSD_V1_TRAN_SPEED_TV_6_0 0xD
#define SD_CSD_V1_TRAN_SPEED_TV_7_0 0xE
#define SD_CSD_V1_TRAN_SPEED_TV_8_0 0xF

//CCC

#define SD_CSD_V1_CCC_MASK 0xFFF0
#define SD_CSD_V1_CCC_GET_INTERNAL(w4_ccc) (((w4_ccc) & SD_CSD_V1_CCC_MASK) >> 4)
#define SD_CSD_V1_CCC_GET(w4_ccc) (SD_CSD_V1_CCC_GET_INTERNAL(INV_16(w4_ccc)))
#define SD_CSD_V1_CCC_SET(w4_ccc, ccc) ((w4_ccc) | (INV_16((((ccc) & 0xFFF) << 4))))

#define SD_CSD_V1_CCC_CLASS_0  0x001
#define SD_CSD_V1_CCC_CLASS_1  0x002
#define SD_CSD_V1_CCC_CLASS_2  0x004
#define SD_CSD_V1_CCC_CLASS_3  0x008
#define SD_CSD_V1_CCC_CLASS_4  0x010
#define SD_CSD_V1_CCC_CLASS_5  0x020
#define SD_CSD_V1_CCC_CLASS_6  0x040
#define SD_CSD_V1_CCC_CLASS_7  0x080
#define SD_CSD_V1_CCC_CLASS_8  0x100
#define SD_CSD_V1_CCC_CLASS_9  0x200
#define SD_CSD_V1_CCC_CLASS_10 0x400
#define SD_CSD_V1_CCC_CLASS_11 0x800

#define SD_CSD_V1_CCC_CLASS_GET_INTERNAL(w4_ccc, cl) ((((w4_ccc) & SD_CSD_V1_CCC_MASK) >> 4) & (cl))
#define SD_CSD_V1_CCC_CLASS_GET(w4_ccc, cl) (SD_CSD_V1_CCC_CLASS_GET_INTERNAL(INV_16(w4_ccc), cl))
#define SD_CSD_V1_CCC_CLASS_SET(w4_ccc, cl) ((w4_ccc) | (INV_16((((cl) & 0xFFF) << 4))))

//READ_BL_LEN

#define SD_CSD_V1_READ_BL_LEN_MASK 0xF
#define SD_CSD_V1_READ_BL_LEN_GET_INTERNAL(w4_rbl) ((w4_rbl) & SD_CSD_V1_READ_BL_LEN_MASK)
#define SD_CSD_V1_READ_BL_LEN_GET(w4_rbl) (SD_CSD_V1_READ_BL_LEN_GET_INTERNAL(INV_16(w4_rbl)))
#define SD_CSD_V1_READ_BL_LEN_SET(w4_rbl, rbl) ((w4_rbl) | (INV_16(((rbl) & SD_CSD_V1_READ_BL_LEN_MASK))))

#define SD_CSD_V1_READ_BL_LEN_512B    9
#define SD_CSD_V1_READ_BL_LEN_1KB     10
#define SD_CSD_V1_READ_BL_LEN_2KB     11

//_qw6

#define SD_CSD_V1_READ_BL_PARTIAL_MASK    0x8000000000000000LL

#define SD_CSD_V1_READ_BL_PARTIAL_GET(qw6) ((INV_64(qw6)) & SD_CSD_V1_READ_BL_PARTIAL_MASK)
#define SD_CSD_V1_READ_BL_PARTIAL_SET(qw6) ((qw6) | (INV_64(SD_CSD_V1_READ_BL_PARTIAL_MASK)))

//

#define SD_CSD_V1_WRITE_BLK_MISALIGN_MASK 0x4000000000000000LL

#define SD_CSD_V1_WRITE_BLK_MISALIGN_GET(qw6) ((INV_64(qw6)) & SD_CSD_V1_WRITE_BLK_MISALIGN_MASK)
#define SD_CSD_V1_WRITE_BLK_MISALIGN_SET(qw6) ((qw6) | (INV_64(SD_CSD_V1_WRITE_BLK_MISALIGN_MASK)))

//

#define SD_CSD_V1_READ_BLK_MISALIGN_MASK  0x2000000000000000LL

#define SD_CSD_V1_READ_BLK_MISALIGN_GET(qw6) ((INV_64(qw6)) & SD_CSD_V1_READ_BLK_MISALIGN_MASK)
#define SD_CSD_V1_READ_BLK_MISALIGN_SET(qw6) ((qw6) | (INV_64(SD_CSD_V1_READ_BLK_MISALIGN_MASK)))

//

#define SD_CSD_V1_DSR_IMP_MASK            0x1000000000000000LL

#define SD_CSD_V1_DSR_IMP_GET(qw6) ((INV_64(qw6)) & SD_CSD_V1_DSR_IMP_MASK)
#define SD_CSD_V1_DSR_IMP_SET(qw6) ((qw6) | (INV_64(SD_CSD_V1_DSR_IMP_MASK)))

//

#define SD_CSD_V1_C_SIZE_MASK             0x03FFC00000000000LL

#define SD_CSD_V1_C_SIZE_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_C_SIZE_MASK) >> 46)
#define SD_CSD_V1_C_SIZE_GET(qw6) (SD_CSD_V1_C_SIZE_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_C_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0xFFF) << 46)))

//

#define SD_CSD_V1_VDD_R_CURR_MIN_MASK     0x0000380000000000LL

#define SD_CSD_V1_VDD_R_CURR_MIN_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_VDD_R_CURR_MIN_MASK) >> 43)
#define SD_CSD_V1_VDD_R_CURR_MIN_GET(qw6) (SD_CSD_V1_VDD_R_CURR_MIN_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_VDD_R_CURR_MIN_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 43)))

#define SD_CSD_V1_VDD_R_CURR_MIN_0_5MA 0
#define SD_CSD_V1_VDD_R_CURR_MIN_1MA   1
#define SD_CSD_V1_VDD_R_CURR_MIN_5MA   2
#define SD_CSD_V1_VDD_R_CURR_MIN_10MA  3
#define SD_CSD_V1_VDD_R_CURR_MIN_25MA  4
#define SD_CSD_V1_VDD_R_CURR_MIN_35MA  5
#define SD_CSD_V1_VDD_R_CURR_MIN_60MA  6
#define SD_CSD_V1_VDD_R_CURR_MIN_100MA 7

//

#define SD_CSD_V1_VDD_R_CURR_MAX_MASK     0x0000070000000000LL

#define SD_CSD_V1_VDD_R_CURR_MAX_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_VDD_R_CURR_MAX_MASK) >> 40)
#define SD_CSD_V1_VDD_R_CURR_MAX_GET(qw6) (SD_CSD_V1_VDD_R_CURR_MAX_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_VDD_R_CURR_MAX_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 40)))

#define SD_CSD_V1_VDD_R_CURR_MAX_1MA   0
#define SD_CSD_V1_VDD_R_CURR_MAX_5MA   1
#define SD_CSD_V1_VDD_R_CURR_MAX_10MA  2
#define SD_CSD_V1_VDD_R_CURR_MAX_25MA  3
#define SD_CSD_V1_VDD_R_CURR_MAX_35MA  4
#define SD_CSD_V1_VDD_R_CURR_MAX_45MA  5
#define SD_CSD_V1_VDD_R_CURR_MAX_80MA  6
#define SD_CSD_V1_VDD_R_CURR_MAX_200MA 7

//

#define SD_CSD_V1_VDD_W_CURR_MIN_MASK     0x000000E000000000LL

#define SD_CSD_V1_VDD_W_CURR_MIN_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_VDD_W_CURR_MIN_MASK) >> 37)
#define SD_CSD_V1_VDD_W_CURR_MIN_GET(qw6) (SD_CSD_V1_VDD_W_CURR_MIN_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_VDD_W_CURR_MIN_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 37)))

#define SD_CSD_V1_VDD_W_CURR_MIN_0_5MA 0
#define SD_CSD_V1_VDD_W_CURR_MIN_1MA   1
#define SD_CSD_V1_VDD_W_CURR_MIN_5MA   2
#define SD_CSD_V1_VDD_W_CURR_MIN_10MA  3
#define SD_CSD_V1_VDD_W_CURR_MIN_25MA  4
#define SD_CSD_V1_VDD_W_CURR_MIN_35MA  5
#define SD_CSD_V1_VDD_W_CURR_MIN_60MA  6
#define SD_CSD_V1_VDD_W_CURR_MIN_100MA 7

//

#define SD_CSD_V1_VDD_W_CURR_MAX_MASK     0x0000001C00000000LL

#define SD_CSD_V1_VDD_W_CURR_MAX_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_VDD_W_CURR_MAX_MASK) >> 34)
#define SD_CSD_V1_VDD_W_CURR_MAX_GET(qw6) (SD_CSD_V1_VDD_W_CURR_MAX_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_VDD_W_CURR_MAX_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 34)))

#define SD_CSD_V1_VDD_W_CURR_MAX_1MA   0
#define SD_CSD_V1_VDD_W_CURR_MAX_5MA   1
#define SD_CSD_V1_VDD_W_CURR_MAX_10MA  2
#define SD_CSD_V1_VDD_W_CURR_MAX_25MA  3
#define SD_CSD_V1_VDD_W_CURR_MAX_35MA  4
#define SD_CSD_V1_VDD_W_CURR_MAX_45MA  5
#define SD_CSD_V1_VDD_W_CURR_MAX_80MA  6
#define SD_CSD_V1_VDD_W_CURR_MAX_200MA 7

//

#define SD_CSD_V1_C_SIZE_MULT_MASK        0x0000000380000000LL

#define SD_CSD_V1_C_SIZE_MULT_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_C_SIZE_MULT_MASK) >> 31)
#define SD_CSD_V1_C_SIZE_MULT_GET(qw6) (SD_CSD_V1_C_SIZE_MULT_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_C_SIZE_MULT_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 31)))

#define SD_CSD_V1_C_SIZE_MULT_4   0
#define SD_CSD_V1_C_SIZE_MULT_8   1
#define SD_CSD_V1_C_SIZE_MULT_16  2
#define SD_CSD_V1_C_SIZE_MULT_32  3
#define SD_CSD_V1_C_SIZE_MULT_64  4
#define SD_CSD_V1_C_SIZE_MULT_128 5
#define SD_CSD_V1_C_SIZE_MULT_256 6
#define SD_CSD_V1_C_SIZE_MULT_512 7

//

#define SD_CSD_V1_ERASE_BLK_EN_MASK       0x0000000040000000LL

#define SD_CSD_V1_ERASE_BLK_EN_GET(qw6) ((INV_64(qw6)) & SD_CSD_V1_ERASE_BLK_EN_MASK)
#define SD_CSD_V1_ERASE_BLK_EN_SET(qw6) ((qw6) | (INV_64(SD_CSD_V1_ERASE_BLK_EN_MASK)))

//

#define SD_CSD_V1_SECTOR_SIZE_MASK        0x000000003F800000LL

#define SD_CSD_V1_SECTOR_SIZE_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_SECTOR_SIZE_MASK) >> 23)
#define SD_CSD_V1_SECTOR_SIZE_GET(qw6) (SD_CSD_V1_SECTOR_SIZE_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_SECTOR_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7F) << 23)))

//

#define SD_CSD_V1_WP_GRP_SIZE_MASK        0x00000000007F0000LL

#define SD_CSD_V1_WP_GRP_SIZE_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_WP_GRP_SIZE_MASK) >> 16)
#define SD_CSD_V1_WP_GRP_SIZE_GET(qw6) (SD_CSD_V1_WP_GRP_SIZE_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_WP_GRP_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7F) << 16)))

//

#define SD_CSD_V1_WP_GRP_ENABLE_MASK      0x0000000000008000LL

#define SD_CSD_V1_WP_GRP_ENABLE_GET(qw6) ((INV_64(qw6)) & SD_CSD_V1_WP_GRP_ENABLE_MASK)
#define SD_CSD_V1_WP_GRP_ENABLE_SET(qw6) ((qw6) | (INV_64(SD_CSD_V1_WP_GRP_ENABLE_MASK)))

//

#define SD_CSD_V1_R2W_FACTOR_MASK         0x0000000000001C00LL

#define SD_CSD_V1_R2W_FACTOR_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_R2W_FACTOR_MASK) >> 10)
#define SD_CSD_V1_R2W_FACTOR_GET(qw6) (SD_CSD_V1_R2W_FACTOR_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_R2W_FACTOR_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 10)))

#define SD_CSD_V1_R2W_FACTOR_1   0
#define SD_CSD_V1_R2W_FACTOR_2   1
#define SD_CSD_V1_R2W_FACTOR_4   2
#define SD_CSD_V1_R2W_FACTOR_8   3
#define SD_CSD_V1_R2W_FACTOR_16  4
#define SD_CSD_V1_R2W_FACTOR_32  5

//

#define SD_CSD_V1_WRITE_BL_LEN_MASK       0x00000000000003C0LL

#define SD_CSD_V1_WRITE_BL_LEN_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V1_WRITE_BL_LEN_MASK) >> 6)
#define SD_CSD_V1_WRITE_BL_LEN_GET(qw6) (SD_CSD_V1_WRITE_BL_LEN_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V1_WRITE_BL_LEN_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0xF) << 6)))

#define SD_CSD_V1_WRITE_BL_LEN_512B    9
#define SD_CSD_V1_WRITE_BL_LEN_1KB     10
#define SD_CSD_V1_WRITE_BL_LEN_2KB     11

//

#define SD_CSD_V1_WRITE_BL_PARTIAL_MASK   0x0000000000000020LL

#define SD_CSD_V1_WRITE_BL_PARTIAL_GET(qw6) ((INV_64(qw6)) & SD_CSD_V1_WRITE_BL_PARTIAL_MASK)
#define SD_CSD_V1_WRITE_BL_PARTIAL_SET(qw6) ((qw6) | (INV_64(SD_CSD_V1_WRITE_BL_PARTIAL_MASK)))

//_b14

#define SD_CSD_V1_FILE_FORMAT_GRP_MASK    0x80
#define SD_CSD_V1_FILE_FORMAT_GRP_GET(b14) ((b14) & SD_CSD_V1_FILE_FORMAT_GRP_MASK)
#define SD_CSD_V1_FILE_FORMAT_GRP_SET(b14) ((b14) | (SD_CSD_V1_FILE_FORMAT_GRP_MASK))

//

#define SD_CSD_V1_COPY_MASK               0x40
#define SD_CSD_V1_COPY_GET(b14) ((b14) & SD_CSD_V1_COPY_MASK)
#define SD_CSD_V1_COPY_SET(b14) ((b14) | (SD_CSD_V1_COPY_MASK))

//

#define SD_CSD_V1_PERM_WRITE_PROTECT_MASK 0x20
#define SD_CSD_V1_PERM_WRITE_PROTECT_GET(b14) ((b14) & SD_CSD_V1_PERM_WRITE_PROTECT_MASK)
#define SD_CSD_V1_PERM_WRITE_PROTECT_SET(b14) ((b14) | (SD_CSD_V1_PERM_WRITE_PROTECT_MASK))

//

#define SD_CSD_V1_TMP_WRITE_PROTECT_MASK  0x10
#define SD_CSD_V1_TMP_WRITE_PROTECT_GET(b14) ((b14) & SD_CSD_V1_TMP_WRITE_PROTECT_MASK)
#define SD_CSD_V1_TMP_WRITE_PROTECT_SET(b14) ((b14) | (SD_CSD_V1_TMP_WRITE_PROTECT_MASK))

//

#define SD_CSD_V1_FILE_FORMAT_MASK        0x0C
#define SD_CSD_V1_FILE_FORMAT_GET(b14) (((b14) & SD_CSD_V1_FILE_FORMAT_MASK) >> 2)
#define SD_CSD_V1_FILE_FORMAT_SET(b14, value) ((b14) | (((value) & 0x3) << 2))

#define SD_CSD_V1_FILE_FORMAT_HARD_DISK 0
#define SD_CSD_V1_FILE_FORMAT_DOS_FAT 1
#define SD_CSD_V1_FILE_FORMAT_UFF 2
#define SD_CSD_V1_FILE_FORMAT_OTHERS 3

//========== CSD V2 ==========

typedef struct SD_CSD_V2
{
   union SD_CSD_V2_b0
   {
      uint8_t CSD_STRUCTURE; // [127:126]
      //Reserved             // [125:120]
   }b0;

   uint8_t TAAC;       // [119:112]
   uint8_t NSAC;       // [111:104]
   uint8_t TRAN_SPEED; // [103:96]

   union SD_CSD_V2_w4
   {
      uint16_t CCC;         // [95:84]
      uint16_t READ_BL_LEN; // [83:80]
   } w4;

   union SD_CSD_V2_qw6
   {
      uint64_t READ_BL_PARTIAL;    // [79:79]
      uint64_t WRITE_BLK_MISALIGN; // [78:78]
      uint64_t READ_BLK_MISALIGN;  // [77:77]
      uint64_t DSR_IMP;            // [76:76]

      //Reserved                      [75:70]

      uint64_t C_SIZE;             // [69:48]

      //Reserved                      [47:47]

      uint64_t ERASE_BLK_EN;       // [46:46]
      uint64_t SECTOR_SIZE;        // [45:39]
      uint64_t WP_GRP_SIZE;        // [38:32]

      uint64_t WP_GRP_ENABLE;      // [31:31]

      //Reserved                      [30:29]

      uint64_t R2W_FACTOR; //         [28:26]
      uint64_t WRITE_BL_LEN; //       [25:22]
      uint64_t WRITE_BL_PARTIAL; //   [21:21]

      //Reserved                      [20:16]
   }qw6;

   union SD_CSD_V2_b14
   {
      uint8_t FILE_FORMAT_GRP;    // [15:15]
      uint8_t COPY;               // [14:14]
      uint8_t PERM_WRITE_PROTECT; // [13:13]
      uint8_t TMP_WRITE_PROTECT;  // [12:12]
      uint8_t FILE_FORMAT;        // [11:10]
      //Reserved                     [9:8]
   }b14;

   uint8_t CRC7; // [7:0]

}SD_CSD_V2;

//TAAC

#define SD_CSD_V2_TAAC_TU_MASK 7
#define SD_CSD_V2_TAAC_TU_GET(taac) ((taac) & SD_CSD_V2_TAAC_TU_MASK)
#define SD_CSD_V2_TAAC_TU_SET(taac, tu) ((taac) | ((tu) & 7))

#define SD_CSD_V2_TAAC_MF_MASK 0x78
#define SD_CSD_V2_TAAC_MF_GET(taac) (((taac) & SD_CSD_V2_TAAC_MF_MASK) >> 3)
#define SD_CSD_V2_TAAC_MF_SET(taac, mf) ((taac) | (((mf) & 0xF) << 3))

//time unit
#define SD_CSD_V2_TAAC_TU_1NS   0
#define SD_CSD_V2_TAAC_TU_10NS  1
#define SD_CSD_V2_TAAC_TU_100NS 2
#define SD_CSD_V2_TAAC_TU_1US   3
#define SD_CSD_V2_TAAC_TU_10US  4
#define SD_CSD_V2_TAAC_TU_100US 5
#define SD_CSD_V2_TAAC_TU_1MS   6
#define SD_CSD_V2_TAAC_TU_10MS  7

//multiplier factor
#define SD_CSD_V2_TAAC_MF_1_0 1
#define SD_CSD_V2_TAAC_MF_1_2 2
#define SD_CSD_V2_TAAC_MF_1_3 3
#define SD_CSD_V2_TAAC_MF_1_5 4
#define SD_CSD_V2_TAAC_MF_2_0 5
#define SD_CSD_V2_TAAC_MF_2_5 6
#define SD_CSD_V2_TAAC_MF_3_0 7
#define SD_CSD_V2_TAAC_MF_3_5 8
#define SD_CSD_V2_TAAC_MF_4_0 9
#define SD_CSD_V2_TAAC_MF_4_5 0xA
#define SD_CSD_V2_TAAC_MF_5_0 0xB
#define SD_CSD_V2_TAAC_MF_5_5 0xC
#define SD_CSD_V2_TAAC_MF_6_0 0xD
#define SD_CSD_V2_TAAC_MF_7_0 0xE
#define SD_CSD_V2_TAAC_MF_8_0 0xF

//TRAN_SPEED

#define SD_CSD_V2_TRAN_SPEED_TR_MASK 7
#define SD_CSD_V2_TRAN_SPEED_TR_GET(tran_speed) ((tran_speed) & SD_CSD_V2_TRAN_SPEED_TR_MASK)
#define SD_CSD_V2_TRAN_SPEED_TR_SET(tran_speed, tr) ((tran_speed) | ((tr) & 7))

#define SD_CSD_V2_TRAN_SPEED_TV_MASK 0x78
#define SD_CSD_V2_TRAN_SPEED_TV_GET(tran_speed) (((tran_speed) & SD_CSD_V2_TRAN_SPEED_TV_MASK) >> 3)
#define SD_CSD_V2_TRAN_SPEED_TV_SET(tran_speed, tv) ((tran_speed) | (((tv) & 0xF) << 3))

//frequency unit
#define SD_CSD_V2_TRAN_SPEED_TR_100KBITS 0
#define SD_CSD_V2_TRAN_SPEED_TR_1MBITS 1
#define SD_CSD_V2_TRAN_SPEED_TR_10MBITS 2
#define SD_CSD_V2_TRAN_SPEED_TR_100MBITS 3

//multiplier factor
#define SD_CSD_V2_TRAN_SPEED_TV_1_0 1
#define SD_CSD_V2_TRAN_SPEED_TV_1_2 2
#define SD_CSD_V2_TRAN_SPEED_TV_1_3 3
#define SD_CSD_V2_TRAN_SPEED_TV_1_5 4
#define SD_CSD_V2_TRAN_SPEED_TV_2_0 5
#define SD_CSD_V2_TRAN_SPEED_TV_2_5 6
#define SD_CSD_V2_TRAN_SPEED_TV_3_0 7
#define SD_CSD_V2_TRAN_SPEED_TV_3_5 8
#define SD_CSD_V2_TRAN_SPEED_TV_4_0 9
#define SD_CSD_V2_TRAN_SPEED_TV_4_5 0xA
#define SD_CSD_V2_TRAN_SPEED_TV_5_0 0xB
#define SD_CSD_V2_TRAN_SPEED_TV_5_5 0xC
#define SD_CSD_V2_TRAN_SPEED_TV_6_0 0xD
#define SD_CSD_V2_TRAN_SPEED_TV_7_0 0xE
#define SD_CSD_V2_TRAN_SPEED_TV_8_0 0xF

//CCC

#define SD_CSD_V2_CCC_MASK 0xFFF0
#define SD_CSD_V2_CCC_GET_INTERNAL(w4_ccc) (((w4_ccc) & SD_CSD_V2_CCC_MASK) >> 4)
#define SD_CSD_V2_CCC_GET(w4_ccc) (SD_CSD_V2_CCC_GET_INTERNAL(INV_16(w4_ccc)))
#define SD_CSD_V2_CCC_SET(w4_ccc, ccc) ((w4_ccc) | (INV_16((((ccc) & 0xFFF) << 4))))

#define SD_CSD_V2_CCC_CLASS_0  0x001
#define SD_CSD_V2_CCC_CLASS_1  0x002
#define SD_CSD_V2_CCC_CLASS_2  0x004
#define SD_CSD_V2_CCC_CLASS_3  0x008
#define SD_CSD_V2_CCC_CLASS_4  0x010
#define SD_CSD_V2_CCC_CLASS_5  0x020
#define SD_CSD_V2_CCC_CLASS_6  0x040
#define SD_CSD_V2_CCC_CLASS_7  0x080
#define SD_CSD_V2_CCC_CLASS_8  0x100
#define SD_CSD_V2_CCC_CLASS_9  0x200
#define SD_CSD_V2_CCC_CLASS_10 0x400
#define SD_CSD_V2_CCC_CLASS_11 0x800

#define SD_CSD_V2_CCC_CLASS_GET_INTERNAL(w4_ccc, cl) ((((w4_ccc) & SD_CSD_V2_CCC_MASK) >> 4) & (cl))
#define SD_CSD_V2_CCC_CLASS_GET(w4_ccc, cl) (SD_CSD_V2_CCC_CLASS_GET_INTERNAL(INV_16(w4_ccc), cl))
#define SD_CSD_V2_CCC_CLASS_SET(w4_ccc, cl) ((w4_ccc) | (INV_16((((cl) & 0xFFF) << 4))))

//READ_BL_LEN

#define SD_CSD_V2_READ_BL_LEN_MASK 0xF
#define SD_CSD_V2_READ_BL_LEN_GET_INTERNAL(w4_rbl) ((w4_rbl) & SD_CSD_V2_READ_BL_LEN_MASK)
#define SD_CSD_V2_READ_BL_LEN_GET(w4_rbl) (SD_CSD_V2_READ_BL_LEN_GET_INTERNAL(INV_16(w4_rbl)))
#define SD_CSD_V2_READ_BL_LEN_SET(w4_rbl, rbl) ((w4_rbl) | (INV_16(((rbl) & SD_CSD_V2_READ_BL_LEN_MASK))))

#define SD_CSD_V2_READ_BL_LEN_512B    9

//======================================================

//_qw6

#define SD_CSD_V2_READ_BL_PARTIAL_MASK    0x8000000000000000LL

#define SD_CSD_V2_READ_BL_PARTIAL_GET(qw6) ((INV_64(qw6)) & SD_CSD_V2_READ_BL_PARTIAL_MASK)
#define SD_CSD_V2_READ_BL_PARTIAL_SET(qw6) ((qw6) | (INV_64(SD_CSD_V2_READ_BL_PARTIAL_MASK)))

//

#define SD_CSD_V2_WRITE_BLK_MISALIGN_MASK 0x4000000000000000LL

#define SD_CSD_V2_WRITE_BLK_MISALIGN_GET(qw6) ((INV_64(qw6)) & SD_CSD_V2_WRITE_BLK_MISALIGN_MASK)
#define SD_CSD_V2_WRITE_BLK_MISALIGN_SET(qw6) ((qw6) | (INV_64(SD_CSD_V2_WRITE_BLK_MISALIGN_MASK)))

//

#define SD_CSD_V2_READ_BLK_MISALIGN_MASK  0x2000000000000000LL

#define SD_CSD_V2_READ_BLK_MISALIGN_GET(qw6) ((INV_64(qw6)) & SD_CSD_V2_READ_BLK_MISALIGN_MASK)
#define SD_CSD_V2_READ_BLK_MISALIGN_SET(qw6) ((qw6) | (INV_64(SD_CSD_V2_READ_BLK_MISALIGN_MASK)))

//

#define SD_CSD_V2_DSR_IMP_MASK            0x1000000000000000LL

#define SD_CSD_V2_DSR_IMP_GET(qw6) ((INV_64(qw6)) & SD_CSD_V2_DSR_IMP_MASK)
#define SD_CSD_V2_DSR_IMP_SET(qw6) ((qw6) | (INV_64(SD_CSD_V2_DSR_IMP_MASK)))

//

#define SD_CSD_V2_C_SIZE_MASK             0x003FFFFF00000000LL

#define SD_CSD_V2_C_SIZE_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V2_C_SIZE_MASK) >> 32)
#define SD_CSD_V2_C_SIZE_GET(qw6) (SD_CSD_V2_C_SIZE_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V2_C_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x3FFFFF) << 32)))

//

#define SD_CSD_V2_ERASE_BLK_EN_MASK       0x0000000040000000LL

#define SD_CSD_V2_ERASE_BLK_EN_GET(qw6) ((INV_64(qw6)) & SD_CSD_V2_ERASE_BLK_EN_MASK)
#define SD_CSD_V2_ERASE_BLK_EN_SET(qw6) ((qw6) | (INV_64(SD_CSD_V2_ERASE_BLK_EN_MASK)))

//

#define SD_CSD_V2_SECTOR_SIZE_MASK        0x000000003F800000LL

#define SD_CSD_V2_SECTOR_SIZE_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V2_SECTOR_SIZE_MASK) >> 23)
#define SD_CSD_V2_SECTOR_SIZE_GET(qw6) (SD_CSD_V2_SECTOR_SIZE_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V2_SECTOR_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7F) << 23)))

//

#define SD_CSD_V2_WP_GRP_SIZE_MASK        0x00000000007F0000LL

#define SD_CSD_V2_WP_GRP_SIZE_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V2_WP_GRP_SIZE_MASK) >> 16)
#define SD_CSD_V2_WP_GRP_SIZE_GET(qw6) (SD_CSD_V2_WP_GRP_SIZE_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V2_WP_GRP_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7F) << 16)))

//

#define SD_CSD_V2_WP_GRP_ENABLE_MASK      0x0000000000008000LL

#define SD_CSD_V2_WP_GRP_ENABLE_GET(qw6) ((INV_64(qw6)) & SD_CSD_V2_WP_GRP_ENABLE_MASK)
#define SD_CSD_V2_WP_GRP_ENABLE_SET(qw6) ((qw6) | (INV_64(SD_CSD_V2_WP_GRP_ENABLE_MASK)))

//

#define SD_CSD_V2_R2W_FACTOR_MASK         0x0000000000001C00LL

#define SD_CSD_V2_R2W_FACTOR_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V2_R2W_FACTOR_MASK) >> 10)
#define SD_CSD_V2_R2W_FACTOR_GET(qw6) (SD_CSD_V2_R2W_FACTOR_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V2_R2W_FACTOR_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 10)))

#define SD_CSD_V2_R2W_FACTOR_4   2

//

#define SD_CSD_V2_WRITE_BL_LEN_MASK       0x00000000000003C0LL

#define SD_CSD_V2_WRITE_BL_LEN_GET_INTERNAL(qw6) (((qw6) & SD_CSD_V2_WRITE_BL_LEN_MASK) >> 6)
#define SD_CSD_V2_WRITE_BL_LEN_GET(qw6) (SD_CSD_V2_WRITE_BL_LEN_GET_INTERNAL(INV_64(qw6)))
#define SD_CSD_V2_WRITE_BL_LEN_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0xF) << 6)))

#define SD_CSD_V2_WRITE_BL_LEN_512B    9

//

#define SD_CSD_V2_WRITE_BL_PARTIAL_MASK   0x0000000000000020LL

#define SD_CSD_V2_WRITE_BL_PARTIAL_GET(qw6) ((INV_64(qw6)) & SD_CSD_V2_WRITE_BL_PARTIAL_MASK)
#define SD_CSD_V2_WRITE_BL_PARTIAL_SET(qw6) ((qw6) | (INV_64(SD_CSD_V2_WRITE_BL_PARTIAL_MASK)))

//_b14

#define SD_CSD_V2_FILE_FORMAT_GRP_MASK    0x80
#define SD_CSD_V2_FILE_FORMAT_GRP_GET(b14) ((b14) & SD_CSD_V2_FILE_FORMAT_GRP_MASK)
#define SD_CSD_V2_FILE_FORMAT_GRP_SET(b14) ((b14) | (SD_CSD_V2_FILE_FORMAT_GRP_MASK))

//

#define SD_CSD_V2_COPY_MASK               0x40
#define SD_CSD_V2_COPY_GET(b14) ((b14) & SD_CSD_V2_COPY_MASK)
#define SD_CSD_V2_COPY_SET(b14) ((b14) | (SD_CSD_V2_COPY_MASK))

//

#define SD_CSD_V2_PERM_WRITE_PROTECT_MASK 0x20
#define SD_CSD_V2_PERM_WRITE_PROTECT_GET(b14) ((b14) & SD_CSD_V2_PERM_WRITE_PROTECT_MASK)
#define SD_CSD_V2_PERM_WRITE_PROTECT_SET(b14) ((b14) | (SD_CSD_V2_PERM_WRITE_PROTECT_MASK))

//

#define SD_CSD_V2_TMP_WRITE_PROTECT_MASK  0x10
#define SD_CSD_V2_TMP_WRITE_PROTECT_GET(b14) ((b14) & SD_CSD_V2_TMP_WRITE_PROTECT_MASK)
#define SD_CSD_V2_TMP_WRITE_PROTECT_SET(b14) ((b14) | (SD_CSD_V2_TMP_WRITE_PROTECT_MASK))

//

#define SD_CSD_V2_FILE_FORMAT_MASK        0x0C
#define SD_CSD_V2_FILE_FORMAT_GET(b14) (((b14) & SD_CSD_V2_FILE_FORMAT_MASK) >> 2)
#define SD_CSD_V2_FILE_FORMAT_SET(b14, value) ((b14) | (((value) & 0x3) << 2))

#define SD_CSD_V2_FILE_FORMAT_HARD_DISK 0

//========== SCR ==========

//version of SCR register changes with each standard

#define SD_SCR_SD_SPEC_1_01 0
#define SD_SCR_SD_SPEC_1_10 1
#define SD_SCR_SD_SPEC_2_00 2
#define SD_SCR_SD_SPEC_3_01 2
#define SD_SCR_SD_SPEC_4_10 2
#define SD_SCR_SD_SPEC_5_00 2
#define SD_SCR_SD_SPEC_6_00 2

//

#define SD_SCR_SCR_STRUCTURE_MASK 0xF0
#define SD_SCR_SCR_STRUCTURE_GET(b0) (((b0) & SD_SCR_SCR_STRUCTURE_MASK) >> 4)
#define SD_SCR_SCR_STRUCTURE_SET(b0, value) ((b0) | (((value) & 0xF) << 4))

//

#define SD_SCR_SD_SPEC_MASK 0x0F
#define SD_SCR_SD_SPEC_GET(b0) ((b0) & SD_SCR_SD_SPEC_MASK)
#define SD_SCR_SD_SPEC_SET(b0, value) ((b0) | ((value) & SD_SCR_SD_SPEC_MASK))

//========== SD_SCR_1_01 ==========

//April 15 2001
typedef struct SD_SCR_V1_01
{
   //TODO
}SD_SCR_V1_01;

//========== SD_SCR_1_10 ==========

//March 18, 2005
typedef struct SD_SCR_V1_10
{
   union SD_SCR_V1_10_b0
   {
      uint8_t SCR_STRUCTURE; // [63:60]
      uint8_t SD_SPEC; //       [59:56]
   } b0;

   union SD_SCR_V1_10_b1
   {
      uint8_t DATA_STAT_AFTER_ERASE; // [55:55]
      uint8_t SD_SECURITY; //           [54:52]
      uint8_t SD_BUS_WIDTHS; //         [51:48]
   } b1;

   uint16_t Reserved; //[47:32]
   uint32_t Reserved_Man; // [31:0]

}SD_SCR_V1_10;

//========== SD_SCR_2_00 ==========

//September 25, 2006
typedef struct SD_SCR_V2_00
{
   union SD_SCR_V2_00_b0
   {
      uint8_t SCR_STRUCTURE; // [63:60]
      uint8_t SD_SPEC; //       [59:56]
   } b0;

   union SD_SCR_V2_00_b1
   {
      uint8_t DATA_STAT_AFTER_ERASE; // [55:55]
      uint8_t SD_SECURITY; //           [54:52]
      uint8_t SD_BUS_WIDTHS; //         [51:48]
   } b1;

   uint16_t Reserved; //[47:32]
   uint32_t Reserved_Man; // [31:0]

}SD_SCR_V2_00;

//========== SD_SCR_3_01 ==========

//May 18, 2010
typedef struct SD_SCR_V3_01
{
   union SD_SCR_V3_01_b0
   {
      uint8_t SCR_STRUCTURE; // [63:60]
      uint8_t SD_SPEC; //       [59:56]
   } b0;

   union SD_SCR_V3_01_b1
   {
      uint8_t DATA_STAT_AFTER_ERASE; // [55:55]
      uint8_t SD_SECURITY; //           [54:52]
      uint8_t SD_BUS_WIDTHS; //         [51:48]
   } b1;

   union SD_SCR_V3_01_w2
   {
      uint16_t SD_SPEC3; //    [47]
      uint16_t EX_SECURITY; // [46:43]
      //Reserved;              [42:34]
      uint16_t CMD_SUPPORT; // [33:32]
   } w2;

   uint32_t Reserved_Man; // [31:0]

}SD_SCR_V3_01;

//========== SD_SCR_4_10 ==========

//January 22, 2013
typedef struct SD_SCR_V4_10
{
   union SD_SCR_V4_10_b0
   {
      uint8_t SCR_STRUCTURE; // [63:60]
      uint8_t SD_SPEC; //       [59:56]
   } b0;

   union SD_SCR_V4_10_b1
   {
      uint8_t DATA_STAT_AFTER_ERASE; // [55:55]
      uint8_t SD_SECURITY; //           [54:52]
      uint8_t SD_BUS_WIDTHS; //         [51:48]
   } b1;

   union SD_SCR_V4_10_w2
   {
      uint16_t SD_SPEC3; //    [47]
      uint16_t EX_SECURITY; // [46:43]
      uint16_t SD_SPEC4; //    [42]
      //Reserved;              [41:36]
      uint16_t CMD_SUPPORT; // [35:32]
   } w2;

   uint32_t Reserved_Man; // [31:0]

}SD_SCR_V4_10;

//========== SD_SCR_5_00 ==========

//August 10, 2016
typedef struct SD_SCR_V5_00
{
   union SD_SCR_V5_00_b0
   {
      uint8_t SCR_STRUCTURE; // [63:60]
      uint8_t SD_SPEC; //       [59:56]
   } b0;

   union SD_SCR_V5_00_b1
   {
      uint8_t DATA_STAT_AFTER_ERASE; // [55:55]
      uint8_t SD_SECURITY; //           [54:52]
      uint8_t SD_BUS_WIDTHS; //         [51:48]
   } b1;

   union SD_SCR_V5_00_w2
   {
      uint16_t SD_SPEC3; //    [47]
      uint16_t EX_SECURITY; // [46:43]
      uint16_t SD_SPEC4; //    [42]
      uint16_t SD_SPECX; //    [41:38]
      //Reserved;              [37:36]
      uint16_t CMD_SUPPORT; // [35:32]
   } w2;

   uint32_t Reserved_Man; // [31:0]

}SD_SCR_V5_00;

//

#define SD_SCR_V5_00_DATA_STAT_AFTER_ERASE_MASK 0x80
#define SD_SCR_V5_00_DATA_STAT_AFTER_ERASE_GET(b1) (((b1) & SD_SCR_V5_00_DATA_STAT_AFTER_ERASE_MASK) >> 7)
#define SD_SCR_V5_00_DATA_STAT_AFTER_ERASE_SET(b1, value) ((b1) | (((value) & 0x01) << 7)) 

//

#define SD_SCR_V5_00_SD_SECURITY_MASK 0x70
#define SD_SCR_V5_00_SD_SECURITY_GET(b1) (((b1) & SD_SCR_V5_00_SD_SECURITY_MASK) >> 4)
#define SD_SCR_V5_00_SD_SECURITY_SET(b1, value) ((b1) | (((value) & 0x07) << 4))

#define SD_SCR_V5_00_SD_SECURITY_NO_SECURITY 0
#define SD_SCR_V5_00_SD_SECURITY_NOT_USED 1
#define SD_SCR_V5_00_SD_SECURITY_SDSC_CARD 2
#define SD_SCR_V5_00_SD_SECURITY_SDHC_CARD 3
#define SD_SCR_V5_00_SD_SECURITY_SDXC_CARD 4

//

#define SD_SCR_V5_00_SD_BUS_WIDTHS_MASK 0x0F
#define SD_SCR_V5_00_SD_BUS_WIDTHS_GET(b1) ((b1) & SD_SCR_V5_00_SD_BUS_WIDTHS_MASK)
#define SD_SCR_V5_00_SD_BUS_WIDTHS_SET(b1, value) (b1) | (((value) & SD_SCR_V5_00_SD_BUS_WIDTHS_MASK))

#define SD_SCR_V5_00_SD_BUS_WIDTHS_1_BIT 0x1
#define SD_SCR_V5_00_SD_BUS_WIDTHS_4_BIT 0x4

//

#define SD_SCR_V5_00_SD_SPEC3_MASK 0x8000
#define SD_SCR_V5_00_SD_SPEC3_GET_INTERNAL(w2) ((w2) & SD_SCR_V5_00_SD_SPEC3_MASK)
#define SD_SCR_V5_00_SD_SPEC3_GET(w2) (SD_SCR_V5_00_SD_SPEC3_GET_INTERNAL(INV_16(w2)))
#define SD_SCR_V5_00_SD_SPEC3_SET(w2) ((w2) | (INV_16(SD_SCR_V5_00_SD_SPEC3_MASK)))

//

#define SD_SCR_V5_00_EX_SECURITY_MASK 0x7800
#define SD_SCR_V5_00_EX_SECURITY_GET_INTERNAL(w2) (((w2) & SD_SCR_V5_00_EX_SECURITY_MASK) >> 11)
#define SD_SCR_V5_00_EX_SECURITY_GET(w2) (SD_SCR_V5_00_EX_SECURITY_GET_INTERNAL(INV_16(w2)))
#define SD_SCR_V5_00_EX_SECURITY_SET(w2, value) ((w2) | (INV_16((((value) & 0xF) << 11))))

//

#define SD_SCR_V5_00_SD_SPEC4_MASK 0x0400
#define SD_SCR_V5_00_SD_SPEC4_GET_INTERNAL(w2) ((w2) & SD_SCR_V5_00_SD_SPEC4_MASK)
#define SD_SCR_V5_00_SD_SPEC4_GET(w2) (SD_SCR_V5_00_SD_SPEC4_GET_INTERNAL(INV_16(w2)))
#define SD_SCR_V5_00_SD_SPEC4_SET(w2) ((w2) | (INV_16(SD_SCR_V5_00_SD_SPEC4_MASK)))

//

#define SD_SCR_V5_00_SD_SPECX_MASK 0x03C0
#define SD_SCR_V5_00_SD_SPECX_GET_INTERNAL(w2) (((w2) & SD_SCR_V5_00_SD_SPECX_MASK) >> 6)
#define SD_SCR_V5_00_SD_SPECX_GET(w2) (SD_SCR_V5_00_SD_SPECX_GET_INTERNAL(INV_16(w2)))
#define SD_SCR_V5_00_SD_SPECX_SET(w2, value) ((w2) | (INV_16((((value) & 0xF) << 6))))

#define SD_SCR_V5_00_SD_SPECX_V5 1

//

#define SD_SCR_V5_00_CMD_SUPPORT_MASK 0x000F
#define SD_SCR_V5_00_CMD_SUPPORT_GET_INTERNAL(w2) ((w2) & SD_SCR_V5_00_CMD_SUPPORT_MASK)
#define SD_SCR_V5_00_CMD_SUPPORT_GET(w2) (SD_SCR_V5_00_CMD_SUPPORT_GET_INTERNAL(INV_16(w2)))
#define SD_SCR_V5_00_CMD_SUPPORT_SET(w2, value) ((w2) | (INV_16(((value) & SD_SCR_V5_00_CMD_SUPPORT_MASK))))

#define SD_SCR_V5_00_CMD_SUPPORT_SPD_CLS_CTRL 0x1
#define SD_SCR_V5_00_CMD_SUPPORT_SET_BLK_CNT  0x2
#define SD_SCR_V5_00_CMD_SUPPORT_EXT_REG_SNGL_BLK 0x4
#define SD_SCR_V5_00_CMD_SUPPORT_EXT_REG_MULT_BLK 0x8

//========== SD_SCR_6_00 ==========

//April 10, 2017
typedef struct SD_SCR_V6_00
{
   union SD_SCR_V6_00_b0
   {
      uint8_t SCR_STRUCTURE; // [63:60]
      uint8_t SD_SPEC; //       [59:56]
   } b0;

   union SD_SCR_V6_00_b1
   {
      uint8_t DATA_STAT_AFTER_ERASE; // [55:55]
      uint8_t SD_SECURITY; //           [54:52]
      uint8_t SD_BUS_WIDTHS; //         [51:48]
   } b1;

   union SD_SCR_V6_00_w2
   {
      uint16_t SD_SPEC3; //    [47]
      uint16_t EX_SECURITY; // [46:43]
      uint16_t SD_SPEC4; //    [42]
      uint16_t SD_SPECX; //    [41:38]
      //Reserved;              [37:36]
      uint16_t CMD_SUPPORT; // [35:32]
   } w2;

   uint32_t Reserved_Man; // [31:0]

}SD_SCR_V6_00;

//========== SSR ==========

//card status register

typedef struct SSR
{
   union SSR_w0
   {
      uint16_t DAT_BUS_WIDTH; // [511:510]
      uint16_t SECURED_MODE; //  [509]
   } w0;

   uint16_t SD_CARD_TYPE; //           [495:480]
   uint32_t SIZE_OF_PROTECTED_AREA; // [479:448]
   uint8_t SPEED_CLASS; //             [447:440]
   uint8_t PERFORMANCE_MOVE; //        [439:432]
   uint8_t AU_SIZE; //                 [431:424]
   uint16_t ERASE_SIZE; //             [423:408]

   union SSR_b13
   {
      uint8_t ERASE_TIMEOUT; //   [407:402]
      uint8_t ERASE_OFFSET; //    [401:400]
   } b13;

   union SSR_b14
   {
      uint8_t UHS_SPEED_GRADE; // [399:396]
      uint8_t UHS_AU_SIZE; //     [395:392]
   } b14;

   uint8_t VIDEO_SPEED_CLASS; //  [391:384]
   uint16_t VSC_AU_SIZE; //       [383:368]
   uint8_t SUS_ADDR[3]; //        [367:344]
   uint8_t Reserved_1[4]; //      [343:312]
   uint8_t Reserved_2[39]; //     [311:0]

}SSR;

// _w0

#define SSR_DAT_BUS_WIDTH_MASK 0xC000
#define SSR_DAT_BUS_WIDTH_GET_INTERNAL(w0) (((w0) & SSR_DAT_BUS_WIDTH_MASK) >> 14)
#define SSR_DAT_BUS_WIDTH_GET(w0) (SSR_DAT_BUS_WIDTH_GET_INTERNAL(INV_16(w0)))
#define SSR_DAT_BUS_WIDTH_SET(w0, value) ((w0) | (INV_16(((value) & 0x03) << 14))) 

#define SSR_DAT_BUS_WIDTH_1BIT 0
#define SSR_DAT_BUS_WIDTH_4BIT 2

//

#define SSR_SECURED_MODE_MASK 0x2000
#define SSR_SECURED_MODE_GET_INTERNAL(w0) ((w0) & SSR_SECURED_MODE_MASK)
#define SSR_SECURED_MODE_GET(w0) (SSR_SECURED_MODE_GET_INTERNAL(INV_16(w0)))
#define SSR_SECURED_MODE_SET(w0) ((w0) | (INV_16(SSR_SECURED_MODE_MASK))) 

// --

#define SSR_SD_CARD_TYPE_MASK 0x00FF
#define SSR_SD_CARD_TYPE_GET_INTERNAL(ct) ((ct) & SSR_SD_CARD_TYPE_MASK)
#define SSR_SD_CARD_TYPE_GET(ct) (SSR_SD_CARD_TYPE_GET_INTERNAL(INV_16(ct)))
#define SSR_SD_CARD_TYPE_SET(ct, value) ((ct) | (INV_16((value) & SSR_SD_CARD_TYPE_MASK)))

#define SSR_SD_CARD_TYPE_REGULAR_SD 0
#define SSR_SD_CARD_TYPE_SD_ROM 1
#define SSR_SD_CARD_TYPE_OTP 2

//

#define SSR_SIZE_OF_PROTECTED_AREA_GET(spa) (INV_32(spa))
#define SSR_SIZE_OF_PROTECTED_AREA_SET(spa, value) ((spa) | (INV_32(value)))

//

#define SSR_SPEED_CLASS_GET(spc) (spc)
#define SSR_SPEED_CLASS_SET(spc, value) ((spc) | (value))

#define SSR_SPEED_CLASS_0 0
#define SSR_SPEED_CLASS_2 1
#define SSR_SPEED_CLASS_4 2
#define SSR_SPEED_CLASS_6 3
#define SSR_SPEED_CLASS_10 4

//

#define SSR_PERFORMANCE_MOVE_GET(pfm) (pfm)
#define SSR_PERFORMANCE_MOVE_SET(pfm, value) ((pfm) | (value))

//

#define SSR_AU_SIZE_MASK 0xF0
#define SSR_AU_SIZE_GET(au) (((au) & SSR_AU_SIZE_MASK) >> 4)
#define SSR_AU_SIZE_SET(au, value) ((au) | (((value) & 0xF) << 4))

#define SSR_AU_SIZE_NOT_DEF  0x0
#define SSR_AU_SIZE_16KB  0x1
#define SSR_AU_SIZE_32KB  0x2
#define SSR_AU_SIZE_64KB  0x3
#define SSR_AU_SIZE_128KB 0x4
#define SSR_AU_SIZE_256KB 0x5
#define SSR_AU_SIZE_512KB 0x6
#define SSR_AU_SIZE_1MB   0x7
#define SSR_AU_SIZE_2MB   0x8
#define SSR_AU_SIZE_4MB   0x9
#define SSR_AU_SIZE_8MB   0xA
#define SSR_AU_SIZE_12MB  0xB
#define SSR_AU_SIZE_16MB  0xC
#define SSR_AU_SIZE_24MB  0xD
#define SSR_AU_SIZE_32MB  0xE
#define SSR_AU_SIZE_64MB  0xF

//

#define SSR_ERASE_SIZE_GET(es) (INV_16(es))
#define SSR_ERASE_SIZE_SET(es, value) ((es) | (INV_16(value)))

// _b13

#define SSR_ERASE_TIMEOUT_MASK 0xFC
#define SSR_ERASE_TIMEOUT_GET(b13) (((b13) & SSR_ERASE_TIMEOUT_MASK) >> 2)
#define SSR_ERASE_TIMEOUT_SET(b13, value) ((b13) | (((value) & 0x3F) << 2))

//

#define SSR_ERASE_OFFSET_MASK 0x03
#define SSR_ERASE_OFFSET_GET(b13) ((b13) & SSR_ERASE_OFFSET_MASK)
#define SSR_ERASE_OFFSET_SET(b13, value) ((b13) | ((value) & SSR_ERASE_OFFSET_MASK))

#define SSR_ERASE_OFFSET_0SEC 0
#define SSR_ERASE_OFFSET_1SEC 1
#define SSR_ERASE_OFFSET_2SEC 2
#define SSR_ERASE_OFFSET_3SEC 3

// _b14

#define SSR_UHS_SPEED_GRADE_MASK 0xF0
#define SSR_UHS_SPEED_GRADE_GET(b14) (((b14) & SSR_UHS_SPEED_GRADE_MASK) >> 4)
#define SSR_UHS_SPEED_GRADE_SET(b14, value) ((b14) | (((value) & 0x0F) << 4))

#define SSR_UHS_SPEED_GRADE_LT10MBS 0
#define SSR_UHS_SPEED_GRADE_GE10MBs 1
#define SSR_UHS_SPEED_GRADE_GE30MBs 3

//

#define SSR_UHS_AU_SIZE_MASK 0x0F
#define SSR_UHS_AU_SIZE_GET(b14) ((b14) & SSR_UHS_AU_SIZE_MASK)
#define SSR_UHS_AU_SIZE_SET(b14, value) ((b14) | ((value) & SSR_UHS_AU_SIZE_MASK))

#define SSR_UHS_AU_SIZE_NOT_DEF   0x0
#define SSR_UHS_AU_SIZE_NOT_USED1 0x1
#define SSR_UHS_AU_SIZE_NOT_USED2 0x2
#define SSR_UHS_AU_SIZE_NOT_USED3 0x3
#define SSR_UHS_AU_SIZE_NOT_USED4 0x4
#define SSR_UHS_AU_SIZE_NOT_USED5 0x5
#define SSR_UHS_AU_SIZE_NOT_USED6 0x6
#define SSR_UHS_AU_SIZE_1MB       0x7
#define SSR_UHS_AU_SIZE_2MB       0x8
#define SSR_UHS_AU_SIZE_4MB       0x9
#define SSR_UHS_AU_SIZE_8MB       0xA
#define SSR_UHS_AU_SIZE_12MB      0xB
#define SSR_UHS_AU_SIZE_16MB      0xC
#define SSR_UHS_AU_SIZE_24MB      0xD
#define SSR_UHS_AU_SIZE_32MB      0xE
#define SSR_UHS_AU_SIZE_64MB      0xF

// ---

#define SSR_VIDEO_SPEED_CLASS_GET(vsc) (vsc)
#define SSR_VIDEO_SPEED_CLASS_SET(vsc, value) ((vsc) | (value))

#define SSR_VIDEO_SPEED_CLASS_0 0
#define SSR_VIDEO_SPEED_CLASS_6 6
#define SSR_VIDEO_SPEED_CLASS_10 10
#define SSR_VIDEO_SPEED_CLASS_30 30
#define SSR_VIDEO_SPEED_CLASS_60 60

//

#define SSR_VSC_AU_SIZE_MASK 0x3FF
#define SSR_VSC_AU_SIZE_GET_INTERNAL(vas) ((vas) & SSR_VSC_AU_SIZE_MASK)
#define SSR_VSC_AU_SIZE_GET(vas) (SSR_VSC_AU_SIZE_GET_INTERNAL(INV_16(vas)))
#define SSR_VSC_AU_SIZE_SET(vas, value) ((vas) | (INV_16((value) & SSR_VSC_AU_SIZE_MASK)))

#define SSR_VSC_AU_SIZE_008 0x008
#define SSR_VSC_AU_SIZE_010 0x010
#define SSR_VSC_AU_SIZE_015 0x015
#define SSR_VSC_AU_SIZE_018 0x018
#define SSR_VSC_AU_SIZE_01B 0x01B
#define SSR_VSC_AU_SIZE_01C 0x01C
#define SSR_VSC_AU_SIZE_01E 0x01E
#define SSR_VSC_AU_SIZE_020 0x020
#define SSR_VSC_AU_SIZE_024 0x024
#define SSR_VSC_AU_SIZE_028 0x028
#define SSR_VSC_AU_SIZE_02A 0x02A
#define SSR_VSC_AU_SIZE_02D 0x02D
#define SSR_VSC_AU_SIZE_030 0x030
#define SSR_VSC_AU_SIZE_036 0x036
#define SSR_VSC_AU_SIZE_038 0x038
#define SSR_VSC_AU_SIZE_03C 0x03C
#define SSR_VSC_AU_SIZE_040 0x040
#define SSR_VSC_AU_SIZE_048 0x048
#define SSR_VSC_AU_SIZE_050 0x050
#define SSR_VSC_AU_SIZE_060 0x060
#define SSR_VSC_AU_SIZE_070 0x070
#define SSR_VSC_AU_SIZE_078 0x078
#define SSR_VSC_AU_SIZE_080 0x080
#define SSR_VSC_AU_SIZE_090 0x090
#define SSR_VSC_AU_SIZE_0A0 0x0A0
#define SSR_VSC_AU_SIZE_0C0 0x0C0
#define SSR_VSC_AU_SIZE_0D8 0x0D8
#define SSR_VSC_AU_SIZE_0E0 0x0E0
#define SSR_VSC_AU_SIZE_0F0 0x0F0
#define SSR_VSC_AU_SIZE_100 0x100
#define SSR_VSC_AU_SIZE_120 0x120
#define SSR_VSC_AU_SIZE_140 0x140
#define SSR_VSC_AU_SIZE_180 0x180
#define SSR_VSC_AU_SIZE_1B0 0x1B0
#define SSR_VSC_AU_SIZE_1C0 0x1C0
#define SSR_VSC_AU_SIZE_1E0 0x1E0
#define SSR_VSC_AU_SIZE_200 0x200

//

#define SSR_SUS_ADDR_GET(sus) ((sus[0] << 16) | (sus[1] << 8) | (sus[2]))
#define SSR_SUS_ADDR_SET(sus, value) sus[0] = (sus[0]) | ((value & 0x00FF0000) >> 16); sus[1] = (sus[1]) | ((value & 0x0000FF00) >> 8); sus[2] = (sus[2]) | (value & 0x000000FF);

//========== Switch Status ==========

typedef struct SW_STATUS_V0
{
   uint16_t MAX_CURR_PWR_CONS; // [511:496]
   uint16_t SPRT_BIT_FUN_GR_6; // [495:480]
   uint16_t SPRT_BIT_FUN_GR_5; // [479:464]
   uint16_t SPRT_BIT_FUN_GR_4; // [463:448]
   uint16_t SPRT_BIT_FUN_GR_3; // [447:432]
   uint16_t SPRT_BIT_FUN_GR_2; // [431:416]
   uint16_t SPRT_BIT_FUN_GR_1; // [415:400]

   union SW_STATUS_V0_b14
   {
      uint8_t FUN_SEL_FUN_GR_6; // [399:396]
      uint8_t FUN_SEL_FUN_GR_5; // [395:392]
   }b14;

   union SW_STATUS_V0_b15
   {
      uint8_t FUN_SEL_FUN_GR_4; // [391:388]
      uint8_t FUN_SEL_FUN_GR_3; // [387:384]
   }b15;

   union SW_STATUS_V0_b16
   { 
      uint8_t FUN_SEL_FUN_GR_2; // [383:380]
      uint8_t FUN_SEL_FUN_GR_1; // [379:376]
   }b16;

   uint8_t DAT_STUCT_VER; // [375:368]

   uint8_t Reserved_1[46]; // [271:0]

}SW_STATUS_V0;

typedef struct SW_STATUS_V1
{
   uint16_t MAX_CURR_PWR_CONS; // [511:496]
   uint16_t SPRT_BIT_FUN_GR_6; // [495:480]
   uint16_t SPRT_BIT_FUN_GR_5; // [479:464]
   uint16_t SPRT_BIT_FUN_GR_4; // [463:448]
   uint16_t SPRT_BIT_FUN_GR_3; // [447:432]
   uint16_t SPRT_BIT_FUN_GR_2; // [431:416]
   uint16_t SPRT_BIT_FUN_GR_1; // [415:400]

   union SW_STATUS_V1_b14
   {
      uint8_t FUN_SEL_FUN_GR_6; // [399:396]
      uint8_t FUN_SEL_FUN_GR_5; // [395:392]
   }b14;

   union SW_STATUS_V1_b15
   {
      uint8_t FUN_SEL_FUN_GR_4; // [391:388]
      uint8_t FUN_SEL_FUN_GR_3; // [387:384]
   }b15;

   union SW_STATUS_V1_b16
   { 
      uint8_t FUN_SEL_FUN_GR_2; // [383:380]
      uint8_t FUN_SEL_FUN_GR_1; // [379:376]
   }b16;

   uint8_t DAT_STUCT_VER; // [375:368]

   uint16_t BUSY_ST_FUN_GR_6; // [367:352]
   uint16_t BUSY_ST_FUN_GR_5; // [351:336]
   uint16_t BUSY_ST_FUN_GR_4; // [335:320]
   uint16_t BUSY_ST_FUN_GR_3; // [319:304]
   uint16_t BUSY_ST_FUN_GR_2; // [303:288]
   uint16_t BUSY_ST_FUN_GR_1; // [287:272]

   uint8_t Reserved_1[34]; // [271:0]

}SW_STATUS_V1;

#define SW_STATUS_V1_MAX_CURR_PWR_CONS_GET(f) (INV_16(f))
#define SW_STATUS_V1_MAX_CURR_PWR_CONS_SET(f, value) (f) | (INV_16(value))

#define SW_STATUS_V1_SPRT_BIT_FUN_GR_6_GET(f) (INV_16(f))
#define SW_STATUS_V1_SPRT_BIT_FUN_GR_6_SET(f, value) (f) | (INV_16(value))

#define SW_STATUS_V1_SPRT_BIT_FUN_GR_5_GET(f) (INV_16(f))
#define SW_STATUS_V1_SPRT_BIT_FUN_GR_5_SET(f, value) (f) | (INV_16(value))

#define SW_STATUS_V1_SPRT_BIT_FUN_GR_4_GET(f) (INV_16(f))
#define SW_STATUS_V1_SPRT_BIT_FUN_GR_4_SET(f, value) (f) | (INV_16(value))

#define SW_STATUS_V1_SPRT_BIT_FUN_GR_3_GET(f) (INV_16(f))
#define SW_STATUS_V1_SPRT_BIT_FUN_GR_3_SET(f, value) (f) | (INV_16(value))

#define SW_STATUS_V1_SPRT_BIT_FUN_GR_2_GET(f) (INV_16(f))
#define SW_STATUS_V1_SPRT_BIT_FUN_GR_2_SET(f, value) (f) | (INV_16(value))

#define SW_STATUS_V1_SPRT_BIT_FUN_GR_1_GET(f) (INV_16(f))
#define SW_STATUS_V1_SPRT_BIT_FUN_GR_1_SET(f, value) (f) | (INV_16(value))

//

#define SW_STATUS_V1_FUN_SEL_FUN_GR_6_MASK 0xF0
#define SW_STATUS_V1_FUN_SEL_FUN_GR_6_GET(b14) (((b14) & SW_STATUS_V1_FUN_SEL_FUN_GR_6_MASK) >> 4)
#define SW_STATUS_V1_FUN_SEL_FUN_GR_6_SET(b14, value) ((b14) | (((value) & 0x0F) << 4))

#define SW_STATUS_V1_FUN_SEL_FUN_GR_5_MASK 0x0F
#define SW_STATUS_V1_FUN_SEL_FUN_GR_5_GET(b14) ((b14) & SW_STATUS_V1_FUN_SEL_FUN_GR_5_MASK)
#define SW_STATUS_V1_FUN_SEL_FUN_GR_5_SET(b14, value) (b14) | (((value) & SW_STATUS_V1_FUN_SEL_FUN_GR_5_MASK))

//

#define SW_STATUS_V1_FUN_SEL_FUN_GR_4_MASK 0xF0
#define SW_STATUS_V1_FUN_SEL_FUN_GR_4_GET(b15) (((b15) & SW_STATUS_V1_FUN_SEL_FUN_GR_4_MASK) >> 4)
#define SW_STATUS_V1_FUN_SEL_FUN_GR_4_SET(b15, value) ((b15) | (((value) & 0x0F) << 4))

#define SW_STATUS_V1_FUN_SEL_FUN_GR_3_MASK 0x0F
#define SW_STATUS_V1_FUN_SEL_FUN_GR_3_GET(b15) & SW_STATUS_V1_FUN_SEL_FUN_GR_3_MASK)
#define SW_STATUS_V1_FUN_SEL_FUN_GR_3_SET(b15, value) (b15) | (((value) & SW_STATUS_V1_FUN_SEL_FUN_GR_3_MASK))

//

#define SW_STATUS_V1_FUN_SEL_FUN_GR_2_MASK 0xF0
#define SW_STATUS_V1_FUN_SEL_FUN_GR_2_GET(b16) (((b16) & SW_STATUS_V1_FUN_SEL_FUN_GR_2_MASK) >> 4)
#define SW_STATUS_V1_FUN_SEL_FUN_GR_2_SET(b16, value) ((b16) | (((value) & 0x0F) << 4))

#define SW_STATUS_V1_FUN_SEL_FUN_GR_1_MASK 0x0F
#define SW_STATUS_V1_FUN_SEL_FUN_GR_1_GET(b16) & SW_STATUS_V1_FUN_SEL_FUN_GR_1_MASK)
#define SW_STATUS_V1_FUN_SEL_FUN_GR_1_SET(b16, value) (b16) | (((value) & SW_STATUS_V1_FUN_SEL_FUN_GR_1_MASK))

//

#define SW_STATUS_V1_DAT_STUCT_VER_GET(f) (f)
#define SW_STATUS_V1_DAT_STUCT_VER_SET(f, value) ((f) | (value))

#pragma pack(pop)
