#pragma once

#include <taihen.h>

#define SceSdifForDriver_NID 0x96D306FA
#define SceIofilemgrForDriver_NID 0x40FD29C7
#define SceSblGcAuthMgrGcAuthForDriver_NID 0xC6627F5E
#define SceKernelUtilsForDriver_NID 0x496AD8B4
#define SceSblGcAuthMgrDrmBBForDriver_NID 0x1926B182

extern tai_hook_ref_t sd_read_hook_ref;
extern SceUID sd_read_hook_id;

extern tai_hook_ref_t sd_write_hook_ref;
extern SceUID sd_write_hook_id;

extern SceUID gen_init_1_patch_uid;
extern SceUID gen_init_2_patch_uid;
extern SceUID gen_init_3_patch_uid;

extern SceUID hs_dis_patch1_uid;
extern SceUID hs_dis_patch2_uid;

extern tai_hook_ref_t init_sd_hook_ref;
extern SceUID init_sd_hook_id;

extern tai_hook_ref_t send_command_hook_ref;
extern SceUID send_command_hook_id;

extern tai_hook_ref_t gc_cmd56_handshake_hook_ref;
extern SceUID gc_cmd56_handshake_hook_id;

extern tai_hook_ref_t mmc_read_hook_ref;
extern SceUID mmc_read_hook_id;

extern tai_hook_ref_t mmc_write_hook_ref;
extern SceUID mmc_write_hook_id;

extern tai_hook_ref_t get_insert_state_hook_ref;
extern SceUID get_insert_state_hook_id;

extern tai_hook_ref_t clear_sensitive_data_hook_ref;
extern SceUID clear_sensitive_data_hook_id;

extern SceUID insert_handler_patch_id;

extern SceUID remove_handler_patch_id;

extern tai_hook_ref_t sys_wide_time_hook_ref;
extern SceUID sys_wide_time_hook_id;

extern tai_hook_ref_t fast_mutex_lock_hook_ref;
extern SceUID fast_mutex_lock_hook_id;

extern tai_hook_ref_t fast_mutex_unlock_hook_ref;
extern SceUID fast_mutex_unlock_hook_id;

extern SceUID suspend_cid_check_patch_id;
extern SceUID resume_cid_check_patch_id;
