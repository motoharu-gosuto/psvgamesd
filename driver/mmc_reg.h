/* mmc_reg.h
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

typedef struct MMC_CID
{
   uint8_t MID;    //[127:120]
   //Reserved        [119:114]
   uint8_t CBX;    //[113:112] - MMC_CBX type
   uint8_t OID;    //[111:104]
   uint8_t PNM[6]; //[103:56]
   uint8_t PRV;    //[55:48] - "n.m" two Binary Coded Decimal (BCD) digits 4 bits each
   uint32_t PSN;   //[47:16]
   uint8_t MDT;    //[15:8] - m/y to hex digits 4 bits each. year starting at 1997
   uint8_t CRC7;   //[7:0]
}MMC_CID;

#define MMC_CID_CBX_MASK 3

#define MMC_CID_GET_CBX(cbx) ((cbx) & MMC_CID_CBX_MASK)
#define MMC_CID_SET_CBX(cbx, value) (cbx | ((value) & MMC_CID_CBX_MASK))

#define MMC_CID_CBX_CARD 0
#define MMC_CID_CBX_BGA 1
#define MMC_CID_CBX_POP 2

#define MMC_CID_PNM_SET(pnm, value) strncpy((char*)pnm, value, 6)

#define MMC_CID_PRV_N_GET(prv) (((prv) & 0xF0) >> 4)
#define MMC_CID_PRV_M_GET(prv) ((prv) & 0x0F)
#define MMC_CID_PRV_NM_SET(n, m) ((((n) & 0x0F) << 4) | ((m) & (0x0F)))

#define MMC_CID_PSN_GET(psn) (INV_32(psn))
#define MMC_CID_PSN_SET(psn, value) (INV_32(value))

#define MMC_CID_MDT_JAN 1
#define MMC_CID_MDT_FEB 2
#define MMC_CID_MDT_MAR 3
#define MMC_CID_MDT_APR 4
#define MMC_CID_MDT_MAY 5
#define MMC_CID_MDT_JUN 6
#define MMC_CID_MDT_JUL 7
#define MMC_CID_MDT_AUG 8
#define MMC_CID_MDT_SEP 9
#define MMC_CID_MDT_OCT 10
#define MMC_CID_MDT_NOV 11
#define MMC_CID_MDT_DEC 12

#define MMC_CID_MDT_MIN_YEAR 1997
#define MMC_CID_MDT_MAX_YEAR 2012

#define MMC_CID_MDT_M_GET(mdt) (((mdt) & 0xF0) >> 4)
#define MMC_CID_MDT_Y_GET(mdt) (((mdt) & 0x0F) + MMC_CID_MDT_MIN_YEAR)
#define MMC_CID_MDT_M_Y_SET(m, y) ((((m) & 0x0F) << 4) | (((y) - MMC_CID_MDT_MIN_YEAR) & 0x0F))

//========== CSD ==========

//there are no differences in MMC_CSD between versions of MMC standard (like there is in SD standard)
//this is because all changes happen in EXT_CSD register that is only present in MMC standard

typedef struct MMC_CSD
{
   union MMC_CSD_b0
   {
      uint8_t CSD_STRUCTURE; // [127:126]
      uint8_t SPEC_VERS;     // [125:122]
      //Reserved             // [121:120]
   }b0;

   uint8_t TAAC;       // [119:112]
   uint8_t NSAC;       // [111:104]
   uint8_t TRAN_SPEED; // [103:96]

   union MMC_CSD_w4
   {
      uint16_t CCC;         // [95:84]
      uint16_t READ_BL_LEN; // [83:80]
   } w4;

   union MMC_CSD_qw6
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

      uint64_t ERASE_GRP_SIZE;     // [46:42]
      uint64_t ERASE_GRP_MULT;     // [41:37]
      uint64_t WP_GRP_SIZE;        // [36:32]

      uint64_t WP_GRP_ENABLE;      // [31:31]

      uint64_t DEFAULT_ECC;        // [30:29]

      uint64_t R2W_FACTOR;         // [28:26]
      uint64_t WRITE_BL_LEN;       // [25:22]
      uint64_t WRITE_BL_PARTIAL;   // [21:21]

      //Reserved                      [20:17]
      uint64_t CONTENT_PROT_APP;   // [16:16]
   }qw6;

   union MMC_CSD_b14
   {
      uint8_t FILE_FORMAT_GRP;    // [15:15]
      uint8_t COPY;               // [14:14]
      uint8_t PERM_WRITE_PROTECT; // [13:13]
      uint8_t TMP_WRITE_PROTECT;  // [12:12]
      uint8_t FILE_FORMAT;        // [11:10]
      uint8_t ECC;                // [9:8]
   }b14;

   uint8_t CRC7; // [7:0]

}MMC_CSD;

//STRUCTURE

#define MMC_CSD_STRUCTURE_MASK 0xC0
#define MMC_CSD_STRUCTURE_GET(b0_structure) (((b0_structure) & MMC_CSD_STRUCTURE_MASK) >> 6)
#define MMC_CSD_STRUCTURE_SET(b0, b0_structure) ((b0) | (((b0_structure) & 3) << 6))

#define MMC_CSD_STRUCTURE_VERSION_1_0 0
#define MMC_CSD_STRUCTURE_VERSION_1_1 1
#define MMC_CSD_STRUCTURE_VERSION_1_2 2
#define MMC_CSD_STRUCTURE_VERSION_EXT_CSD 3

//SPEC_VERS

#define MMC_CSD_SPEC_VERS_MASK 0x3C
#define MMC_CSD_SPEC_VERS_GET(b0_spec_vers) (((b0_spec_vers) & MMC_CSD_SPEC_VERS_MASK) >> 2)
#define MMC_CSD_SPEC_VERS_SET(b0, b0_spec_vers) ((b0) | (((b0_spec_vers) & 0xF) << 2))

#define MMC_CSD_SPEC_VERSION_4_X 4

//TAAC

#define MMC_CSD_TAAC_TU_MASK 7
#define MMC_CSD_TAAC_TU_GET(taac) ((taac) & MMC_CSD_TAAC_TU_MASK)
#define MMC_CSD_TAAC_TU_SET(taac, tu) ((taac) | ((tu) & 7))

#define MMC_CSD_TAAC_MF_MASK 0x78
#define MMC_CSD_TAAC_MF_GET(taac) (((taac) & MMC_CSD_TAAC_MF_MASK) >> 3)
#define MMC_CSD_TAAC_MF_SET(taac, mf) ((taac) | (((mf) & 0xF) << 3))

//time unit
#define MMC_CSD_TAAC_TU_1NS   0
#define MMC_CSD_TAAC_TU_10NS  1
#define MMC_CSD_TAAC_TU_100NS 2
#define MMC_CSD_TAAC_TU_1US   3
#define MMC_CSD_TAAC_TU_10US  4
#define MMC_CSD_TAAC_TU_100US 5
#define MMC_CSD_TAAC_TU_1MS   6
#define MMC_CSD_TAAC_TU_10MS  7

//multiplier factor
#define MMC_CSD_TAAC_MF_1_0 1
#define MMC_CSD_TAAC_MF_1_2 2
#define MMC_CSD_TAAC_MF_1_3 3
#define MMC_CSD_TAAC_MF_1_5 4
#define MMC_CSD_TAAC_MF_2_0 5
#define MMC_CSD_TAAC_MF_2_5 6
#define MMC_CSD_TAAC_MF_3_0 7
#define MMC_CSD_TAAC_MF_3_5 8
#define MMC_CSD_TAAC_MF_4_0 9
#define MMC_CSD_TAAC_MF_4_5 0xA
#define MMC_CSD_TAAC_MF_5_0 0xB
#define MMC_CSD_TAAC_MF_5_5 0xC
#define MMC_CSD_TAAC_MF_6_0 0xD
#define MMC_CSD_TAAC_MF_7_0 0xE
#define MMC_CSD_TAAC_MF_8_0 0xF

//TRAN_SPEED

#define MMC_CSD_TRAN_SPEED_FU_MASK 7
#define MMC_CSD_TRAN_SPEED_FU_GET(tran_speed) ((tran_speed) & MMC_CSD_TRAN_SPEED_FU_MASK)
#define MMC_CSD_TRAN_SPEED_FU_SET(tran_speed, fu) ((tran_speed) | ((fu) & 7))

#define MMC_CSD_TRAN_SPEED_MF_MASK 0x78
#define MMC_CSD_TRAN_SPEED_MF_GET(tran_speed) (((tran_speed) & MMC_CSD_TRAN_SPEED_MF_MASK) >> 3)
#define MMC_CSD_TRAN_SPEED_MF_SET(tran_speed, mf) ((tran_speed) | (((mf) & 0xF) << 3))

//frequency unit
#define MMC_CSD_TRAN_SPEED_FU_100KHZ 0
#define MMC_CSD_TRAN_SPEED_FU_1MHZ 1
#define MMC_CSD_TRAN_SPEED_FU_10MHZ 2
#define MMC_CSD_TRAN_SPEED_FU_100MHZ 3

//multiplier factor
#define MMC_CSD_TRAN_SPEED_MF_1_0 1
#define MMC_CSD_TRAN_SPEED_MF_1_2 2
#define MMC_CSD_TRAN_SPEED_MF_1_3 3
#define MMC_CSD_TRAN_SPEED_MF_1_5 4
#define MMC_CSD_TRAN_SPEED_MF_2_0 5
#define MMC_CSD_TRAN_SPEED_MF_2_6 6
#define MMC_CSD_TRAN_SPEED_MF_3_0 7
#define MMC_CSD_TRAN_SPEED_MF_3_5 8
#define MMC_CSD_TRAN_SPEED_MF_4_0 9
#define MMC_CSD_TRAN_SPEED_MF_4_5 0xA
#define MMC_CSD_TRAN_SPEED_MF_5_2 0xB
#define MMC_CSD_TRAN_SPEED_MF_5_5 0xC
#define MMC_CSD_TRAN_SPEED_MF_6_0 0xD
#define MMC_CSD_TRAN_SPEED_MF_7_0 0xE
#define MMC_CSD_TRAN_SPEED_MF_8_0 0xF

//CCC

#define MMC_CSD_CCC_MASK 0xFFF0
#define MMC_CSD_CCC_GET_INTERNAL(w4_ccc) (((w4_ccc) & MMC_CSD_CCC_MASK) >> 4)
#define MMC_CSD_CCC_GET(w4_ccc) (MMC_CSD_CCC_GET_INTERNAL(INV_16(w4_ccc)))
#define MMC_CSD_CCC_SET(w4_ccc, ccc) ((w4_ccc) | (INV_16((((ccc) & 0xFFF) << 4))))

#define MMC_CSD_CCC_CLASS_0  0x001
#define MMC_CSD_CCC_CLASS_1  0x002
#define MMC_CSD_CCC_CLASS_2  0x004
#define MMC_CSD_CCC_CLASS_3  0x008
#define MMC_CSD_CCC_CLASS_4  0x010
#define MMC_CSD_CCC_CLASS_5  0x020
#define MMC_CSD_CCC_CLASS_6  0x040
#define MMC_CSD_CCC_CLASS_7  0x080
#define MMC_CSD_CCC_CLASS_8  0x100
#define MMC_CSD_CCC_CLASS_9  0x200
#define MMC_CSD_CCC_CLASS_10 0x400
#define MMC_CSD_CCC_CLASS_11 0x800

#define MMC_CSD_CCC_CLASS_GET_INTERNAL(w4_ccc, cl) ((((w4_ccc) & MMC_CSD_CCC_MASK) >> 4) & (cl))
#define MMC_CSD_CCC_CLASS_GET(w4_ccc, cl) (MMC_CSD_CCC_CLASS_GET_INTERNAL(INV_16(w4_ccc), cl))
#define MMC_CSD_CCC_CLASS_SET(w4_ccc, cl) ((w4_ccc) | (INV_16((((cl) & 0xFFF) << 4))))

//READ_BL_LEN

#define MMC_CSD_READ_BL_LEN_MASK 0xF
#define MMC_CSD_READ_BL_LEN_GET_INTERNAL(w4_rbl) ((w4_rbl) & MMC_CSD_READ_BL_LEN_MASK)
#define MMC_CSD_READ_BL_LEN_GET(w4_rbl) (MMC_CSD_READ_BL_LEN_GET_INTERNAL(INV_16(w4_rbl)))
#define MMC_CSD_READ_BL_LEN_SET(w4_rbl, rbl) ((w4_rbl) | (INV_16(((rbl) & MMC_CSD_READ_BL_LEN_MASK))))

#define MMC_CSD_READ_BL_LEN_1B      0
#define MMC_CSD_READ_BL_LEN_2B      1
#define MMC_CSD_READ_BL_LEN_4B      2
#define MMC_CSD_READ_BL_LEN_8B      3
#define MMC_CSD_READ_BL_LEN_16B     4
#define MMC_CSD_READ_BL_LEN_32B     5
#define MMC_CSD_READ_BL_LEN_64B     6
#define MMC_CSD_READ_BL_LEN_128B    7
#define MMC_CSD_READ_BL_LEN_256B    8
#define MMC_CSD_READ_BL_LEN_512B    9
#define MMC_CSD_READ_BL_LEN_1KB     10
#define MMC_CSD_READ_BL_LEN_2KB     11
#define MMC_CSD_READ_BL_LEN_4KB     12
#define MMC_CSD_READ_BL_LEN_8KB     13
#define MMC_CSD_READ_BL_LEN_16KB    14
#define MMC_CSD_READ_BL_LEN_EXT_CSD 15

//_qw6

#define MMC_CSD_READ_BL_PARTIAL_MASK    0x8000000000000000LL

#define MMC_CSD_READ_BL_PARTIAL_GET(qw6) ((INV_64(qw6)) & MMC_CSD_READ_BL_PARTIAL_MASK)
#define MMC_CSD_READ_BL_PARTIAL_SET(qw6) ((qw6) | (INV_64(MMC_CSD_READ_BL_PARTIAL_MASK)))

//

#define MMC_CSD_WRITE_BLK_MISALIGN_MASK 0x4000000000000000LL

#define MMC_CSD_WRITE_BLK_MISALIGN_GET(qw6) ((INV_64(qw6)) & MMC_CSD_WRITE_BLK_MISALIGN_MASK)
#define MMC_CSD_WRITE_BLK_MISALIGN_SET(qw6) ((qw6) | (INV_64(MMC_CSD_WRITE_BLK_MISALIGN_MASK)))

//

#define MMC_CSD_READ_BLK_MISALIGN_MASK  0x2000000000000000LL

#define MMC_CSD_READ_BLK_MISALIGN_GET(qw6) ((INV_64(qw6)) & MMC_CSD_READ_BLK_MISALIGN_MASK)
#define MMC_CSD_READ_BLK_MISALIGN_SET(qw6) ((qw6) | (INV_64(MMC_CSD_READ_BLK_MISALIGN_MASK)))

//

#define MMC_CSD_DSR_IMP_MASK            0x1000000000000000LL

#define MMC_CSD_DSR_IMP_GET(qw6) ((INV_64(qw6)) & MMC_CSD_DSR_IMP_MASK)
#define MMC_CSD_DSR_IMP_SET(qw6) ((qw6) | (INV_64(MMC_CSD_DSR_IMP_MASK)))

//

#define MMC_CSD_C_SIZE_MASK             0x03FFC00000000000LL

#define MMC_CSD_C_SIZE_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_C_SIZE_MASK) >> 46)
#define MMC_CSD_C_SIZE_GET(qw6) (MMC_CSD_C_SIZE_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_C_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0xFFF) << 46)))

//

#define MMC_CSD_VDD_R_CURR_MIN_MASK     0x0000380000000000LL

#define MMC_CSD_VDD_R_CURR_MIN_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_VDD_R_CURR_MIN_MASK) >> 43)
#define MMC_CSD_VDD_R_CURR_MIN_GET(qw6) (MMC_CSD_VDD_R_CURR_MIN_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_VDD_R_CURR_MIN_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 43)))

#define MMC_CSD_VDD_R_CURR_MIN_0_5MA 0
#define MMC_CSD_VDD_R_CURR_MIN_1MA   1
#define MMC_CSD_VDD_R_CURR_MIN_5MA   2
#define MMC_CSD_VDD_R_CURR_MIN_10MA  3
#define MMC_CSD_VDD_R_CURR_MIN_25MA  4
#define MMC_CSD_VDD_R_CURR_MIN_35MA  5
#define MMC_CSD_VDD_R_CURR_MIN_60MA  6
#define MMC_CSD_VDD_R_CURR_MIN_100MA 7

//

#define MMC_CSD_VDD_R_CURR_MAX_MASK     0x0000070000000000LL

#define MMC_CSD_VDD_R_CURR_MAX_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_VDD_R_CURR_MAX_MASK) >> 40)
#define MMC_CSD_VDD_R_CURR_MAX_GET(qw6) (MMC_CSD_VDD_R_CURR_MAX_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_VDD_R_CURR_MAX_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 40)))

#define MMC_CSD_VDD_R_CURR_MAX_1MA   0
#define MMC_CSD_VDD_R_CURR_MAX_5MA   1
#define MMC_CSD_VDD_R_CURR_MAX_10MA  2
#define MMC_CSD_VDD_R_CURR_MAX_25MA  3
#define MMC_CSD_VDD_R_CURR_MAX_35MA  4
#define MMC_CSD_VDD_R_CURR_MAX_45MA  5
#define MMC_CSD_VDD_R_CURR_MAX_80MA  6
#define MMC_CSD_VDD_R_CURR_MAX_200MA 7

//

#define MMC_CSD_VDD_W_CURR_MIN_MASK     0x000000E000000000LL

#define MMC_CSD_VDD_W_CURR_MIN_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_VDD_W_CURR_MIN_MASK) >> 37)
#define MMC_CSD_VDD_W_CURR_MIN_GET(qw6) (MMC_CSD_VDD_W_CURR_MIN_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_VDD_W_CURR_MIN_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 37)))

#define MMC_CSD_VDD_W_CURR_MIN_0_5MA 0
#define MMC_CSD_VDD_W_CURR_MIN_1MA   1
#define MMC_CSD_VDD_W_CURR_MIN_5MA   2
#define MMC_CSD_VDD_W_CURR_MIN_10MA  3
#define MMC_CSD_VDD_W_CURR_MIN_25MA  4
#define MMC_CSD_VDD_W_CURR_MIN_35MA  5
#define MMC_CSD_VDD_W_CURR_MIN_60MA  6
#define MMC_CSD_VDD_W_CURR_MIN_100MA 7

//

#define MMC_CSD_VDD_W_CURR_MAX_MASK     0x0000001C00000000LL

#define MMC_CSD_VDD_W_CURR_MAX_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_VDD_W_CURR_MAX_MASK) >> 34)
#define MMC_CSD_VDD_W_CURR_MAX_GET(qw6) (MMC_CSD_VDD_W_CURR_MAX_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_VDD_W_CURR_MAX_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 34)))

#define MMC_CSD_VDD_W_CURR_MAX_1MA   0
#define MMC_CSD_VDD_W_CURR_MAX_5MA   1
#define MMC_CSD_VDD_W_CURR_MAX_10MA  2
#define MMC_CSD_VDD_W_CURR_MAX_25MA  3
#define MMC_CSD_VDD_W_CURR_MAX_35MA  4
#define MMC_CSD_VDD_W_CURR_MAX_45MA  5
#define MMC_CSD_VDD_W_CURR_MAX_80MA  6
#define MMC_CSD_VDD_W_CURR_MAX_200MA 7

//

#define MMC_CSD_C_SIZE_MULT_MASK        0x0000000380000000LL

#define MMC_CSD_C_SIZE_MULT_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_C_SIZE_MULT_MASK) >> 31)
#define MMC_CSD_C_SIZE_MULT_GET(qw6) (MMC_CSD_C_SIZE_MULT_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_C_SIZE_MULT_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 31)))

#define MMC_CSD_C_SIZE_MULT_4   0
#define MMC_CSD_C_SIZE_MULT_8   1
#define MMC_CSD_C_SIZE_MULT_16  2
#define MMC_CSD_C_SIZE_MULT_32  3
#define MMC_CSD_C_SIZE_MULT_64  4
#define MMC_CSD_C_SIZE_MULT_128 5
#define MMC_CSD_C_SIZE_MULT_256 6
#define MMC_CSD_C_SIZE_MULT_512 7

//

#define MMC_CSD_ERASE_GRP_SIZE_MASK     0x000000007C000000LL

#define MMC_CSD_ERASE_GRP_SIZE_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_ERASE_GRP_SIZE_MASK) >> 26)
#define MMC_CSD_ERASE_GRP_SIZE_GET(qw6) (MMC_CSD_ERASE_GRP_SIZE_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_ERASE_GRP_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x1F) << 26)))

#define MMC_CSD_ERASE_GRP_MULT_MASK     0x0000000003E00000LL

#define MMC_CSD_ERASE_GRP_MULT_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_ERASE_GRP_MULT_MASK) >> 21)
#define MMC_CSD_ERASE_GRP_MULT_GET(qw6) (MMC_CSD_ERASE_GRP_MULT_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_ERASE_GRP_MULT_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x1F) << 21)))

#define MMC_CSD_WP_GRP_SIZE_MASK        0x00000000001F0000LL

#define MMC_CSD_WP_GRP_SIZE_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_WP_GRP_SIZE_MASK) >> 16)
#define MMC_CSD_WP_GRP_SIZE_GET(qw6) (MMC_CSD_WP_GRP_SIZE_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_WP_GRP_SIZE_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x1F) << 16)))

//

#define MMC_CSD_WP_GRP_ENABLE_MASK      0x0000000000008000LL

#define MMC_CSD_WP_GRP_ENABLE_GET(qw6) ((INV_64(qw6)) & MMC_CSD_WP_GRP_ENABLE_MASK)
#define MMC_CSD_WP_GRP_ENABLE_SET(qw6) ((qw6) | (INV_64(MMC_CSD_WP_GRP_ENABLE_MASK)))

//

#define MMC_CSD_DEFAULT_ECC_MASK        0x0000000000006000LL

#define MMC_CSD_DEFAULT_ECC_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_DEFAULT_ECC_MASK) >> 13)
#define MMC_CSD_DEFAULT_ECC_GET(qw6) (MMC_CSD_DEFAULT_ECC_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_DEFAULT_ECC_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x3) << 13)))

//

#define MMC_CSD_R2W_FACTOR_MASK         0x0000000000001C00LL

#define MMC_CSD_R2W_FACTOR_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_R2W_FACTOR_MASK) >> 10)
#define MMC_CSD_R2W_FACTOR_GET(qw6) (MMC_CSD_R2W_FACTOR_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_R2W_FACTOR_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0x7) << 10)))

#define MMC_CSD_R2W_FACTOR_1   0
#define MMC_CSD_R2W_FACTOR_2   1
#define MMC_CSD_R2W_FACTOR_4   2
#define MMC_CSD_R2W_FACTOR_8   3
#define MMC_CSD_R2W_FACTOR_16  4
#define MMC_CSD_R2W_FACTOR_32  5
#define MMC_CSD_R2W_FACTOR_64  6
#define MMC_CSD_R2W_FACTOR_128 7

//

#define MMC_CSD_WRITE_BL_LEN_MASK       0x00000000000003C0LL

#define MMC_CSD_WRITE_BL_LEN_GET_INTERNAL(qw6) (((qw6) & MMC_CSD_WRITE_BL_LEN_MASK) >> 6)
#define MMC_CSD_WRITE_BL_LEN_GET(qw6) (MMC_CSD_WRITE_BL_LEN_GET_INTERNAL(INV_64(qw6)))
#define MMC_CSD_WRITE_BL_LEN_SET(qw6, value) ((qw6) | INV_64(((((uint64_t)value) & 0xF) << 6)))

#define MMC_CSD_WRITE_BL_LEN_1B      0
#define MMC_CSD_WRITE_BL_LEN_2B      1
#define MMC_CSD_WRITE_BL_LEN_4B      2
#define MMC_CSD_WRITE_BL_LEN_8B      3
#define MMC_CSD_WRITE_BL_LEN_16B     4
#define MMC_CSD_WRITE_BL_LEN_32B     5
#define MMC_CSD_WRITE_BL_LEN_64B     6
#define MMC_CSD_WRITE_BL_LEN_128B    7
#define MMC_CSD_WRITE_BL_LEN_256B    8
#define MMC_CSD_WRITE_BL_LEN_512B    9
#define MMC_CSD_WRITE_BL_LEN_1KB     10
#define MMC_CSD_WRITE_BL_LEN_2KB     11
#define MMC_CSD_WRITE_BL_LEN_4KB     12
#define MMC_CSD_WRITE_BL_LEN_8KB     13
#define MMC_CSD_WRITE_BL_LEN_16KB    14
#define MMC_CSD_WRITE_BL_LEN_EXT_CSD 15

//

#define MMC_CSD_WRITE_BL_PARTIAL_MASK   0x0000000000000020LL

#define MMC_CSD_WRITE_BL_PARTIAL_GET(qw6) ((INV_64(qw6)) & MMC_CSD_WRITE_BL_PARTIAL_MASK)
#define MMC_CSD_WRITE_BL_PARTIAL_SET(qw6) ((qw6) | (INV_64(MMC_CSD_WRITE_BL_PARTIAL_MASK)))

//

#define MMC_CSD_CONTENT_PROT_APP_MASK   0x0000000000000001LL

#define MMC_CSD_CONTENT_PROT_APP_GET(qw6) ((INV_64(qw6)) & MMC_CSD_CONTENT_PROT_APP_MASK)
#define MMC_CSD_CONTENT_PROT_APP_SET(qw6) ((qw6) | (INV_64(MMC_CSD_CONTENT_PROT_APP_MASK)))

//_b14

#define MMC_CSD_FILE_FORMAT_GRP_MASK    0x80
#define MMC_CSD_FILE_FORMAT_GRP_GET(b14) ((b14) & MMC_CSD_FILE_FORMAT_GRP_MASK)
#define MMC_CSD_FILE_FORMAT_GRP_SET(b14) ((b14) | (MMC_CSD_FILE_FORMAT_GRP_MASK))

//

#define MMC_CSD_COPY_MASK               0x40
#define MMC_CSD_COPY_GET(b14) ((b14) & MMC_CSD_COPY_MASK)
#define MMC_CSD_COPY_SET(b14) ((b14) | (MMC_CSD_COPY_MASK))

//

#define MMC_CSD_PERM_WRITE_PROTECT_MASK 0x20
#define MMC_CSD_PERM_WRITE_PROTECT_GET(b14) ((b14) & MMC_CSD_PERM_WRITE_PROTECT_MASK)
#define MMC_CSD_PERM_WRITE_PROTECT_SET(b14) ((b14) | (MMC_CSD_PERM_WRITE_PROTECT_MASK))

//

#define MMC_CSD_TMP_WRITE_PROTECT_MASK  0x10
#define MMC_CSD_TMP_WRITE_PROTECT_GET(b14) ((b14) & MMC_CSD_TMP_WRITE_PROTECT_MASK)
#define MMC_CSD_TMP_WRITE_PROTECT_SET(b14) ((b14) | (MMC_CSD_TMP_WRITE_PROTECT_MASK))

//

#define MMC_CSD_FILE_FORMAT_MASK        0x0C
#define MMC_CSD_FILE_FORMAT_GET(b14) (((b14) & MMC_CSD_FILE_FORMAT_MASK) >> 2)
#define MMC_CSD_FILE_FORMAT_SET(b14, value) ((b14) | (((value) & 0x3) << 2))

#define MMC_CSD_FILE_FORMAT_HARD_DISK 0
#define MMC_CSD_FILE_FORMAT_DOS_FAT 1
#define MMC_CSD_FILE_FORMAT_UFF 2
#define MMC_CSD_FILE_FORMAT_OTHERS 3

//

#define MMC_CSD_ECC_MASK                0x03
#define MMC_CSD_ECC_GET(b14) ((b14) & MMC_CSD_ECC_MASK)
#define MMC_CSD_ECC_SET(b14, value) ((b14) | ((value) & MMC_CSD_ECC_MASK))

#define MMC_CSD_ECC_NONE 0
#define MMC_CSD_ECC_NONE_BCH 1

//========== EXT_CSD ==========

//There are different versions of EXT_CSD register
//layout of the register changes with each new standard of MMC

typedef struct EXT_CSD_MMCA_4_0
{
   //TODO
}EXT_CSD_MMCA_4_0;

typedef struct EXT_CSD_MMCA_4_1
{
   //TODO
}EXT_CSD_MMCA_4_1;

typedef struct EXT_CSD_MMCA_4_2
{
   //Modes Segment

   uint8_t Reserved_1[181]; // [180:0]
   uint8_t ERASED_MEM_CONT; // [181]
   uint8_t Reserved_2; // [182]
   uint8_t BUS_WIDTH; // [183]
   uint8_t Reserved_3; // [184]
   uint8_t HS_TIMING; // [185]
   uint8_t Reserved_4; // [186]
   uint8_t POWER_CLASS; // [187]
   uint8_t Reserved_5; // [188]
   uint8_t CMD_SET_REV; // [189]
   uint8_t Reserved_6; // [190]
   uint8_t CMD_SET; // [191]

   //Properties Segment

   uint8_t EXT_CSD_REV; // [192]
   uint8_t Reserved_7; // [193]
   uint8_t CSD_STRUCTURE; // [194]
   uint8_t Reserved_8; // [195]
   uint8_t CARD_TYPE; // [196]
   uint8_t Reserved_9[3]; // [199:197]
   uint8_t PWR_CL_52_195; // [200]
   uint8_t PWR_CL_26_195; // [201]
   uint8_t PWR_CL_52_360; // [202]
   uint8_t PWR_CL_26_360; // [203]
   uint8_t Reserved_10; // [204]
   uint8_t MIN_PERF_R_4_26; // [205]
   uint8_t MIN_PERF_W_4_26; // [206]
   uint8_t MIN_PERF_R_8_26_4_52; // [207]
   uint8_t MIN_PERF_W_8_26_4_52; // [208]
   uint8_t MIN_PERF_R_8_52; // [209]
   uint8_t MIN_PERF_W_8_52; // [210]
   uint8_t Reserved_11; // [211]
   uint32_t SEC_COUNT; // [215:212]
   uint8_t Reserved_12[288]; // [503:216]
   uint8_t S_CMD_SET; // [504]
   uint8_t Reserved_13[7]; // [511:505]

}EXT_CSD_MMCA_4_2;

typedef struct EXT_CSD_MMCA_4_3
{
   //TODO
}EXT_CSD_MMCA_4_3;

typedef struct EXT_CSD_MMCA_4_4
{
   //Modes segment

   uint8_t Reserved_1[134]; //[133:0]
   uint8_t SEC_BAD_BLK_MGMNT; //[134]
   uint8_t Reserved_2[1]; //[135]
   uint32_t ENH_START_ADDR; //[139:136]
   uint8_t ENH_SIZE_MULT[3]; //[142:140]
   uint8_t GP_SIZE_MULT[12]; //[154:143]
   uint8_t PARTITION_SETTING_COMPLETED; //[155]
   uint8_t PARTITIONS_ATTRIBUTE; //[156]
   uint8_t MAX_ENH_SIZE_MULT[3]; //[159:157]
   uint8_t PARTITIONING_SUPPORT; //[160]
   uint8_t Reserved_3[1]; //[161]
   uint8_t RST_n_FUNCTION; //[162]
   uint8_t Reserved_4[5]; //[167:163]
   uint8_t RPMB_SIZE_MULT; //[168]
   uint8_t FW_CONFIG; //[169]
   uint8_t Reserved_5[1]; //[170]
   uint8_t USER_WP; //[171]
   uint8_t Reserved_6[1]; //[172]
   uint8_t BOOT_WP; //[173]
   uint8_t Reserved_7[1]; //[174]
   uint8_t ERASE_GROUP_DEF; //[175]
   uint8_t Reserved_8[1]; //[176]
   uint8_t BOOT_BUS_WIDTH; //[177]
   uint8_t BOOT_CONFIG_PROT; //[178]
   uint8_t PARTITION_CONFIG; //[179]
   uint8_t Reserved_9[1]; //[180]
   uint8_t ERASED_MEM_CONT; //[181]
   uint8_t Reserved_10[1]; //[182]
   uint8_t BUS_WIDTH; //[183]
   uint8_t Reserved_11[1]; //[184]
   uint8_t HS_TIMING; //[185]
   uint8_t Reserved_12[1]; //[186]
   uint8_t POWER_CLASS; //[187]
   uint8_t Reserved_13[1]; //[188]
   uint8_t CMD_SET_REV; //[189]
   uint8_t Reserved_14[1]; //[190]
   uint8_t CMD_SET; //[191]

   //Properties segment

   uint8_t EXT_CSD_REV; //[192]
   uint8_t Reserved_15[1]; //[193]
   uint8_t CSD_STRUCTURE; //[194]
   uint8_t Reserved_16[1]; //[195]
   uint8_t CARD_TYPE; //[196]
   uint8_t Reserved_17[3]; //[199:197]
   uint8_t PWR_CL_52_195; //[200]
   uint8_t PWR_CL_26_195; //[201]
   uint8_t PWR_CL_52_360; //[202]
   uint8_t PWR_CL_26_360; //[203]
   uint8_t Reserved_18[1]; //[204]
   uint8_t MIN_PERF_R_4_26; //[205]
   uint8_t MIN_PERF_W_4_26; //[206]
   uint8_t MIN_PERF_R_8_26_4_52; //[207]
   uint8_t MIN_PERF_W_8_26_4_52; //[208]
   uint8_t MIN_PERF_R_8_52; //[209]
   uint8_t MIN_PERF_W_8_52; //[210]
   uint8_t Reserved_19[1]; //[211]
   uint32_t SEC_COUNT; //[215:212]
   uint8_t Reserved_20[1]; //[216]
   uint8_t S_A_TIMEOUT; //[217]
   uint8_t Reserved_21[1]; //[218]
   uint8_t S_C_VCCQ; //[219]
   uint8_t S_C_VCC; //[220]
   uint8_t HC_WP_GRP_SIZE; //[221]
   uint8_t REL_WR_SEC_C; //[222]
   uint8_t ERASE_TIMEOUT_MULT; //[223]
   uint8_t HC_ERASE_GRP_SIZE; //[224]
   uint8_t ACC_SIZE; //[225]
   uint8_t BOOT_SIZE_MULTI; //[226]
   uint8_t Reserved_22[1]; //[227]
   uint8_t BOOT_INFO; //[228]
   uint8_t SEC_TRIM_MULT; //[229]
   uint8_t SEC_ERASE_MULT; //[230]
   uint8_t SEC_FEATURE_SUPPORT; //[231]
   uint8_t TRIM_MULT; //[232]
   uint8_t Reserved_23[1]; //[233]
   uint8_t MIN_PERF_DDR_R_8_52; //[234]
   uint8_t MIN_PERF_DDR_W_8_52; //[235]
   uint8_t Reserved_24[2]; //[237:236]
   uint8_t PWR_CL_DDR_52_195; //[238]
   uint8_t PWR_CL_DDR_52_360; //[239]
   uint8_t Reserved_25[1]; //[240]
   uint8_t INI_TIMEOUT_AP; //[241]
   uint8_t Reserved_26[262]; //[503:242]
   uint8_t S_CMD_SET; //[504]
   uint8_t Reserved_27[7]; //[511:505]

}EXT_CSD_MMCA_4_4;

typedef struct EXT_CSD_MMCA_4_5
{
   //TODO
}EXT_CSD_MMCA_4_5;

typedef struct EXT_CSD_MMCA_5_0
{
   //TODO
}EXT_CSD_MMCA_5_0;

typedef struct EXT_CSD_MMCA_5_1
{
   // Modes Segment

   uint8_t Reserved_1[15]; // [14:0]
   uint8_t CMDQ_MODE_EN; // [15]
   uint8_t SECURE_REMOVAL_TYPE; // [16]
   uint8_t PRODUCT_STATE_AWARENESS_ENABLEMENT; // [17]
   uint32_t MAX_PRE_LOADING_DATA_SIZE; // [21:18]
   uint32_t PRE_LOADING_DATA_SIZE; // [25:22]
   uint8_t FFU_STATUS; // [26]
   uint8_t Reserved_2[2]; // [28:27]
   uint8_t MODE_OPERATION_CODES; // [29]
   uint8_t MODE_CONFIG; // [30]
   uint8_t BARRIER_CTRL; // [31]
   uint8_t FLUSH_CACHE; // [32]
   uint8_t CACHE_CTRL; // [33]
   uint8_t POWER_OFF_NOTIFICATION; // [34]
   uint8_t PACKED_FAILURE_INDEX; // [35]
   uint8_t PACKED_COMMAND_STATUS; // [36]
   uint8_t CONTEXT_CONF[15]; // [51:37]
   uint8_t EXT_PARTITIONS_ATTRIBUTE[2]; // [53:52]
   uint8_t EXCEPTION_EVENTS_STATUS[2]; // [55:54]
   uint8_t EXCEPTION_EVENTS_CTRL[2]; // [57:56]
   uint8_t DYNCAP_NEEDED; // [58]
   uint8_t CLASS_6_CTRL; // [59]
   uint8_t INI_TIMEOUT_EMU; // [60]
   uint8_t DATA_SECTOR_SIZE; // [61]
   uint8_t USE_NATIVE_SECTOR; // [62]
   uint8_t NATIVE_SECTOR_SIZE; // [63]
   uint8_t VENDOR_SPECIFIC_FIELD[64]; // [127:64]
   uint8_t Reserved_3[2]; // [129:128]
   uint8_t PROGRAM_CID_CSD_DDR_SUPPORT; // [130]
   uint8_t PERIODIC_WAKEUP; // [131]
   uint8_t TCASE_SUPPORT; // [132]
   uint8_t PRODUCTION_STATE_AWARENESS; // [133]
   uint8_t SEC_BAD_BLK_MGMNT; // [134]
   uint8_t Reserved_4; // [135]
   uint32_t ENH_START_ADDR; // [139:136]
   uint8_t ENH_SIZE_MULT[3]; // [142:140]
   uint8_t GP_SIZE_MULT[12]; // [154:143]
   uint8_t PARTITION_SETTING_COMPLETED; // [155]
   uint8_t PARTITIONS_ATTRIBUTE; // [156]
   uint8_t MAX_ENH_SIZE_MULT[3]; // [159:157]
   uint8_t PARTITIONING_SUPPORT; // [160]
   uint8_t HPI_MGMT; // [161]
   uint8_t RST_n_FUNCTION; // [162]
   uint8_t BKOPS_EN; // [163]
   uint8_t BKOPS_START; // [164]
   uint8_t SANITIZE_START; // [165]
   uint8_t WR_REL_PARAM; // [166]
   uint8_t WR_REL_SET; // [167]
   uint8_t RPMB_SIZE_MULT; // [168]
   uint8_t FW_CONFIG; // [169]
   uint8_t Reserved_5; // [170]
   uint8_t USER_WP; // [171]
   uint8_t Reserved_6; // [172]
   uint8_t BOOT_WP; // [173]
   uint8_t BOOT_WP_STATUS; // [174]
   uint8_t ERASE_GROUP_DEF; // [175]
   uint8_t Reserved_7; // [176]
   uint8_t BOOT_BUS_CONDITIONS; // [177]
   uint8_t BOOT_CONFIG_PROT; // [178]
   uint8_t PARTITION_CONFIG; // [179]
   uint8_t Reserved_8; // [180]
   uint8_t ERASED_MEM_CONT; // [181]
   uint8_t Reserved_9; // [182]
   uint8_t BUS_WIDTH; // [183]
   uint8_t STROBE_SUPPORT; // [184]
   uint8_t HS_TIMING; // [185]
   uint8_t Reserved_10; // [186]
   uint8_t POWER_CLASS; // [187]
   uint8_t Reserved_11; // [188]
   uint8_t CMD_SET_REV; // [189]
   uint8_t Reserved_12; // [190]
   uint8_t CMD_SET; // [191]

   //Properties segment

   uint8_t EXT_CSD_REV; // [192]
   uint8_t Reserved_13; // [193]
   uint8_t CSD_STRUCTURE; // [194]
   uint8_t Reserved_14; // [195]
   uint8_t DEVICE_TYPE; // [196]
   uint8_t DRIVER_STRENGTH; // [197]
   uint8_t OUT_OF_INTERRUPT_TIME; // [198]
   uint8_t PARTITION_SWITCH_TIME; // [199]
   uint8_t PWR_CL_52_195; // [200]
   uint8_t PWR_CL_26_195; // [201]
   uint8_t PWR_CL_52_360; // [202]
   uint8_t PWR_CL_26_360; // [203]
   uint8_t Reserved_15; // [204]
   uint8_t MIN_PERF_R_4_26; // [205]
   uint8_t MIN_PERF_W_4_26; // [206]
   uint8_t MIN_PERF_R_8_26_4_52; // [207]
   uint8_t MIN_PERF_W_8_26_4_52; // [208]
   uint8_t MIN_PERF_R_8_52; // [209]
   uint8_t MIN_PERF_W_8_52; // [210]
   uint8_t SECURE_WP_INFO; // [211]
   uint32_t SEC_COUNT; // [215:212]
   uint8_t SLEEP_NOTIFICATION_TIME; // [216]
   uint8_t S_A_TIMEOUT; // [217]
   uint8_t PRODUCTION_STATE_AWARENESS_TIMEOUT; // [218]
   uint8_t S_C_VCCQ; // [219]
   uint8_t S_C_VCC; // [220]
   uint8_t HC_WP_GRP_SIZE; // [221]
   uint8_t REL_WR_SEC_C; // [222]
   uint8_t ERASE_TIMEOUT_MULT; // [223]
   uint8_t HC_ERASE_GRP_SIZE; // [224]
   uint8_t ACC_SIZE; // [225]
   uint8_t BOOT_SIZE_MULT; // [226]
   uint8_t Reserved_16; // [227]
   uint8_t BOOT_INFO; // [228]
   uint8_t SEC_TRIM_MULT; // [229]
   uint8_t SEC_ERASE_MULT; // [230]
   uint8_t SEC_FEATURE_SUPPORT; // [231]
   uint8_t TRIM_MULT; // [232]
   uint8_t Reserved_17; // [233]
   uint8_t MIN_PERF_DDR_R_8_52; // [234]
   uint8_t MIN_PERF_DDR_W_8_52; // [235]
   uint8_t PWR_CL_200_130; // [236]
   uint8_t PWR_CL_200_195; // [237]
   uint8_t PWR_CL_DDR_52_195; // [238]
   uint8_t PWR_CL_DDR_52_360; // [239]
   uint8_t CACHE_FLUSH_POLICY; // [240]
   uint8_t INI_TIMEOUT_AP; // [241]
   uint32_t CORRECTLY_PRG_SECTORS_NUM; // [245:242]
   uint8_t BKOPS_STATUS; // [246]
   uint8_t POWER_OFF_LONG_TIME; // [247]
   uint8_t GENERIC_CMD6_TIME; // [248]
   uint32_t CACHE_SIZE; // [252:249]
   uint8_t PWR_CL_DDR_200_360; // [253]
   uint8_t FIRMWARE_VERSION[8]; // [261:254]
   uint8_t DEVICE_VERSION[2]; // [263:262]
   uint8_t OPTIMAL_TRIM_UNIT_SIZE; // [264]
   uint8_t OPTIMAL_WRITE_SIZE; // [265]
   uint8_t OPTIMAL_READ_SIZE; // [266]
   uint8_t PRE_EOL_INFO; // [267]
   uint8_t DEVICE_LIFE_TIME_EST_TYP_A; // [268]
   uint8_t DEVICE_LIFE_TIME_EST_TYP_B; // [269]
   uint8_t VENDOR_PROPRIETARY_HEALTH_REPORT[32]; // [301:270]
   uint32_t NUMBER_OF_FW_SECTORS_CORRECTLY_PROGRAMMED; // [305:302]
   uint8_t Reserved_18; // [306]
   uint8_t CMDQ_DEPTH; // [307]
   uint8_t CMDQ_SUPPORT; // [308]
   uint8_t Reserved_19[177]; // [485:309]
   uint8_t BARRIER_SUPPORT; // [486]
   uint32_t FFU_ARG; // [490:487]
   uint8_t OPERATION_CODE_TIMEOUT; // [491]
   uint8_t FFU_FEATURES; // [492]
   uint8_t SUPPORTED_MODES; // [493]
   uint8_t EXT_SUPPORT; // [494]
   uint8_t LARGE_UNIT_SIZE_M1; // [495]
   uint8_t CONTEXT_CAPABILITIES; // [496]
   uint8_t TAG_RES_SIZE; // [497]
   uint8_t TAG_UNIT_SIZE; // [498]
   uint8_t DATA_TAG_SUPPORT; // [499]
   uint8_t MAX_PACKED_WRITES; // [500]
   uint8_t MAX_PACKED_READS; // [501]
   uint8_t BKOPS_SUPPORT; // [502]
   uint8_t HPI_FEATURES; // [503]
   uint8_t S_CMD_SET; // [504]
   uint8_t EXT_SECURITY_ERR; // [505]
   uint8_t Reserved_20[6]; // [511:506]

}EXT_CSD_MMCA_5_1;

#define MMC_EXT_CSD_CSD_STRUCTURE_1_0 0
#define MMC_EXT_CSD_CSD_STRUCTURE_1_1 1
#define MMC_EXT_CSD_CSD_STRUCTURE_1_2 2

//

#define MMC_EXT_CSD_EXT_CSD_REV_4_0 0
#define MMC_EXT_CSD_EXT_CSD_REV_4_1 1
#define MMC_EXT_CSD_EXT_CSD_REV_4_2 2
#define MMC_EXT_CSD_EXT_CSD_REV_4_3 3
#define MMC_EXT_CSD_EXT_CSD_REV_4_4 5
#define MMC_EXT_CSD_EXT_CSD_REV_4_5 6
#define MMC_EXT_CSD_EXT_CSD_REV_5_0 7
#define MMC_EXT_CSD_EXT_CSD_REV_5_1 8

//

#define MMC_EXT_CSD_CARD_TYPE_GET(type) (((type) & 0x3))
#define MMC_EXT_CSD_CARD_TYPE_SET(type, value) (((value) & 0x3) | type)

#define MMC_EXT_CSD_CARD_TYPE_26MHZ 1
#define MMC_EXT_CSD_CARD_TYPE_52MHZ 2

//

#define MMC_EXT_CSD_MIN_PERF_DEFAULT 0x00
#define MMC_EXT_CSD_MIN_PERF_A 0x08
#define MMC_EXT_CSD_MIN_PERF_B 0x0A
#define MMC_EXT_CSD_MIN_PERF_C 0x0F
#define MMC_EXT_CSD_MIN_PERF_D 0x14
#define MMC_EXT_CSD_MIN_PERF_E 0x1E
#define MMC_EXT_CSD_MIN_PERF_F 0x28
#define MMC_EXT_CSD_MIN_PERF_G 0x32
#define MMC_EXT_CSD_MIN_PERF_H 0x3C
#define MMC_EXT_CSD_MIN_PERF_J 0x46
#define MMC_EXT_CSD_MIN_PERF_K 0x50
#define MMC_EXT_CSD_MIN_PERF_M 0x64
#define MMC_EXT_CSD_MIN_PERF_O 0x78
#define MMC_EXT_CSD_MIN_PERF_R 0x8C
#define MMC_EXT_CSD_MIN_PERF_T 0xA0

//

#define MMC_EXT_CSD_S_CMD_SET_MMC_0 0
#define MMC_EXT_CSD_S_CMD_SET_MMCA_1 1
#define MMC_EXT_CSD_S_CMD_SET_MMCA_2 2
#define MMC_EXT_CSD_S_CMD_SET_MMCA_3 3
#define MMC_EXT_CSD_S_CMD_SET_MMCA_4 4

#pragma pack(pop)
