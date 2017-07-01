#pragma once

#include <stdint.h>

#pragma pack(push, 1)

struct sd_context_global;
struct cmd_input;

typedef struct cmd_input // size is 0x240
{
   uint32_t size; // 0x240
   uint32_t unk_4;
   uint32_t command;
   uint32_t argument;
   
   uint32_t unk_10;
   uint32_t unk_14;
   uint32_t unk_18;
   uint32_t unk_1C;

   void* buffer; // cmd data buffer ptr
   uint16_t b_size; // cmd buffer size
   uint16_t flags; // unknown
   uint32_t unk_28;
   uint32_t unk_2C;

   uint8_t data0[0x30];   
   
   struct cmd_input* next_cmd;
   uint32_t unk_64;
   uint32_t array_index;
   uint32_t unk_6C;
   
   uint32_t unk_70;
   uint32_t unk_74;
   struct sd_context_global* gctx_ptr;
   uint32_t unk_7C;
   
   void* vaddr_80; //3
   uint32_t unk_84;
   uint32_t unk_88;
   uint32_t unk_8C;

   uint8_t data1[0xF0];

   uint32_t unk_180;
   void* paddr_184; //3
   uint32_t unk_188;
   uint32_t unk_18C;

   uint32_t unk_190;
   uint32_t unk_194;
   uint32_t unk_198;
   uint32_t unk_19C;

   uint32_t unk_1A0;
   uint32_t unk_1A4;
   void* paddr_1A8; //1
   void* paddr_1AC; //2

   uint32_t unk_1B0;
   uint32_t unk_1B4;
   uint32_t unk_1B8;
   uint32_t unk_1BC;

   void* vaddr_1C0; //1
   uint32_t unk_1C4;
   uint32_t unk_1C8;
   uint32_t unk_1CC;

   uint8_t data2[0x30];

   void* vaddr_200; //2
   uint32_t unk_204;
   uint32_t unk_208;
   uint32_t unk_20C;

   uint8_t data3[0x30];
} cmd_input;

typedef struct sd_context_data // size is 0xC0
{
    struct cmd_input* cmd_ptr;
    struct cmd_input* cmd_ptr_next;
    uint32_t unk_8;
    uint32_t unk_C;
    
    uint32_t dev_type_idx; // (1,2,3)
    void* ctx; //pointer to custom context (sd_context_part_mmc*, sd_context_part_sd*, sd_context_part_wlanbt*)
    uint32_t unk_18;
    uint32_t unk_1C;

    uint32_t array_idx; // (0,1,2)
    uint32_t unk_24;
    uint32_t unk_28;
    uint32_t unk_2C;

    void* membase_1000; // membase of SceSdif (0,1,2) memblock of size 0x1000
    uint32_t unk_34;
    uint32_t unk_38;
    SceUID uid_1000; // UID of SceSdif (0,1,2) memblock of size 0x1000

    uint32_t unk_40; // SceKernelThreadMgr related, probably UID for SceSdif (0,1,2)
    uint32_t unk_44;
    uint32_t unk_48;
    uint32_t unk_4C;

    uint32_t unk_50;
    uint32_t unk_54;
    uint32_t unk_58;
    uint32_t unk_5C;

    uint32_t unk_60;
    uint32_t unk_64;
    uint32_t unk_68;
    uint32_t unk_6C;

    uint32_t unk_70;
    uint32_t unk_74;
    uint32_t unk_78;
    uint32_t unk_7C;

    //it looks like this chunk is separate structure since offset 0x2480 is used too often

    uint32_t unk_80;
    SceUID uid_10000; // UID of SceSdif (0,1,2) memblock of size 0x10000
    void* membase_10000; // membase of SceSdif (0,1,2) memblock of size 0x10000
    uint32_t unk_8C;

    uint32_t unk_90;
    uint32_t lockable_int;
    uint32_t unk_98;
    uint32_t unk_9C;

    uint32_t unk_A0;
    uint32_t unk_A4;
    uint32_t unk_A8;
    uint32_t unk_AC;

    uint32_t unk_B0;
    uint32_t unk_B4;
    uint32_t unk_B8;
    uint32_t unk_BC;
} sd_context_data;

typedef struct sd_context_part_base
{
   struct sd_context_global* gctx_ptr;
   uint32_t unk_4;
   uint32_t size; //cmd buffer size
   uint32_t unk_C; //0 for mmc however 0x200 for sd, can be size

   uint8_t unk_10; //can be padding
   uint8_t CID[15]; //this is CID data but in reverse

   uint8_t unk_20; //can be padding
   uint8_t CSD[15]; //this is CSD data but in reverse
}sd_context_part_base;

typedef struct sd_context_part_mmc // size is 0x398
{
   sd_context_part_base ctxb;
   
   uint8_t data[0x360];
   
   void* unk_390;
   uint32_t unk_394;
} sd_context_part_mmc;

typedef struct sd_context_part_sd // size is 0xC0
{
   sd_context_part_base ctxb;

   uint8_t data[0x90];
} sd_context_part_sd;

typedef struct sd_context_part_wlanbt // size is 0x398
{
   struct sd_context_global* gctx_ptr;
   
   uint8_t data[0x394];
} sd_context_part_wlanbt;

typedef struct sd_context_global // size is 0x24C0
{
    struct cmd_input commands[16];
    struct sd_context_data ctx_data;
} sd_context_global;

typedef struct output_23a4ef01
{
    int unk_0;
    int unk_4;
    int unk_8;
    int unk_C;
}output_23a4ef01;

#pragma pack(pop)

//TODO: API does not have best naming. please check the wiki also

#define SCE_SDIF_DEV_EMMC 0
#define SCE_SDIF_DEV_GAME_CARD 1
#define SCE_SDIF_DEV_WLAN_BT 2

sd_context_global* ksceSdifGetSdContextGlobal(int sd_ctx_idx);

sd_context_part_mmc* ksceSdifGetSdContextPartMmc(int sd_ctx_idx);
sd_context_part_sd* ksceSdifGetSdContextPartSd(int sd_ctx_idx);
sd_context_part_wlanbt* ksceSdifGetSdContextPartSdio(int sd_ctx_idx);

int ksceSdifGetCardInsertState1(int sd_ctx_idx);
int ksceSdifGetCardInsertState2(int sd_ctx_idx);

int ksceSdifInitializeSdContextPartMmc(int sd_ctx_index, sd_context_part_mmc** result);
int ksceSdifInitializeSdContextPartSd(int sd_ctx_index, sd_context_part_sd** result);

//uses CMD17 for single sector and CMD23, CMD24 for multiple sectors
int ksceSdifReadSectorAsync(void* ctx_part, int sector, char* buffer, int nSectors);
int ksceSdifReadSector(void* ctx_part, int sector, char* buffer, int nSectors);

//uses CMD18 for single sector and CMD23, CMD25 for multiple sectors
int ksceSdifWriteSectorAsync(void* ctx_part, int sector, char* buffer, int nSectors);
int ksceSdifWriteSector(void* ctx_part, int sector, char* buffer, int nSectors);

int ksceSdifCopyCtx(void* ctx_part, output_23a4ef01* unk0);

//=================================================

int ksceMsifReadSector(int sector, char* buffer, int nSectors);
int ksceMsifWriteSector(int sector, char* buffer, int nSectors);
int ksceMsifEnableSlowMode();
int ksceMsifDisableSlowMode();
int ksceMsifGetSlowModeState();

int ksceMsifInit1();
int ksceMsifInit2(char* unk0_40); 
