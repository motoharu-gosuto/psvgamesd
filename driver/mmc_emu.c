
#include "mmc_emu.h"

#include <psp2kern/kernel/threadmgr.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "global_log.h"
#include "reader.h"
#include "functions.h"

char sprintfBuffer[256];

#define IDLE_MMC_STATE 0
#define READY_MMC_STATE 1
#define IDENTIFICATION_MMC_STATE 2
#define STANDBY_MMC_STATE 3
#define TRANSFER_MMC_STATE 4
#define DATA_MMC_STATE 5
#define RECEIVE_MMC_STATE 6
#define PROGRAMING_MMC_STATE 7
#define DISCONNECT_STATE 8
#define BUS_TEST_STATE 9
#define SLEEP_MMC_STATE 10
#define INVALID_MMC_STATE -1

#define MMC_ILLEGAL_COMMAND_FLAG 0x00400000

#define MMC_COMMAND_COMPLETE_FLAG 0x80000000

const char CMD2_resp[0x10] = {0x00, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x10, 0x41, 0x41, 0x46, 0x54, 0x4C, 0x50, 0x11, 0x00, 0x11};
const char CMD9_resp[0x10] = {0x00, 0x40, 0x40, 0x82, 0xFF, 0xFF, 0xDB, 0xF6, 0xFF, 0x03, 0x59, 0x1F, 0x32, 0x01, 0x7F, 0x90};

const char CMD8_data[0x200] = 
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x0F, 0x00, 0x06, 0x07, 0x08, 0x08, 0x01, 
    0x04, 0x05, 0x00, 0x00, 0x00, 0x01, 0x01, 0x15, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x12, 0x02, 0x00, 0x13, 0x06, 0x17, 0x00
};

