#include "sd_emu.h" 

#include <psp2kern/kernel/threadmgr.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "global_log.h"
#include "reader.h"
#include "functions.h"

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
#define SD_SWITCH_FUNCTION_GROUP1 0x0000000F
#define SD_SWITCH_FUNCTION_GROUP2 0x000000F0
#define SD_SWITCH_FUNCTION_GROUP3 0x00000F00
#define SD_SWITCH_FUNCTION_GROUP4 0x0000F000

const char SD_CMD2_resp[0x10] = {0x00, 0x09, 0x01, 0x04, 0x46, 0x90, 0xE8, 0x80, 0x47, 0x34, 0x36, 0x4C, 0x53, 0x44, 0x53, 0x03};
const char SD_CMD9_resp[0x10] = {0x00, 0x40, 0x40, 0x0A, 0x80, 0x7F, 0xD3, 0xDB, 0x01, 0x00, 0x59, 0x5B, 0x32, 0x00, 0x0E, 0x40};

const char SD_CMD55_CMD51_data[0x8] = {0x01, 0x22, 0xC0, 0x01, 0x80, 0x00, 0x00, 0x00};
                                 
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

                memcpy(cmd_data2->buffer, SD_CMD55_CMD51_data, 0x08);

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
      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      memcpy(cmd_data1->response.db.data, SD_CMD2_resp, 0x10);
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
      if((cmd_data1->argument & SD_SWITCH_FUNCTION_GROUP1) == 1)
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
      cmd_data1->state_flags = cmd_data1->state_flags | SD_COMMAND_COMPLETE_FLAG;
      memcpy(cmd_data1->response.db.data, SD_CMD9_resp, 0x10);
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