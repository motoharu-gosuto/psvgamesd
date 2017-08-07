#include "sd_emu.h" 

#include <psp2kern/kernel/threadmgr.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "global_log.h"
#include "reader.h"
#include "functions.h"

#include "reg_common.h"
#include "sd_reg.h"

#define IDLE_SD_STATE 0
#define READY_SD_STATE 1
#define IDENTIFICATION_SD_STATE 2
#define STANDBY_SD_STATE 3
#define TRANSFER_SD_STATE 4
#define DATA_SD_STATE 5
#define RECEIVE_SD_STATE 6
#define PROGRAMING_SD_STATE 7
#define DISCONNECT_SD_STATE 8
#define INVALID_SD_STATE -1

#define SD_APP_CMD_FLAG 0x00000020

#define SD_COMMAND_COMPLETE_FLAG 0x80000000

#define SD_INIT_COMPLETE 0x80000000
#define SD_CCS_SDHC_SDXC 0x40000000

#define SD_CARD_RCA 0xAAAA

#define SD_SWITCH_FUNCTION_MODE_SWITCH 0x80000000
#define SD_SWITCH_FUNCTION_GROUP1_MASK 0x0000000F
#define SD_SWITCH_FUNCTION_GROUP2_MASK 0x000000F0
#define SD_SWITCH_FUNCTION_GROUP3_MASK 0x00000F00
#define SD_SWITCH_FUNCTION_GROUP4_MASK 0x0000F000

//------------------




const char SD_CMD55_CMD51_data[0x8] = {0x01, 0x00, 0x00, 0x00, 0x40, 0x78, 0x7D, 0x01};

//------------------

#define SD_EMU_MID 0x03
#define SD_EMU_OID "SD"
#define SD_EMU_PNM "SL64G"
#define SD_EMU_PRV_N 8
#define SD_EMU_PRV_M 0
#define SD_EMU_PSN 0xE8904604
#define SD_EMU_YEAR 2016
#define SD_EMU_MON SD_CID_MDT_SEP

void get_sd_cid(SD_CID* cid)
{
   memset(cid, 0, sizeof(SD_CID));

   cid->MID = SD_EMU_MID;
   SD_CID_OID_SET(cid->OID, SD_EMU_OID);
   SD_CID_PNM_SET(cid->PNM, SD_EMU_PNM);
   cid->PRV = SD_CID_PRV_NM_SET(SD_EMU_PRV_N, SD_EMU_PRV_M);
   cid->PSN = SD_CID_PSN_SET(cid->PSN, SD_EMU_PSN);
   cid->MDT = SD_CID_MDT_Y_M_SET(SD_EMU_YEAR, SD_EMU_MON);   
}

void get_sd_csd(SD_CSD_V2* csd)
{
   memset(csd, 0, sizeof(SD_CSD_V2));

   csd->b0.CSD_STRUCTURE = SD_CSD_STRUCTURE_SET(csd->b0.CSD_STRUCTURE, SD_CSD_STRUCTURE_VERSION_2_0);

   csd->TAAC = SD_CSD_V2_TAAC_TU_SET(csd->TAAC, SD_CSD_V2_TAAC_TU_1MS);
   csd->TAAC = SD_CSD_V2_TAAC_MF_SET(csd->TAAC, SD_CSD_V2_TAAC_MF_1_0);
   
   csd->TRAN_SPEED = SD_CSD_V2_TRAN_SPEED_TR_SET(csd->TRAN_SPEED, SD_CSD_V2_TRAN_SPEED_TR_10MBITS);
   csd->TRAN_SPEED = SD_CSD_V2_TRAN_SPEED_TV_SET(csd->TRAN_SPEED, SD_CSD_V2_TRAN_SPEED_TV_2_5);

   csd->w4.CCC = SD_CSD_V2_CCC_SET(csd->w4.CCC, SD_CSD_V2_CCC_CLASS_0 |
                                                SD_CSD_V2_CCC_CLASS_2 |
                                                SD_CSD_V2_CCC_CLASS_4 |
                                                SD_CSD_V2_CCC_CLASS_5 |

                                                SD_CSD_V2_CCC_CLASS_7 |
                                                SD_CSD_V2_CCC_CLASS_8 |
                                                SD_CSD_V2_CCC_CLASS_10);

   csd->w4.READ_BL_LEN = SD_CSD_V2_READ_BL_LEN_SET(csd->w4.READ_BL_LEN, SD_CSD_V2_READ_BL_LEN_512B);

   csd->qw6.C_SIZE = SD_CSD_V2_C_SIZE_SET(csd->qw6.C_SIZE, 0x1DBD3);
   csd->qw6.ERASE_BLK_EN = SD_CSD_V2_ERASE_BLK_EN_SET(csd->qw6.ERASE_BLK_EN);
   csd->qw6.SECTOR_SIZE = SD_CSD_V2_SECTOR_SIZE_SET(csd->qw6.SECTOR_SIZE, 0x7F);
   csd->qw6.R2W_FACTOR = SD_CSD_V2_R2W_FACTOR_SET(csd->qw6.R2W_FACTOR, SD_CSD_V2_R2W_FACTOR_4);
   csd->qw6.WRITE_BL_LEN = SD_CSD_V2_WRITE_BL_LEN_SET(csd->qw6.WRITE_BL_LEN, SD_CSD_V2_WRITE_BL_LEN_512B);
   
   csd->b14.COPY = SD_CSD_V2_COPY_SET(csd->b14.COPY);
}

