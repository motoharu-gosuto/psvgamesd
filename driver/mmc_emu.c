/* mmc_emu.c
 *
 * Copyright (C) 2017 Motoharu Gosuto
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "mmc_emu.h"

#include <psp2kern/kernel/threadmgr.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "global_log.h"
#include "reader.h"
#include "functions.h"
#include "defines.h"

#include "reg_common.h"
#include "mmc_reg.h"

#define IDLE_MMC_STATE 0
#define READY_MMC_STATE 1
#define IDENTIFICATION_MMC_STATE 2
#define STANDBY_MMC_STATE 3
#define TRANSFER_MMC_STATE 4
#define DATA_MMC_STATE 5
#define RECEIVE_MMC_STATE 6
#define PROGRAMING_MMC_STATE 7
#define DISCONNECT_MMC_STATE 8
#define BUS_TEST_MMC_STATE 9
#define SLEEP_MMC_STATE 10
#define INVALID_MMC_STATE -1

#define MMC_ILLEGAL_COMMAND_FLAG 0x00400000

#define MMC_COMMAND_COMPLETE_FLAG 0x80000000

#define MMC_INIT_COMPLETE 0x80000000
#define MMC_CCS_SDHC_SDXC 0x40000000

//-----------------------------------

#define MMC_EMU_MID 0x11
#define MMC_EMU_OID 0x11
#define MMC_EMU_PNM "PLTFAA"
#define MMC_EMU_PRV_N 1
#define MMC_EMU_PRV_M 0
#define MMC_EMU_PSN 0x00000000
#define MMC_EMU_MON MMC_CID_MDT_DEC
#define MMC_EMU_YEAR 2010

void get_mmc_cid(MMC_CID* cid)
{
   memset(cid, 0, sizeof(MMC_CID));

   cid->MID = MMC_EMU_MID;
   cid->CBX = MMC_CID_SET_CBX(cid->CBX, MMC_CID_CBX_CARD);
   cid->OID = MMC_EMU_OID;
   MMC_CID_PNM_SET(cid->PNM, MMC_EMU_PNM);
   cid->PRV = MMC_CID_PRV_NM_SET(MMC_EMU_PRV_N, MMC_EMU_PRV_M);
   cid->PSN = MMC_CID_PSN_SET(cid->PSN, MMC_EMU_PSN);
   cid->MDT = MMC_CID_MDT_M_Y_SET(MMC_EMU_MON, MMC_EMU_YEAR);
}

void get_mmc_csd(MMC_CSD* csd)
{
   memset(csd, 0, sizeof(MMC_CSD));

   csd->b0.CSD_STRUCTURE = MMC_CSD_STRUCTURE_SET(csd->b0.CSD_STRUCTURE, MMC_CSD_STRUCTURE_VERSION_1_2);
   csd->b0.SPEC_VERS = MMC_CSD_SPEC_VERS_SET(csd->b0.SPEC_VERS, MMC_CSD_SPEC_VERSION_4_X);

   csd->TAAC = MMC_CSD_TAAC_TU_SET(csd->TAAC, MMC_CSD_TAAC_TU_10MS);
   csd->TAAC = MMC_CSD_TAAC_MF_SET(csd->TAAC, MMC_CSD_TAAC_MF_8_0);

   csd->NSAC = 0x01; //100 clock cycles

   csd->TRAN_SPEED = MMC_CSD_TRAN_SPEED_FU_SET(csd->TRAN_SPEED, MMC_CSD_TRAN_SPEED_FU_10MHZ);
   csd->TRAN_SPEED = MMC_CSD_TRAN_SPEED_MF_SET(csd->TRAN_SPEED, MMC_CSD_TRAN_SPEED_MF_2_6);

   csd->w4.CCC = MMC_CSD_CCC_SET(csd->w4.CCC, MMC_CSD_CCC_CLASS_0 |
                                              MMC_CSD_CCC_CLASS_2 |
                                              MMC_CSD_CCC_CLASS_4 |
                                              MMC_CSD_CCC_CLASS_5 |
                                              MMC_CSD_CCC_CLASS_6 |
                                              MMC_CSD_CCC_CLASS_7 |
                                              MMC_CSD_CCC_CLASS_8);

   csd->w4.READ_BL_LEN =  MMC_CSD_READ_BL_LEN_SET(csd->w4.READ_BL_LEN, MMC_CSD_READ_BL_LEN_512B);

   csd->qw6.C_SIZE = MMC_CSD_C_SIZE_SET(csd->qw6.C_SIZE, 0xFFF); //density is greated than 2GB
   csd->qw6.VDD_R_CURR_MIN = MMC_CSD_VDD_R_CURR_MIN_SET(csd->qw6.VDD_R_CURR_MIN, MMC_CSD_VDD_R_CURR_MIN_60MA);
   csd->qw6.VDD_R_CURR_MAX = MMC_CSD_VDD_R_CURR_MAX_SET(csd->qw6.VDD_R_CURR_MAX, MMC_CSD_VDD_R_CURR_MAX_80MA);
   csd->qw6.VDD_W_CURR_MIN = MMC_CSD_VDD_W_CURR_MIN_SET(csd->qw6.VDD_W_CURR_MIN, MMC_CSD_VDD_W_CURR_MIN_60MA);
   csd->qw6.VDD_W_CURR_MAX = MMC_CSD_VDD_W_CURR_MAX_SET(csd->qw6.VDD_W_CURR_MAX, MMC_CSD_VDD_W_CURR_MAX_80MA);
   csd->qw6.C_SIZE_MULT = MMC_CSD_C_SIZE_MULT_SET(csd->qw6.C_SIZE_MULT, MMC_CSD_C_SIZE_MULT_512);
   csd->qw6.ERASE_GRP_SIZE = MMC_CSD_ERASE_GRP_SIZE_SET(csd->qw6.ERASE_GRP_SIZE, 0x1F);
   csd->qw6.ERASE_GRP_MULT = MMC_CSD_ERASE_GRP_MULT_SET(csd->qw6.ERASE_GRP_MULT, 0x1F);
   csd->qw6.WP_GRP_SIZE = MMC_CSD_WP_GRP_SIZE_SET(csd->qw6.WP_GRP_SIZE, 0x1F);
   csd->qw6.WP_GRP_ENABLE = MMC_CSD_WP_GRP_ENABLE_SET(csd->qw6.WP_GRP_ENABLE);
   csd->qw6.DEFAULT_ECC = MMC_CSD_DEFAULT_ECC_SET(csd->qw6.DEFAULT_ECC, 0);
   csd->qw6.R2W_FACTOR = MMC_CSD_R2W_FACTOR_SET(csd->qw6.R2W_FACTOR, 0);
   csd->qw6.WRITE_BL_LEN = MMC_CSD_WRITE_BL_LEN_SET(csd->qw6.WRITE_BL_LEN, MMC_CSD_WRITE_BL_LEN_512B);

   csd->b14.COPY =  MMC_CSD_COPY_SET(csd->b14.COPY);
   csd->b14.FILE_FORMAT = MMC_CSD_FILE_FORMAT_SET(csd->b14.FILE_FORMAT, MMC_CSD_FILE_FORMAT_HARD_DISK);
   csd->b14.ECC = MMC_CSD_ECC_SET(csd->b14.ECC, MMC_CSD_ECC_NONE);
}

void get_mmc_ext_csd(EXT_CSD_MMCA_4_2* ext_cid)
{
   memset(ext_cid, 0, sizeof(EXT_CSD_MMCA_4_2));

   ext_cid->Reserved_1[0x5E] = 0x01;
   ext_cid->Reserved_1[0xA2] = 0x02;
   ext_cid->Reserved_1[0xAB] = 0x80;
   ext_cid->Reserved_1[0xAF] = 0x01;

   ext_cid->EXT_CSD_REV = MMC_EXT_CSD_EXT_CSD_REV_4_2;
   ext_cid->CSD_STRUCTURE = MMC_EXT_CSD_CSD_STRUCTURE_1_2;
   ext_cid->CARD_TYPE = MMC_EXT_CSD_CARD_TYPE_SET(ext_cid->CARD_TYPE, MMC_EXT_CSD_CARD_TYPE_26MHZ | MMC_EXT_CSD_CARD_TYPE_52MHZ);

   ext_cid->Reserved_9[0x02] = 0x01;

   ext_cid->MIN_PERF_R_4_26 = MMC_EXT_CSD_MIN_PERF_A;
   ext_cid->MIN_PERF_W_4_26 = MMC_EXT_CSD_MIN_PERF_A;
   ext_cid->MIN_PERF_R_8_26_4_52 = MMC_EXT_CSD_MIN_PERF_A;
   ext_cid->MIN_PERF_W_8_26_4_52 = MMC_EXT_CSD_MIN_PERF_A;
   ext_cid->MIN_PERF_R_8_52 = MMC_EXT_CSD_MIN_PERF_A;
   ext_cid->MIN_PERF_W_8_52 = MMC_EXT_CSD_MIN_PERF_A;

   ext_cid->SEC_COUNT = 0x00710000;

   ext_cid->Reserved_12[0x01] = 0x0F;
   ext_cid->Reserved_12[0x03] = 0x06;
   ext_cid->Reserved_12[0x04] = 0x07;
   ext_cid->Reserved_12[0x05] = 0x08;
   ext_cid->Reserved_12[0x06] = 0x08;
   ext_cid->Reserved_12[0x07] = 0x01;
   ext_cid->Reserved_12[0x08] = 0x04;
   ext_cid->Reserved_12[0x09] = 0x05;
   ext_cid->Reserved_12[0x0D] = 0x01;
   ext_cid->Reserved_12[0x0E] = 0x01;
   ext_cid->Reserved_12[0x0F] = 0x15;
   ext_cid->Reserved_12[0x10] = 0x01;
   ext_cid->Reserved_12[0x19] = 0x14;
   ext_cid->Reserved_12[0x1F] = 0xC2;

   ext_cid->S_CMD_SET = MMC_EXT_CSD_S_CMD_SET_MMCA_1;

   ext_cid->Reserved_13[0x0] = 0x12;
   ext_cid->Reserved_13[0x1] = 0x02;
   ext_cid->Reserved_13[0x2] = 0x00;
   ext_cid->Reserved_13[0x3] = 0x13;
   ext_cid->Reserved_13[0x4] = 0x06;
   ext_cid->Reserved_13[0x5] = 0x17;
   ext_cid->Reserved_13[0x6] = 0x00;
}

//-----------------------------------

int g_mmc_card_state = INVALID_MMC_STATE;
int g_mmc_ready_for_data = 0;

int emulate_mmc_command(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
    switch(cmd_data1->command)
    {
        case 0:
        {
            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = IDLE_MMC_STATE;
            g_mmc_ready_for_data = 0;
            return cmd_data1->error_code;
        }
        //ignore intermediate CMD1 response and report that card is ready immediately
        case 1:
        {
            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;

            cmd_data1->response.dw.dw0 = 0x00FF8080 | MMC_INIT_COMPLETE | MMC_CCS_SDHC_SDXC;

            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = READY_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 2:
        {
            MMC_CID mmc_cid;
            get_mmc_cid(&mmc_cid);

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            memcpy_inv(cmd_data1->response.db.data, (char*)&mmc_cid, 0x10);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = IDENTIFICATION_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 3:
        {
            g_mmc_ready_for_data = 1;

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = STANDBY_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 5:
        {
            cmd_data1->error_code = 0x80320002;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = INVALID_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 6:
        {
            switch(cmd_data1->argument)
            {
                case 0x03AF0100:
                {
                    g_mmc_ready_for_data = 0;

                    cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                    cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8);
                    cmd_data1->error_code = 0;
                    cmd_data1->unk_64 = 3;
                    cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                    //ksceKernelDelayThread(100000); //1 second / 10

                    cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

                    g_mmc_card_state = TRANSFER_MMC_STATE;
                    return cmd_data1->error_code;
                }
                case 0x03B90100:
                {
                    g_mmc_ready_for_data = 0;

                    cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                    cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8) | MMC_ILLEGAL_COMMAND_FLAG;
                    cmd_data1->error_code = 0;
                    cmd_data1->unk_64 = 3;
                    cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                    //ksceKernelDelayThread(100000); //1 second / 10

                    cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

                    g_mmc_card_state = TRANSFER_MMC_STATE;
                    return cmd_data1->error_code;
                }
                case 0x03B70100:
                {
                    g_mmc_ready_for_data = 0;

                    cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                    cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8);
                    cmd_data1->error_code = 0;
                    cmd_data1->unk_64 = 3;
                    cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                    //ksceKernelDelayThread(100000); //1 second / 10

                    cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

                    g_mmc_card_state = TRANSFER_MMC_STATE;
                    return cmd_data1->error_code;
                }
                default:
                {
                    #ifdef ENABLE_DEBUG_LOG
                    snprintf(sprintfBuffer, 256, "Unsupported cmd6 argument: %x\n", cmd_data1->argument);
                    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
                    #endif
                    return 0x80320002;
                }
            }
        }
        case 7:
        {
            g_mmc_ready_for_data = 1;

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 8:
        {
            if(g_mmc_card_state == IDLE_MMC_STATE)
            {
                cmd_data1->error_code = 0x80320002;
                cmd_data1->unk_64 = 3;
                cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                g_mmc_card_state = INVALID_MMC_STATE;
                return cmd_data1->error_code;
            }
            else
            {
                g_mmc_ready_for_data = 1;

                cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8);
                cmd_data1->error_code = 0;
                cmd_data1->unk_64 = 3;
                cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                EXT_CSD_MMCA_4_2 mmc_ext_cid;
                get_mmc_ext_csd(&mmc_ext_cid);

                //not sure if this is needed
                /*
                if(cmd_data1->base_198 > 0)
                {
                    memcpy_inv(cmd_data1->base_198, (char*)&mmc_ext_cid, 0x200);
                }
                */

                memcpy_inv(cmd_data1->buffer, (char*)&mmc_ext_cid, 0x200);

                //ksceKernelDelayThread(100000); //1 second / 10

                cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

                g_mmc_card_state = TRANSFER_MMC_STATE;
                return cmd_data1->error_code;
            }
        }
        case 9:
        {
            MMC_CSD mmc_csd;
            get_mmc_csd(&mmc_csd);

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            memcpy_inv(cmd_data1->response.db.data, (char*)&mmc_csd, 0x10);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = STANDBY_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 13:
        {
            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8); //cmd13 returns current state so we dont need to set g_mmc_ready_for_data
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 16:
        {
            cmd_data1->error_code = 0x80320002;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        //this command currently glitches. infinite loop after 2nd command
        case 17:
        {
            g_mmc_ready_for_data = 1;

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            {
                g_ctx_part = ctx->ctx_data.ctx;
                g_sector = cmd_data1->argument;
                g_buffer = cmd_data1->buffer;
                g_nSectors = 1;

                ksceKernelSignalCond(req_cond); //send request
                ksceKernelLockMutex(resp_lock, 1, 0); //lock mutex
                ksceKernelWaitCond(resp_cond, 0); //wait for response
                ksceKernelUnlockMutex(resp_lock, 1); //unlock mutex
            }

            //not sure if this is needed
            if(cmd_data1->base_198 > 0)
            {
                memcpy(cmd_data1->base_198, cmd_data1->buffer, 0x200);
            }

            //ksceKernelDelayThread(100000); //1 second / 10

            cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        //not sure if this command works
        case 23:
        {
            g_mmc_ready_for_data = 1;

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            if(cmd_data2 > 0)
            {
                if(cmd_data2->command == 18)
                {
                    cmd_data2->state_flags = cmd_data2->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                    cmd_data2->response.dw.dw0 = (g_mmc_card_state << 9) | (g_mmc_ready_for_data << 8);
                    cmd_data2->error_code = 0;
                    cmd_data2->unk_64 = 3;
                    cmd_data2->wide_time1 = ksceKernelGetSystemTimeWide();

                    {
                        g_ctx_part = ctx->ctx_data.ctx;
                        g_sector = cmd_data2->argument;
                        g_buffer = cmd_data2->buffer;
                        g_nSectors = cmd_data1->argument;

                        ksceKernelSignalCond(req_cond); //send request
                        ksceKernelLockMutex(resp_lock, 1, 0); //lock mutex
                        ksceKernelWaitCond(resp_cond, 0); //wait for response
                        ksceKernelUnlockMutex(resp_lock, 1); //unlock mutex
                    }

                    //not sure if this is needed
                    if(cmd_data1->base_198 > 0)
                    {
                        memcpy(cmd_data1->base_198, cmd_data2->buffer, 0x200 * cmd_data1->argument);
                    }

                    //ksceKernelDelayThread(100000); //1 second / 10

                    cmd_data2->wide_time2 = ksceKernelGetSystemTimeWide();
                }
                else
                {
                    #ifdef ENABLE_DEBUG_LOG
                    snprintf(sprintfBuffer, 256, "Unsupported command: %d in cmd23\n", cmd_data2->command);
                    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
                    #endif
                    return 0x80320002;
                }
            }
            else
            {
                #ifdef ENABLE_DEBUG_LOG
                FILE_GLOBAL_WRITE_LEN("Expected second command in cmd23\n");
                #endif
                return 0x80320002;
            }

            g_mmc_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 55:
        {
            cmd_data1->error_code = 0x80320002;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_mmc_card_state = INVALID_MMC_STATE;
            return cmd_data1->error_code;
        }
        default:
        {
            #ifdef ENABLE_DEBUG_LOG
            snprintf(sprintfBuffer, 256, "Unsupported command: %d\n", cmd_data1->command);
            FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
            #endif
            return 0x80320002;
        }
    }
}