int g_card_state = INVALID_MMC_STATE;
int g_ready_for_data = 0;

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
            
            g_card_state = IDLE_MMC_STATE;
            g_ready_for_data = 0;
            return cmd_data1->error_code;
        }
        case 1:
        {
            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = 0xC0FF8080;
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = READY_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 2:
        {
            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            memcpy(cmd_data1->response.db.data, CMD2_resp, 0x10);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = IDENTIFICATION_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 3:
        {
            g_ready_for_data = 1;

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = STANDBY_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 5:
        {
            cmd_data1->error_code = 0x80320002;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = INVALID_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 6:
        {
            switch(cmd_data1->argument)
            {
                case 0x03AF0100:
                {
                    g_ready_for_data = 0;

                    cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                    cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
                    cmd_data1->error_code = 0;
                    cmd_data1->unk_64 = 3;
                    cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                    //ksceKernelDelayThread(100000); //1 second / 10

                    cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

                    g_card_state = TRANSFER_MMC_STATE;
                    return cmd_data1->error_code;
                }
                case 0x03B90100:
                {
                    g_ready_for_data = 0;

                    cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                    cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8) | MMC_ILLEGAL_COMMAND_FLAG;
                    cmd_data1->error_code = 0;
                    cmd_data1->unk_64 = 3;
                    cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                    //ksceKernelDelayThread(100000); //1 second / 10

                    cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

                    g_card_state = TRANSFER_MMC_STATE;
                    return cmd_data1->error_code;
                }
                case 0x03B70100:
                {
                    g_ready_for_data = 0;

                    cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                    cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
                    cmd_data1->error_code = 0;
                    cmd_data1->unk_64 = 3;
                    cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                    //ksceKernelDelayThread(100000); //1 second / 10

                    cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

                    g_card_state = TRANSFER_MMC_STATE;
                    return cmd_data1->error_code;
                }
                default:
                {
                    snprintf(sprintfBuffer, 256, "Unsupported cmd6 argument: %x\n", cmd_data1->argument);
                    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
                    return 0x80320002;
                }
            }
        }
        case 7:
        {
            g_ready_for_data = 1;

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 8:
        {
            if(g_card_state == IDLE_MMC_STATE)
            {
                cmd_data1->error_code = 0x80320002;
                cmd_data1->unk_64 = 3;
                cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                g_card_state = INVALID_MMC_STATE;
                return cmd_data1->error_code;
            }
            else
            {
                g_ready_for_data = 1;

                cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
                cmd_data1->error_code = 0;
                cmd_data1->unk_64 = 3;
                cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

                //not sure if this is needed
                if(cmd_data1->base_198 > 0)
                {
                    memcpy(cmd_data1->base_198, CMD8_data, 0x200);
                }

                memcpy(cmd_data1->buffer, CMD8_data, 0x200);

                //ksceKernelDelayThread(100000); //1 second / 10

                cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

                g_card_state = TRANSFER_MMC_STATE;
                return cmd_data1->error_code;
            }
        }
        case 9:
        {
            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            memcpy(cmd_data1->response.db.data, CMD9_resp, 0x10);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = STANDBY_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 13:
        {
            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 16:
        {
            cmd_data1->error_code = 0x80320002;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        //this command currently glitches. infinite loop after 2nd command
        case 17:
        {
            g_ready_for_data = 1;

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            {
                g_ctx_part = ctx->ctx_data.ctx;
                g_sector = cmd_data1->argument;
                g_buffer = cmd_data1->buffer;
                g_nSectors = 1;

                sceKernelSignalCondForDriver(req_cond); //send request
                ksceKernelLockMutex(resp_lock, 1, 0); //lock mutex
                sceKernelWaitCondForDriver(resp_cond, 0); //wait for response
                ksceKernelUnlockMutex(resp_lock, 1); //unlock mutex
            }

            //not sure if this is needed
            if(cmd_data1->base_198 > 0)
            {
                memcpy(cmd_data1->base_198, cmd_data1->buffer, 0x200);
            }

            //ksceKernelDelayThread(100000); //1 second / 10

            cmd_data1->wide_time2 = ksceKernelGetSystemTimeWide();

            g_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        //not sure if this command works
        case 23:
        {
            g_ready_for_data = 1;

            cmd_data1->state_flags = cmd_data1->state_flags | MMC_COMMAND_COMPLETE_FLAG;
            cmd_data1->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
            cmd_data1->error_code = 0;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            if(cmd_data2 > 0)
            {
                if(cmd_data2->command == 18)
                {
                    cmd_data2->state_flags = cmd_data2->state_flags | MMC_COMMAND_COMPLETE_FLAG;
                    cmd_data2->response.dw.dw0 = (g_card_state << 9) | (g_ready_for_data << 8);
                    cmd_data2->error_code = 0;
                    cmd_data2->unk_64 = 3;
                    cmd_data2->wide_time1 = ksceKernelGetSystemTimeWide();

                    {
                        g_ctx_part = ctx->ctx_data.ctx;
                        g_sector = cmd_data2->argument;
                        g_buffer = cmd_data2->buffer;
                        g_nSectors = cmd_data1->argument;

                        sceKernelSignalCondForDriver(req_cond); //send request
                        ksceKernelLockMutex(resp_lock, 1, 0); //lock mutex
                        sceKernelWaitCondForDriver(resp_cond, 0); //wait for response
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
                    snprintf(sprintfBuffer, 256, "Unsupported command: %d in cmd23\n", cmd_data2->command);
                    FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
                    return 0x80320002;
                }
            }
            else
            {
                FILE_GLOBAL_WRITE_LEN("Expected second command in cmd23\n");
                return 0x80320002;
            }

            g_card_state = TRANSFER_MMC_STATE;
            return cmd_data1->error_code;
        }
        case 55:
        {
            cmd_data1->error_code = 0x80320002;
            cmd_data1->unk_64 = 3;
            cmd_data1->wide_time1 = ksceKernelGetSystemTimeWide();

            g_card_state = INVALID_MMC_STATE;
            return cmd_data1->error_code;
        }
        default:
        {
            snprintf(sprintfBuffer, 256, "Unsupported command: %d\n", cmd_data1->command);
            FILE_GLOBAL_WRITE_LEN(sprintfBuffer);
            return 0x80320002;
        }
    }
} 