void get_sd_scr(SD_SCR_V5_00* scr)
{
   memset(scr, 0, sizeof(SD_SCR_V5_00));

   scr->b0.SD_SPEC = SD_SCR_SD_SPEC_SET(scr->b0.SD_SPEC, SD_SCR_SD_SPEC_5_00);

   scr->b1.SD_SECURITY = SD_SCR_V5_00_SD_SECURITY_SET(scr->b1.SD_SECURITY, SD_SCR_V5_00_SD_SECURITY_SDXC_CARD);
   scr->b1.SD_BUS_WIDTHS = SD_SCR_V5_00_SD_BUS_WIDTHS_SET(scr->b1.SD_BUS_WIDTHS, SD_SCR_V5_00_SD_BUS_WIDTHS_1_BIT | SD_SCR_V5_00_SD_BUS_WIDTHS_4_BIT);
      
   scr->w2.SD_SPEC3 = SD_SCR_V5_00_SD_SPEC3_SET(scr->w2.SD_SPEC3);
   scr->w2.SD_SPEC4 = SD_SCR_V5_00_SD_SPEC4_SET(scr->w2.SD_SPEC4);
   scr->w2.SD_SPECX = SD_SCR_V5_00_SD_SPECX_SET(scr->w2.SD_SPECX, SD_SCR_V5_00_SD_SPECX_V5);
   scr->w2.CMD_SUPPORT = SD_SCR_V5_00_CMD_SUPPORT_SET(scr->w2.CMD_SUPPORT, SD_SCR_V5_00_CMD_SUPPORT_SPD_CLS_CTRL);
}

void get_sd_ssr(SSR* ssr)
{
   memset(ssr, 0, sizeof(SSR));

   ssr->SIZE_OF_PROTECTED_AREA = SSR_SIZE_OF_PROTECTED_AREA_SET(ssr->SIZE_OF_PROTECTED_AREA, 0x04000000);
   ssr->SPEED_CLASS = SSR_SPEED_CLASS_SET(ssr->SPEED_CLASS, SSR_SPEED_CLASS_4);
   ssr->AU_SIZE = SSR_AU_SIZE_SET(ssr->AU_SIZE, SSR_AU_SIZE_128KB);

   ssr->ERASE_SIZE = SSR_ERASE_SIZE_SET(ssr->ERASE_SIZE, 0x0007);
   ssr->b13.ERASE_TIMEOUT = SSR_ERASE_TIMEOUT_SET(ssr->b13.ERASE_TIMEOUT, 0x20);
   ssr->b13.ERASE_OFFSET = SSR_ERASE_OFFSET_SET(ssr->b13.ERASE_OFFSET, SSR_ERASE_OFFSET_2SEC);
   ssr->b14.UHS_SPEED_GRADE = SSR_UHS_SPEED_GRADE_SET(ssr->b14.UHS_SPEED_GRADE, SSR_UHS_SPEED_GRADE_GE30MBs); //reserved
   ssr->b14.UHS_AU_SIZE = SSR_UHS_AU_SIZE_SET(ssr->b14.UHS_AU_SIZE, SSR_UHS_AU_SIZE_1MB); //reserved
}

void get_sw_status(SW_STATUS_V1* swst)
{
   memset(swst, 0, sizeof(SW_STATUS_V1));

   swst->MAX_CURR_PWR_CONS = SW_STATUS_V1_MAX_CURR_PWR_CONS_SET(swst->MAX_CURR_PWR_CONS, 0x0064);
   swst->SPRT_BIT_FUN_GR_6 = SW_STATUS_V1_SPRT_BIT_FUN_GR_6_SET(swst->SPRT_BIT_FUN_GR_6, 0x4000);
   swst->SPRT_BIT_FUN_GR_5 = SW_STATUS_V1_SPRT_BIT_FUN_GR_5_SET(swst->SPRT_BIT_FUN_GR_5, 0xC000);
   swst->SPRT_BIT_FUN_GR_4 = SW_STATUS_V1_SPRT_BIT_FUN_GR_4_SET(swst->SPRT_BIT_FUN_GR_4, 0xC000);
   swst->SPRT_BIT_FUN_GR_3 = SW_STATUS_V1_SPRT_BIT_FUN_GR_3_SET(swst->SPRT_BIT_FUN_GR_3, 0xC000);
   swst->SPRT_BIT_FUN_GR_2 = SW_STATUS_V1_SPRT_BIT_FUN_GR_2_SET(swst->SPRT_BIT_FUN_GR_2, 0xE000);
   swst->SPRT_BIT_FUN_GR_1 = SW_STATUS_V1_SPRT_BIT_FUN_GR_1_SET(swst->SPRT_BIT_FUN_GR_1, 0xC001);

   swst->b14.FUN_SEL_FUN_GR_6 = SW_STATUS_V1_FUN_SEL_FUN_GR_6_SET(swst->b14.FUN_SEL_FUN_GR_6, 0x8);
   swst->b14.FUN_SEL_FUN_GR_5 = SW_STATUS_V1_FUN_SEL_FUN_GR_5_SET(swst->b14.FUN_SEL_FUN_GR_5, 0x0);

   swst->DAT_STUCT_VER = SW_STATUS_V1_DAT_STUCT_VER_SET(swst->DAT_STUCT_VER, 0x80);
}

//------------------

int g_sd_card_state = INVALID_SD_STATE;
int g_sd_ready_for_data = 0;

int g_sd_com_crc_error = 0;
int g_sd_illegal_command = 0;

int g_init_cnt = 0;

int emulate_sd_command(sd_context_global* ctx, cmd_input* cmd_data1, cmd_input* cmd_data2, int nIter, int num)
{
  switch(cmd_data1->command)
  {
    //reset card
    case 0:
    {
        cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
        cmd_data1->error_code = 0;
        cmd_data1->unk_64 = 3;
        cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();
        
        g_sd_card_state = IDLE_SD_STATE;
        g_sd_ready_for_data = 0;
        g_sd_com_crc_error = 0;
        g_sd_illegal_command = 0;
        g_init_cnt = 0;
        return cmd_data1->error_code;
    }
    //send if cond
    case 8:
    {
      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      cmd_data1->response.dw.dw0 = cmd_data1->argument; //echo argument back
      cmd_data1->error_code = 0;
      cmd_data1->unk_64 = 3;
      cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

      g_sd_card_state = IDLE_SD_STATE;
      return cmd_data1->error_code;
    }
    //sdio compatible check
    case 5:
    {
      cmd_data1->error_code = 0x80320002;
      cmd_data1->unk_64 = 3;
      cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

      g_sd_com_crc_error = 1;
      g_sd_illegal_command = 1;

      g_sd_card_state = INVALID_SD_STATE;
      return cmd_data1->error_code;
    }
    //acmd prepare
    case 55:
    {
      //in invalid or idle state - process only initialization commands
      if(g_sd_card_state == INVALID_SD_STATE || g_sd_card_state == IDLE_SD_STATE)
      {
        g_sd_ready_for_data = 1;

        cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
        cmd_data1->response.dw.dw0 = (g_sd_com_crc_error << 23) | (g_sd_illegal_command << 22) | (g_sd_card_state << 9) | (g_sd_ready_for_data << 8) | SD_APP_CMD_FLAG;
        cmd_data1->error_code = 0;
        cmd_data1->unk_64 = 3;
        cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

        if(cmd_data2 > 0)
        {
            if(cmd_data2->command == 41)
            {
              cmd_data2->state_flags = cmd_data2->state_flags | SD_COMMAND_COMPLETE_FLAG;
              
              cmd_data2->response.dw.dw0 = 0x00FF8000 | SD_CCS_SDHC_SDXC;

              if(g_init_cnt > 0)
              {
                cmd_data2->response.dw.dw0 = cmd_data2->response.dw.dw0 | SD_INIT_COMPLETE;
              }

              cmd_data2->error_code = 0;
              cmd_data2->unk_64 = 3;
              cmd_data2->wide_time1 = ksceKernelGetSystemTimeWide();
            }
            else
            {
              snprintf(sprintfBuffer, 256, "Unsupported command: %d in cmd55\n", cmd_data2->command);
              FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
              return 0x80320002;
            }
        }
        else
        {
          FILE_GLOBAL_WRITE_LEN("Expected second command in cmd55\n");
          return 0x80320002;
        }

        //these flags are originally set by CMD5
        g_sd_com_crc_error = 0;
        g_sd_illegal_command = 0;

        g_init_cnt++;

        g_sd_card_state = IDLE_SD_STATE;
        return cmd_data1->error_code;
      }
      else
      {
        g_sd_ready_for_data = 1;

        cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
        cmd_data1->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8) | SD_APP_CMD_FLAG;
        cmd_data1->error_code = 0;
        cmd_data1->unk_64 = 3;
        cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

        if(cmd_data2 > 0)
        {
            switch(cmd_data2->command)
            {
              //set / clear card detect
              case 42:
              {
                cmd_data2->state_flags = cmd_data2->state_flags | SD_COMMAND_COMPLETE_FLAG;
                cmd_data2->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8) | SD_APP_CMD_FLAG;
                cmd_data2->error_code = 0;
                cmd_data2->unk_64 = 3;
                cmd_data2->wide_time1 = ksceKernelGetSystemTimeWide();

                g_sd_card_state = TRANSFER_SD_STATE;
                return cmd_data1->error_code;
              }
              //send sd status
              case 13:
              {
                cmd_data2->state_flags = cmd_data2->state_flags | SD_COMMAND_COMPLETE_FLAG;
                cmd_data2->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8) | SD_APP_CMD_FLAG;
                cmd_data2->error_code = 0;
                cmd_data2->unk_64 = 3;
                cmd_data2->wide_time1 = ksceKernelGetSystemTimeWide();

                SSR sd_ssr;
                get_sd_ssr(&sd_ssr);

                memcpy_inv(cmd_data2->buffer, (char*)&sd_ssr, 0x40);

                g_sd_card_state = TRANSFER_SD_STATE;
                return cmd_data1->error_code;
              }
              //send SCR
              case 51:
              {
                cmd_data2->state_flags = cmd_data2->state_flags | SD_COMMAND_COMPLETE_FLAG;
                cmd_data2->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8) | SD_APP_CMD_FLAG;
                cmd_data2->error_code = 0;
                cmd_data2->unk_64 = 3;
                cmd_data2->wide_time1 = ksceKernelGetSystemTimeWide();

                SD_SCR_V5_00 sd_scr;
                get_sd_scr(&sd_scr);

                memcpy_inv(cmd_data2->buffer, (char*)&sd_scr, 0x08);

                g_sd_card_state = TRANSFER_SD_STATE;
                return cmd_data1->error_code;
              }
              //set bus width
              case 6:
              {
                cmd_data2->state_flags = cmd_data2->state_flags | SD_COMMAND_COMPLETE_FLAG;
                cmd_data2->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8) | SD_APP_CMD_FLAG;
                cmd_data2->error_code = 0;
                cmd_data2->unk_64 = 3;
                cmd_data2->wide_time1 = ksceKernelGetSystemTimeWide();

                g_sd_card_state = TRANSFER_SD_STATE;
                return cmd_data1->error_code;
              }
              default:
              {
                snprintf(sprintfBuffer, 256, "Unsupported command: %d in cmd55\n", cmd_data2->command);
                FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
                return 0x80320002;
              }
            }
        }
        else
        {
          FILE_GLOBAL_WRITE_LEN("Expected second command in cmd55\n");
          return 0x80320002;
        }
      }
    }
    //get CID
    case 2:
    {
      SD_CID sd_cid;
      get_sd_cid(&sd_cid);

      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      memcpy_inv(cmd_data1->response.db.data, (char*)&sd_cid, 0x10);
      cmd_data1->error_code = 0;
      cmd_data1->unk_64 = 3;
      cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

      g_sd_card_state = IDENTIFICATION_SD_STATE;
      return cmd_data1->error_code;
    }
    //publish RCA
    case 3:
    {
      g_sd_ready_for_data = 1;

      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      
      cmd_data1->response.dw.dw0 = SD_CARD_RCA | (g_sd_card_state << 9) | (g_sd_ready_for_data << 8) | SD_APP_CMD_FLAG; //not sure why app flag is set

      cmd_data1->error_code = 0;
      cmd_data1->unk_64 = 3;
      cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

      g_sd_card_state = STANDBY_SD_STATE;
      return cmd_data1->error_code;
    }
    //switch function
    case 6:
    {
      if((cmd_data1->argument & SD_SWITCH_FUNCTION_GROUP1_MASK) == 1)
      {
        g_sd_ready_for_data = 1;

        cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
        cmd_data1->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8);

        cmd_data1->error_code = 0;
        cmd_data1->unk_64 = 3;
        cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

        SW_STATUS_V1 sd_swst;
        get_sw_status(&sd_swst);

        memcpy_inv(cmd_data1->buffer, (char*)&sd_swst, 0x40);

        g_sd_card_state = TRANSFER_SD_STATE;
        return cmd_data1->error_code;
      }
      else
      {
        snprintf(sprintfBuffer, 256, "Unsupported cmd6 argument: %x\n", cmd_data1->argument);
        FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
        return 0x80320002;
      }
    }
    //select card
    case 7:
    {
      g_sd_ready_for_data = 1;

      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      cmd_data1->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8);

      cmd_data1->error_code = 0;
      cmd_data1->unk_64 = 3;
      cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

      g_sd_card_state = TRANSFER_SD_STATE;
      return cmd_data1->error_code;
    }
    //send CSD
    case 9:
    {
      SD_CSD_V2 sd_csd;
      get_sd_csd(&sd_csd);

      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      memcpy_inv(cmd_data1->response.db.data, (char*)&sd_csd, 0x10);
      cmd_data1->error_code = 0;
      cmd_data1->unk_64 = 3;
      cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

      g_sd_card_state = STANDBY_SD_STATE;
      return cmd_data1->error_code;
    }
    //get status
    case 13:
    {
      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      cmd_data1->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8); //cmd13 returns current state so we dont need to set g_sd_ready_for_data
      cmd_data1->error_code = 0;
      cmd_data1->unk_64 = 3;
      cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

      g_sd_card_state = TRANSFER_SD_STATE;
      return cmd_data1->error_code;
    }
    //set block len
    case 16:
    {
      g_sd_ready_for_data = 1;

      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      cmd_data1->response.dw.dw0 = (g_sd_card_state << 9) | (g_sd_ready_for_data << 8);

      cmd_data1->error_code = 0;
      cmd_data1->unk_64 = 3;
      cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

      g_sd_card_state = TRANSFER_SD_STATE;
      return cmd_data1->error_code;
    }
    case 17:
    {
       //not implemented
    }
    case 23:
    {
      //not implemented
    }
    default:
    {
        snprintf(sprintfBuffer, 256, "Unsupported command: %d\n", cmd_data1->command);
        FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
        return 0x80320002;
    }
  }  
}