#include <stdio.h>
#include <string.h>

#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/shellutil.h>

#include <psvgamesd_api.h>

#include "debugScreen.h"
#include "sfo_utils.h"

//---

#define ISO_ROOT_DIRECTORY "ux0:iso"

//original mode
#define DRIVER_MODE_PHYSICAL_MMC 0
//emulate mmc
#define DRIVER_MODE_VIRTUAL_MMC 1
//physical sd card with patches
#define DRIVER_MODE_PHYSICAL_SD 2
//emulate sd
#define DRIVER_MODE_VIRTUAL_SD 3

#define DRIVER_MODE_NAME_PHYSICAL_MMC "physical mmc"
#define DRIVER_MODE_NAME_VIRTUAL_MMC "virtual mmc"
#define DRIVER_MODE_NAME_PHYSICAL_SD "physical sd"
#define DRIVER_MODE_NAME_VIRTUAL_SD "virtual sd"

//---

#define INSERTION_STATE_REMOVED 0
#define INSERTION_STATE_INSERTED 1

//---

int lock_ps_btn = 0;

void ps_btn_lock() 
{
   if (lock_ps_btn == 0)
   {
      int result = sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
      lock_ps_btn = 1;
   }
}

void ps_btn_unlock() 
{
   if (lock_ps_btn == 1)
   {
      int result = sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
      lock_ps_btn = 0;
   }
}

//---

char g_current_directory[256] = {0};

int get_dir_max_file_pos(const char* path)
{
  int max_file_position = 0;

  SceUID dirId = sceIoDopen(path);
  if(dirId >= 0)
  {
    int res = 0;
    do
    {
      SceIoDirent dir;
      memset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dirId, &dir);
      if(res > 0)
      {
        if(SCE_S_ISREG(dir.d_stat.st_mode))
        {
          max_file_position++;          
        }
      }
    }
    while(res > 0);

    sceIoDclose(dirId);
  }
  
  if(max_file_position > 0)
    max_file_position = max_file_position - 1;

  return max_file_position;
}

int get_dir_filename_at_pos(char* path, uint32_t pos, char* dest)
{
  memset(dest, 0, 256);

  int cur_file_position = 0;

  SceUID dirId = sceIoDopen(path);
  if(dirId >= 0)
  {
    int found = -1;

    int res = 0;
    do
    {
      SceIoDirent dir;
      memset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dirId, &dir);
      if(res > 0)
      {
        if(SCE_S_ISREG(dir.d_stat.st_mode))
        {
          if(cur_file_position == pos)
          {
            strncpy(dest, dir.d_name, 256);
            found = 0;
            break;
          }

          cur_file_position++;          
        }
      }
    }
    while(res > 0);

    sceIoDclose(dirId);

    return found;
  }
  else
  {
    return -1;
  }
}

int driver_mode_to_name(uint32_t mode, char* dest)
{
  memset(dest, 0, 256);

  switch(mode)
  {
    case DRIVER_MODE_PHYSICAL_MMC:
      strncpy(dest, DRIVER_MODE_NAME_PHYSICAL_MMC, 256);
      break;
    case DRIVER_MODE_VIRTUAL_MMC:
      strncpy(dest, DRIVER_MODE_NAME_VIRTUAL_MMC, 256);
      break;
    case DRIVER_MODE_PHYSICAL_SD:
      strncpy(dest, DRIVER_MODE_NAME_PHYSICAL_SD, 256);
      break;
    case DRIVER_MODE_VIRTUAL_SD:
      strncpy(dest, DRIVER_MODE_NAME_VIRTUAL_SD, 256);
      break;
    default:
      strncpy(dest, "unknown", 256);
      break;
  }

  return 0;
}

//---

uint32_t g_old_buttons = 0;
uint32_t g_current_buttons = 0;
uint32_t g_pressed_buttons = 0;
uint32_t g_released_buttons = 0;

int readInput()
{
  SceCtrlData ctrl;
  memset(&ctrl, 0, sizeof(SceCtrlData));
  sceCtrlPeekBufferPositive(0, &ctrl, 1);

  g_old_buttons = g_current_buttons;
  g_current_buttons = ctrl.buttons;

  g_pressed_buttons = g_current_buttons & ~g_old_buttons;
  g_released_buttons = ~g_current_buttons & g_old_buttons;

  return g_pressed_buttons;
}

//##########################################################################################

SceUID g_app_running_mutex_id = -1;

uint32_t g_app_running = 0;

uint32_t get_app_running()
{
  sceKernelLockMutex(g_app_running_mutex_id, 1, 0);
  uint32_t temp = g_app_running;
  sceKernelUnlockMutex(g_app_running_mutex_id, 1);
  return temp;
}

void set_app_running(uint32_t value)
{
  sceKernelLockMutex(g_app_running_mutex_id, 1, 0);
  g_app_running = value;
  sceKernelUnlockMutex(g_app_running_mutex_id, 1);
}

//---

SceUID g_redraw_request_mutex_id = -1;

uint32_t g_redraw_request = 0;

uint32_t get_redraw_request()
{
  sceKernelLockMutex(g_redraw_request_mutex_id, 1, 0);
  uint32_t temp = g_redraw_request;
  sceKernelUnlockMutex(g_redraw_request_mutex_id, 1);
  return temp;
}

void set_redraw_request(uint32_t value)
{
  sceKernelLockMutex(g_redraw_request_mutex_id, 1, 0);
  g_redraw_request = value;
  sceKernelUnlockMutex(g_redraw_request_mutex_id, 1);
}

//---

SceUID g_file_position_mutex_id = -1;

uint32_t g_file_position = 0;

uint32_t get_file_position()
{
  sceKernelLockMutex(g_file_position_mutex_id, 1, 0);
  uint32_t temp = g_file_position;
  sceKernelUnlockMutex(g_file_position_mutex_id, 1);
  return temp;
}

void set_file_position(uint32_t value)
{
  sceKernelLockMutex(g_file_position_mutex_id, 1, 0);
  g_file_position = value;
  sceKernelUnlockMutex(g_file_position_mutex_id, 1);
}

//---

SceUID g_max_file_position_mutex_id = -1;

uint32_t g_max_file_position = 0;

uint32_t get_max_file_position()
{
  sceKernelLockMutex(g_max_file_position_mutex_id, 1, 0);
  uint32_t temp = g_max_file_position;
  sceKernelUnlockMutex(g_max_file_position_mutex_id, 1);
  return temp;
}

void set_max_file_position(uint32_t value)
{
  sceKernelLockMutex(g_max_file_position_mutex_id, 1, 0);
  g_max_file_position = value;
  sceKernelUnlockMutex(g_max_file_position_mutex_id, 1);
}

//---

SceUID g_selected_iso_mutex_id = -1;

char g_selected_iso[256] = {0};

void get_selected_iso(char* result)
{
  sceKernelLockMutex(g_selected_iso_mutex_id, 1, 0);
  memset(result, 0, 256);
  strncpy(result, g_selected_iso, 256);
  sceKernelUnlockMutex(g_selected_iso_mutex_id, 1);
}

void set_selected_iso(const char* value)
{
  sceKernelLockMutex(g_selected_iso_mutex_id, 1, 0);
  strncpy(g_selected_iso, value, 256);
  sceKernelUnlockMutex(g_selected_iso_mutex_id, 1);
}

void clear_selected_iso()
{
  sceKernelLockMutex(g_selected_iso_mutex_id, 1, 0);
  memset(g_selected_iso, 0, 256);
  sceKernelUnlockMutex(g_selected_iso_mutex_id, 1);
}

//---

SceUID g_driver_mode_mutex_id = -1;

uint32_t g_driver_mode = DRIVER_MODE_PHYSICAL_MMC;

uint32_t get_driver_mode()
{
  sceKernelLockMutex(g_driver_mode_mutex_id, 1, 0);
  uint32_t temp = g_driver_mode;
  sceKernelUnlockMutex(g_driver_mode_mutex_id, 1);
  return temp;
}

void set_driver_mode(uint32_t value)
{
  sceKernelLockMutex(g_driver_mode_mutex_id, 1, 0);
  g_driver_mode = value;
  sceKernelUnlockMutex(g_driver_mode_mutex_id, 1);
}

//---

SceUID g_insertion_state_mutex_id = -1;

uint32_t g_insertion_state = INSERTION_STATE_REMOVED;

uint32_t get_insertion_state()
{
  sceKernelLockMutex(g_insertion_state_mutex_id, 1, 0);
  uint32_t temp = g_insertion_state;
  sceKernelUnlockMutex(g_insertion_state_mutex_id, 1);
  return temp;
}

void set_insertion_state(uint32_t value)
{
  sceKernelLockMutex(g_insertion_state_mutex_id, 1, 0);
  g_insertion_state = value;
  sceKernelUnlockMutex(g_insertion_state_mutex_id, 1);
}

//---

SceUID g_content_id_mutex_id = -1;

char g_content_id[SFO_MAX_STR_VALUE_LEN] = {0};

void get_content_id(char* result)
{
  sceKernelLockMutex(g_content_id_mutex_id, 1, 0);
  memset(result, 0, SFO_MAX_STR_VALUE_LEN);
  strncpy(result, g_content_id, SFO_MAX_STR_VALUE_LEN);
  sceKernelUnlockMutex(g_content_id_mutex_id, 1);
}

void set_content_id(const char* value)
{
  sceKernelLockMutex(g_content_id_mutex_id, 1, 0);
  strncpy(g_content_id, value, SFO_MAX_STR_VALUE_LEN);
  sceKernelUnlockMutex(g_content_id_mutex_id, 1);
}

void clear_content_id()
{
  sceKernelLockMutex(g_content_id_mutex_id, 1, 0);
  memset(g_content_id, 0, SFO_MAX_STR_VALUE_LEN);
  sceKernelUnlockMutex(g_content_id_mutex_id, 1);
}

//---

#define DUMP_STATE_POLL_START 1
#define DUMP_STATE_POLL_STOP 0

SceUID g_dump_state_poll_running_state_mutex_id = -1;

uint32_t g_dump_state_poll_running_state = 0;

uint32_t get_dump_state_poll_running_state()
{
  sceKernelLockMutex(g_dump_state_poll_running_state_mutex_id, 1, 0);
  uint32_t temp = g_dump_state_poll_running_state;
  sceKernelUnlockMutex(g_dump_state_poll_running_state_mutex_id, 1);
  return temp;
}

void set_dump_state_poll_running_state(uint32_t value)
{
  sceKernelLockMutex(g_dump_state_poll_running_state_mutex_id, 1, 0);
  g_dump_state_poll_running_state = value;
  sceKernelUnlockMutex(g_dump_state_poll_running_state_mutex_id, 1);
}

//---

SceUID g_total_sectors_mutex_id = -1;

uint32_t g_total_sectors = 0;

uint32_t get_total_sectors()
{
  sceKernelLockMutex(g_total_sectors_mutex_id, 1, 0);
  uint32_t temp = g_total_sectors;
  sceKernelUnlockMutex(g_total_sectors_mutex_id, 1);
  return temp;
}

void set_total_sectors(uint32_t value)
{
  sceKernelLockMutex(g_total_sectors_mutex_id, 1, 0);
  g_total_sectors = value;
  sceKernelUnlockMutex(g_total_sectors_mutex_id, 1);
}

//---

SceUID g_progress_sectors_mutex_id = -1;

uint32_t g_progress_sectors = 0;

uint32_t get_progress_sectors()
{
  sceKernelLockMutex(g_progress_sectors_mutex_id, 1, 0);
  uint32_t temp = g_progress_sectors;
  sceKernelUnlockMutex(g_progress_sectors_mutex_id, 1);
  return temp;
}

void set_progress_sectors(uint32_t value)
{
  sceKernelLockMutex(g_progress_sectors_mutex_id, 1, 0);
  g_progress_sectors = value;
  sceKernelUnlockMutex(g_progress_sectors_mutex_id, 1);
}

//---

SceUID g_physical_ins_state_mutex_id = -1;

uint32_t g_physical_ins_state = 0;

uint32_t get_physical_ins_state()
{
  sceKernelLockMutex(g_physical_ins_state_mutex_id, 1, 0);
  uint32_t temp = g_physical_ins_state;
  sceKernelUnlockMutex(g_physical_ins_state_mutex_id, 1);
  return temp;
}

void set_physical_ins_state(uint32_t value)
{
  sceKernelLockMutex(g_physical_ins_state_mutex_id, 1, 0);
  g_physical_ins_state = value;
  sceKernelUnlockMutex(g_physical_ins_state_mutex_id, 1);
}

//##########################################################################################

//insert iso
int SCE_CTRL_START_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_START\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    //insertion only applies to virtual modes
    uint32_t d_mode = get_driver_mode();
    if((d_mode == DRIVER_MODE_VIRTUAL_MMC) || (d_mode == DRIVER_MODE_VIRTUAL_SD))
    {
      //insertion only applies if iso is selected
      char sel_iso[256];
      get_selected_iso(sel_iso);
      if(strnlen(sel_iso, 256) > 0)
      {
        //get prev state
        uint32_t prev_state = get_insertion_state();

        //set current state
        set_insertion_state(INSERTION_STATE_INSERTED);

        //insert card only if state has changed
        if(prev_state != get_insertion_state())
        {
          //insert card using kernel function
          insert_card();

          //wait 2 seconds for insertion
          sceKernelDelayThread(2 * 1000 * 1000);

          //redraw screen
          set_redraw_request(1);
        }
      }
    }
  }

  return 0;
}

//remove iso
int SCE_CTRL_SELECT_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_SELECT\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    //insertion only applies to virtual modes
    uint32_t d_mode = get_driver_mode();
    if((d_mode == DRIVER_MODE_VIRTUAL_MMC) || (d_mode == DRIVER_MODE_VIRTUAL_SD))
    {
      //insertion only applies if iso is selected
      char sel_iso[256];
      get_selected_iso(sel_iso);
      if(strnlen(sel_iso, 256) > 0)
      {
        //get prev state
        uint32_t prev_state = get_insertion_state();

        //set current state
        set_insertion_state(INSERTION_STATE_REMOVED);

        //insert card only if state has changed
        if(prev_state != get_insertion_state())
        {
          //remove card using kernel function
          remove_card();

          //wait 2 seconds for removal
          sceKernelDelayThread(2 * 1000 * 1000);

          //redraw screen
          set_redraw_request(1);
        }
      }
    }
  }

  return 0;
}

//navigate files
int SCE_CTRL_UP_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_UP\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    //update max file position just in case new files were added to directory (for example during dump)
    set_max_file_position(get_dir_max_file_pos(ISO_ROOT_DIRECTORY));

    sceKernelLockMutex(g_file_position_mutex_id, 1, 0);
    if(g_file_position > 0)
      g_file_position--;
    sceKernelUnlockMutex(g_file_position_mutex_id, 1);

    set_redraw_request(1);
  }

  return 0;
}

//navigate files
int SCE_CTRL_DOWN_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_DOWN\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    //update max file position just in case new files were added to directory (for example during dump)
    set_max_file_position(get_dir_max_file_pos(ISO_ROOT_DIRECTORY));

    sceKernelLockMutex(g_file_position_mutex_id, 1, 0);
    if(g_file_position < get_max_file_position())
      g_file_position++;
    sceKernelUnlockMutex(g_file_position_mutex_id, 1);

    set_redraw_request(1);
  }

  return 0;
}

int select_driver_mode(uint32_t prev_mode, uint32_t new_mode)
{
  //execute deinit based on previous mode
  switch(prev_mode)
  {
    case DRIVER_MODE_PHYSICAL_MMC:
    {
      deinitialize_physical_mmc();
    }
    break;
    case DRIVER_MODE_VIRTUAL_MMC:
    {
      deinitialize_virtual_mmc();
    }
    break;
    case DRIVER_MODE_PHYSICAL_SD:
    {
      deinitialize_physical_sd();
    }
    break;
    case DRIVER_MODE_VIRTUAL_SD:
    {
      deinitialize_virtual_sd();
    }
    break;
    default:
    {
    }
    break;
  }

  //execute init based on current mode
  switch(new_mode)
  {
    case DRIVER_MODE_PHYSICAL_MMC:
    {
      initialize_physical_mmc();
    }
    break;
    case DRIVER_MODE_VIRTUAL_MMC:
    {
      initialize_virtual_mmc();
    }
    break;
    case DRIVER_MODE_PHYSICAL_SD:
    {
      initialize_physical_sd();
    }
    break;
    case DRIVER_MODE_VIRTUAL_SD:
    {
      initialize_virtual_sd();
    }
    break;
    default:
    {
    }
    break;
  }

  return 0;
}

int get_gro0_sfo_path(char* sfo_path)
{
  memset(sfo_path, 0, 256);

  SceUID dirId = sceIoDopen("gro0:app");
  if(dirId >= 0)
  {
    int res = 0;
    do
    {
      SceIoDirent dir;
      memset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dirId, &dir);
      if(res > 0)
      {
        if(SCE_S_ISDIR(dir.d_stat.st_mode))
        {
          strncpy(sfo_path, "gro0:app/", 256);
          strncat(sfo_path, dir.d_name, 256);
          strncat(sfo_path, "/sce_sys/param.sfo", 256);
          break;
        }
      }
    }
    while(res > 0);

    sceIoDclose(dirId);
  }

  return 0;
}

int get_current_content_id_internal(char* content_id)
{
  //get path to sfo file in gro0 partition
  char sfo_path[256];
  get_gro0_sfo_path(sfo_path);

  //verify path length
  if(strnlen(sfo_path, 256) == 0)
    return -1;

  //verify that sfo file exists
  SceUID fd = sceIoOpen(sfo_path, SCE_O_RDONLY, 0777);
  if(fd < 0)
    return -1;

  sceIoClose(fd);
  
  //parse file
  if(init_sfo_structures(sfo_path) < 0)
    return -1;

  if(get_utf8_value(sfo_path, SFO_CONTENT_ID_KEY, content_id, SFO_MAX_STR_VALUE_LEN) < 0)
    return -1;

  if(strnlen(content_id, SFO_MAX_STR_VALUE_LEN) == 0)
    return -1;  

  return 0;
}

//select driver mode
int SCE_CTRL_RIGHT_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_RIGHT\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    uint32_t prev_driver_mode = get_driver_mode();

    int phys_ins_state = get_physical_ins_state();

    //forbid swithing states when there is a game card physically inserted
    //this should help to avoid potential conflicts when forgetting to remove physical card
    //and trying to insert virtual card
    if(!(prev_driver_mode == DRIVER_MODE_PHYSICAL_MMC && phys_ins_state > 0) &&
       !(prev_driver_mode == DRIVER_MODE_PHYSICAL_SD && phys_ins_state > 0))
    {
      sceKernelLockMutex(g_driver_mode_mutex_id, 1, 0);
      
      //check overflow condition
      if(g_driver_mode == DRIVER_MODE_VIRTUAL_SD)
        g_driver_mode = DRIVER_MODE_PHYSICAL_MMC;
      else
        g_driver_mode++;
  
      //remove card and deselect iso if previous mode was virtual and card was inserted
      if((prev_driver_mode == DRIVER_MODE_VIRTUAL_MMC) || (prev_driver_mode == DRIVER_MODE_VIRTUAL_SD))
      {
        //get current ins state
        if(get_insertion_state() == INSERTION_STATE_INSERTED)
        {
          //set new ins state
          set_insertion_state(INSERTION_STATE_REMOVED);

          //remove card using kernel function
          remove_card();

          //wait 2 seconds for removal
          sceKernelDelayThread(2 * 1000 * 1000);
        }
  
        //clear iso variable
        clear_selected_iso();

        //deselect iso in kernel
        clear_iso_path();
      }
  
      sceKernelUnlockMutex(g_driver_mode_mutex_id, 1);
  
      //change driver mode only after removing the card and deselecting iso
      select_driver_mode(prev_driver_mode, get_driver_mode());
  
      set_redraw_request(1);
    }
  }

  return 0;
}

//select driver mode
int SCE_CTRL_LEFT_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_LEFT\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    uint32_t prev_driver_mode = get_driver_mode();

    int phys_ins_state = get_physical_ins_state();
    
    //forbid swithing states when there is a game card physically inserted
    //this should help to avoid potential conflicts when forgetting to remove physical card
    //and trying to insert virtual card
    if(!(prev_driver_mode == DRIVER_MODE_PHYSICAL_MMC && phys_ins_state > 0) &&
       !(prev_driver_mode == DRIVER_MODE_PHYSICAL_SD && phys_ins_state > 0))
    {
      sceKernelLockMutex(g_driver_mode_mutex_id, 1, 0);
      
      //check underflow condition
      if(g_driver_mode == DRIVER_MODE_PHYSICAL_MMC)
        g_driver_mode = DRIVER_MODE_VIRTUAL_SD;
      else
        g_driver_mode--;
  
      //remove card and deselect iso if previous mode was virtual and card was inserted
      if((prev_driver_mode == DRIVER_MODE_VIRTUAL_MMC) || (prev_driver_mode == DRIVER_MODE_VIRTUAL_SD))
      {
        //get current ins state
        if(get_insertion_state() == INSERTION_STATE_INSERTED)
        {
          //set new ins state
          set_insertion_state(INSERTION_STATE_REMOVED);

          //remove card using kernel function
          remove_card();

          //wait 2 seconds for removal
          sceKernelDelayThread(2 * 1000 * 1000);
        }
  
        //clear iso variable
        clear_selected_iso();

        //deselect iso in kernel
        clear_iso_path();
      }
  
      sceKernelUnlockMutex(g_driver_mode_mutex_id, 1);
  
      //change driver mode only after removing the card and deselecting iso
      select_driver_mode(prev_driver_mode, get_driver_mode());
  
      set_redraw_request(1);
    }    
  }

  return 0;
}

int SCE_CTRL_LTRIGGER_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_LTRIGGER\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
  }

  return 0;
}

int SCE_CTRL_RTRIGGER_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_RTRIGGER\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
  }

  return 0;
}

//exit app
int SCE_CTRL_TRIANGLE_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_TRIANGLE\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    //set running var - this will be picked by main_draw_loop
    set_app_running(0);
  }

  return 0;
}

//select iso
int SCE_CTRL_CIRCLE_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_CIRCLE\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    //iso execution only applies to virtual mode
    uint32_t d_mode = get_driver_mode();
    if((d_mode == DRIVER_MODE_VIRTUAL_MMC) || (d_mode == DRIVER_MODE_VIRTUAL_SD))
    {
      //get currently selected iso
      char filepath[256];
      int found = get_dir_filename_at_pos(g_current_directory, get_file_position(), filepath);
      if(found >= 0)
      {
        //get previous iso to temp var
        char prev_iso[256];
        get_selected_iso(prev_iso);

        //set new iso
        set_selected_iso(filepath);
        
        //check if selection has changed
        if(strncmp(prev_iso, filepath, 256) != 0)
        {
          //remove previous card if it was inserted
          if(get_insertion_state() == INSERTION_STATE_INSERTED)
          {
            //set insertion var
            set_insertion_state(INSERTION_STATE_REMOVED);

            //remove card using kernel function
            remove_card();

            //wait 2 seconds for removal
            sceKernelDelayThread(2 * 1000 * 1000);
          }

          //construct path to new iso
          char full_path[256];
          memset(full_path, 0, 256);
          strncpy(full_path, g_current_directory, 256);
          strncat(full_path, "/", 256);
          strncat(full_path, filepath, 256);

          //set iso path var
          set_iso_path(full_path);

          //redraw screen
          set_redraw_request(1);
        }
      }
    }
  }

  return 0;
}

//------------------------------------------

int dump_status_poll_thread_internal(SceSize args, void* argp)
{
  uint32_t prev_total_sectors = -1;
  uint32_t prev_progress_sectors = -1;

  while(1)
  {
    //wait 1 second
    sceKernelDelayThread(1 * 1000 * 1000);

    //get stats from kernel
    uint32_t total_sectors = dump_mmc_get_total_sectors(); 
    uint32_t progress_sectors = dump_mmc_get_progress_sectors();

    //set to local vars
    set_total_sectors(total_sectors);
    set_progress_sectors(progress_sectors);

    if(prev_total_sectors != total_sectors || prev_progress_sectors != progress_sectors)
    {
      //redraw screen
      set_redraw_request(1);
    }
    
    prev_total_sectors = total_sectors;
    prev_progress_sectors = progress_sectors;
    
    //check if cancel was requested
    uint32_t rn_state = get_dump_state_poll_running_state();
    if(rn_state == DUMP_STATE_POLL_STOP)
    {
      set_total_sectors(0);
      set_progress_sectors(0);

      set_redraw_request(1);
      return 0;
    }

    //check if dump has finished
    if(total_sectors == progress_sectors)
    {
      set_total_sectors(0);
      set_progress_sectors(0);

      set_redraw_request(1);
      return 0;
    }
  }

  return 0;
}

int dump_status_poll_thread(SceSize args, void* argp)
{
  set_dump_state_poll_running_state(DUMP_STATE_POLL_START);

  dump_status_poll_thread_internal(args, argp);

  set_dump_state_poll_running_state(DUMP_STATE_POLL_STOP);

  return 0;
}

SceUID g_dump_status_poll_thread_id = -1;

int initialize_dump_status_poll_threading()
{
  g_dump_status_poll_thread_id = sceKernelCreateThread("dump_status_poll", dump_status_poll_thread, 0x40, 0x1000, 0, 0, 0);
  
  if(g_dump_status_poll_thread_id >= 0)
    sceKernelStartThread(g_dump_status_poll_thread_id, 0, 0);

  return 0;
}

int deinitialize_dump_status_poll_threading()
{
  if(g_dump_status_poll_thread_id >= 0)
  {
    int waitRet = 0;
    sceKernelWaitThreadEnd(g_dump_status_poll_thread_id, &waitRet, 0);
  
    sceKernelDeleteThread(g_dump_status_poll_thread_id);
    g_dump_status_poll_thread_id = -1;
  }

  return 0;
}

//------------------------------------------

int check_insert_update_content_id(int prev_ins_state)
{
  int ins_state = 0;

  //this should only apply in mmc physical mode because 
  //it is meaningless to dump in physical sd state
  //and it should be forbidden in virtual modes

  uint32_t d_mode = get_driver_mode();
  
  if(d_mode == DRIVER_MODE_PHYSICAL_MMC)
  {
    //get state from driver
    ins_state = get_phys_ins_state();

    //save to user var
    set_physical_ins_state(ins_state);

    //if state has changed
    if(ins_state != prev_ins_state)
    {
      //if card is inserted
      if(ins_state > 0)
      {
        //when card is inserted - insertion is reported immediately
        //however it will take time to initialize the card, do handshake and mount the file system
        //since we will be reading content id from the card - we need to wait at least some time
        //before trying to read. we should also retry on fail
        
        int nRetries = 0;
        while(nRetries < 5)
        {
          sceKernelDelayThread(1 * 1000 * 1000);

          //try to get content id
          char cnt_id[SFO_MAX_STR_VALUE_LEN];
          int res = get_current_content_id_internal(cnt_id);
          if(res >= 0)
          {
            //save new content id
            set_content_id(cnt_id);

            //redraw screen
            set_redraw_request(1);

            break;
          }

          nRetries++;
        }
      }
      else
      {
        //clear current id
        clear_content_id();
        
        //redraw screen
        set_redraw_request(1);
      }
    }
  }

  return ins_state;
}

int insert_status_poll_thread(SceSize args, void* argp)
{
  //check the state upon start - maybe card is already inserted
  int prev_ins_state = check_insert_update_content_id(0);

  while(get_app_running() > 0)
  {
    //wait 1 second
    sceKernelDelayThread(1 * 1000 * 1000);

    //check physical insert state - poll
    prev_ins_state = check_insert_update_content_id(prev_ins_state);
  }

  return 0;
}

SceUID g_insert_status_poll_thread_id = -1;

int initialize_insert_status_poll_threading()
{
  g_insert_status_poll_thread_id = sceKernelCreateThread("insert_status_poll", insert_status_poll_thread, 0x40, 0x1000, 0, 0, 0);

  if(g_insert_status_poll_thread_id >= 0)
    sceKernelStartThread(g_insert_status_poll_thread_id, 0, 0);

  return 0;
}

int deinitialize_insert_status_poll_threading()
{
  if(g_insert_status_poll_thread_id >= 0)
  {
    int waitRet = 0;
    sceKernelWaitThreadEnd(g_insert_status_poll_thread_id, &waitRet, 0);
  
    sceKernelDeleteThread(g_insert_status_poll_thread_id);
    g_insert_status_poll_thread_id = -1;
  }

  return 0;
}

//---

int SCE_CTRL_CROSS_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_CROSS\n");

  //forbid to press any buttons during dump except for square (which is cancel)
  //also protects from re entering dump start state
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_START)
  {
    uint32_t d_mode = get_driver_mode();

    // dumping is only allowed in physical mmc mode
    if(d_mode == DRIVER_MODE_PHYSICAL_MMC)
    {
      //get content id
      char cnt_id[SFO_MAX_STR_VALUE_LEN];
      get_content_id(cnt_id);

      //check if content id is set
      if(strnlen(cnt_id, 256) > 0)
      {
        //start dumping the card - this will start new thread in kernel
        char full_path[256];
        strncpy(full_path, g_current_directory, 256);
        strncat(full_path, "/", 256);
        strncat(full_path, cnt_id, 256);
        strncat(full_path, ".psv", 256);
        
        //start dump process in kernel
        dump_mmc_card_start(full_path);

        //redraw screen
        set_redraw_request(1);

        //start polling only after redraw request
        //since thread will be requesting redraw as well
        
        //if previous dump status poll operation was not canceled - status poll thread will not be deinitialized
        deinitialize_dump_status_poll_threading();
      
        //initialize new thread
        initialize_dump_status_poll_threading();
      }
    }
  }

  return 0;
}

int SCE_CTRL_SQUARE_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_SQUARE\n");

  //forbid to press square (dump cancel) when not in the process of dumping
  //also protects from re entering dump cancel state
  uint32_t rn_state = get_dump_state_poll_running_state();
  if(rn_state != DUMP_STATE_POLL_STOP)
  {
    uint32_t d_mode = get_driver_mode();
    
    // dumping is only allowed in physical mmc mode
    if(d_mode == DRIVER_MODE_PHYSICAL_MMC)
    {
      //stop dump process in kernel
      dump_mmc_card_cancel();

      //redraw screen
      set_redraw_request(1);

      //stop polling dump state

      //indicate that we are entering cancel state (this will stop polling thread)
      set_dump_state_poll_running_state(DUMP_STATE_POLL_STOP);

      //deinitialize status polling thread
      deinitialize_dump_status_poll_threading();
    }
  }

  return 0;
}

//##########################################################################################

int main_ctrl_loop(SceSize args, void* argp)
{
  sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	
	while(get_app_running() > 0)
  {
    uint32_t buttons = readInput();

    if(buttons & SCE_CTRL_SELECT)
    {
      SCE_CTRL_SELECT_callback();
    }
    else if(buttons & SCE_CTRL_START)
    {
      SCE_CTRL_START_callback();
    }
    else if(buttons & SCE_CTRL_UP)
    {
      SCE_CTRL_UP_callback();
    }
    else if(buttons & SCE_CTRL_RIGHT)
    {
      SCE_CTRL_RIGHT_callback();
    }
    else if(buttons & SCE_CTRL_DOWN)
    {
      SCE_CTRL_DOWN_callback();
    }
    else if(buttons & SCE_CTRL_LEFT)
    {
      SCE_CTRL_LEFT_callback();
    }
    else if(buttons & SCE_CTRL_LTRIGGER)
    {
      SCE_CTRL_LTRIGGER_callback();
    }
    else if(buttons & SCE_CTRL_RTRIGGER)
    {
      SCE_CTRL_RTRIGGER_callback();
    }
    else if(buttons & SCE_CTRL_TRIANGLE)
    {
      SCE_CTRL_TRIANGLE_callback();
    }
    else if(buttons & SCE_CTRL_CIRCLE)
    {
      SCE_CTRL_CIRCLE_callback();
    }
    else if(buttons & SCE_CTRL_CROSS)
    {
      SCE_CTRL_CROSS_callback();
    }
    else if(buttons & SCE_CTRL_SQUARE)
    {
      SCE_CTRL_SQUARE_callback();
    }
	}

  return 0;
}

SceUInt64 old_frame_time = 0;
SceUInt64 current_frame_time = 0;

SceUInt64 time_diff(SceUInt64 old, SceUInt64 new)
{
  if(old > new)
    return (0xFFFFFFFF - old) + new;
  else
    return new - old;
}

int get_color_from_poll_state(uint32_t rn_state, int active, int inactive)
{
  if(rn_state == DUMP_STATE_POLL_STOP)
    return active;
  else
    return inactive;
}

int draw_dir(char* path)
{
  psvDebugScreenClear(COLOR_BLACK);

  psvDebugScreenPrintf("\e[9%im welcome to psvgamesd\n", 7);

  uint32_t rn_state = get_dump_state_poll_running_state();

  psvDebugScreenPrintf("\e[9%im directory: %s\n", get_color_from_poll_state(rn_state, 7, 0), path);

  char sel_driver_mode[256];
  driver_mode_to_name(get_driver_mode(), sel_driver_mode);
  psvDebugScreenPrintf("\e[9%im driver mode: %s\n", get_color_from_poll_state(rn_state, 7, 0), sel_driver_mode);

  uint32_t d_mode = get_driver_mode();

  if(d_mode == DRIVER_MODE_PHYSICAL_MMC)
  {
    char cnt_id[SFO_MAX_STR_VALUE_LEN];
    get_content_id(cnt_id);

    if(strnlen(cnt_id, SFO_MAX_STR_VALUE_LEN) > 0)
    {
      psvDebugScreenPrintf("\e[9%im content id: %s\n", get_color_from_poll_state(rn_state, 7, 0), cnt_id);
    }
    else
    {
      psvDebugScreenPrintf("\e[9%im content id: %s\n", get_color_from_poll_state(rn_state, 7, 0), "Game Card is not inserted");
    }
  }
  else
  {
    psvDebugScreenPrintf("\e[9%im content id:\n", 0);
  }

  if(d_mode == DRIVER_MODE_PHYSICAL_MMC)
  {
    if(rn_state == DUMP_STATE_POLL_START)
    {
      uint32_t total_sectors = get_total_sectors();
      uint32_t progress_sectors = get_progress_sectors();

      psvDebugScreenPrintf("\e[9%im dump progress: %x | %x\n", 7, progress_sectors, total_sectors);
    }
    else
    {
      psvDebugScreenPrintf("\e[9%im dump progress:\n", 0);
    }
  }
  else
  {
    psvDebugScreenPrintf("\e[9%im dump progress:\n", 0);
  }

  if(d_mode == DRIVER_MODE_VIRTUAL_MMC || d_mode == DRIVER_MODE_VIRTUAL_SD)
  {
    char sel_iso[256];
    get_selected_iso(sel_iso);
    if(strnlen(sel_iso, 256) > 0)
    {
      psvDebugScreenPrintf("\e[9%im selected iso: %s\n", get_color_from_poll_state(rn_state, 7, 0), sel_iso);

      if(get_insertion_state() == INSERTION_STATE_INSERTED)
      {
        psvDebugScreenPrintf("\e[9%im insertion state: %s\n", get_color_from_poll_state(rn_state, 7, 0), "inserted");
      }
      else
      {
        psvDebugScreenPrintf("\e[9%im insertion state: %s\n", get_color_from_poll_state(rn_state, 7, 0), "removed");
      }
    }
    else
    {
      psvDebugScreenPrintf("\e[9%im selected iso:\n", get_color_from_poll_state(rn_state, 7, 0));
      psvDebugScreenPrintf("\e[9%im insertion state:\n", 0);
    }
  }
  else
  {
    psvDebugScreenPrintf("\e[9%im selected iso:\n", 0);
    psvDebugScreenPrintf("\e[9%im insertion state:\n", 0);
  }

  psvDebugScreenPrintf("\n");

  SceUID dirId = sceIoDopen(path);
  if(dirId >= 0)
  {
    uint32_t cur_file_position = 0;

    int res = 0;
    do
    {
      SceIoDirent dir;
      memset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dirId, &dir);
      if(res > 0)
      {
        if(SCE_S_ISREG(dir.d_stat.st_mode))
        {
          if(cur_file_position == get_file_position())
          {
            psvDebugScreenPrintf("\e[9%im %s\n", get_color_from_poll_state(rn_state, 2, 0), dir.d_name);
          }
          else
          {
            psvDebugScreenPrintf("\e[9%im %s\n", get_color_from_poll_state(rn_state, 7, 0), dir.d_name);
          }
          
          cur_file_position++;
        }
      }
    }
    while(res > 0);

    sceIoDclose(dirId);
  }

  return 0;
}

int main_draw_loop()
{
  while(get_app_running())
  {
    old_frame_time = sceKernelGetSystemTimeWide();

    do
    {
      current_frame_time = sceKernelGetSystemTimeWide();
    }
    while(time_diff(old_frame_time, current_frame_time) < 33333); //30 fps

    if(get_redraw_request())
    {
      draw_dir(g_current_directory);
      set_redraw_request(0);
    }
  }

  psvDebugScreenPrintf("exiting...\n");

  return 0;
}

//##########################################################################################

SceUID g_ctrl_thread_id = -1;

int initialize_threading()
{
  g_app_running_mutex_id = sceKernelCreateMutex("app_running", 0, 0, 0);

  g_redraw_request_mutex_id = sceKernelCreateMutex("redraw_request", 0, 0, 0);

  g_file_position_mutex_id = sceKernelCreateMutex("file_position", 0, 0, 0);

  g_max_file_position_mutex_id = sceKernelCreateMutex("max_file_position", 0, 0, 0);

  g_selected_iso_mutex_id = sceKernelCreateMutex("selected_iso", 0, 0, 0);

  g_insertion_state_mutex_id = sceKernelCreateMutex("insertion_state", 0, 0, 0);

  g_content_id_mutex_id = sceKernelCreateMutex("content_id", 0, 0, 0);

  g_dump_state_poll_running_state_mutex_id = sceKernelCreateMutex("dump_state_poll_running_state", 0, 0, 0);

  g_total_sectors_mutex_id = sceKernelCreateMutex("total_sectors_mutex", 0, 0, 0);
  
  g_progress_sectors_mutex_id = sceKernelCreateMutex("progress_sectors_mutex", 0, 0, 0);

  g_physical_ins_state_mutex_id = sceKernelCreateMutex("physical_ins_state_mutex", 0, 0, 0);

  g_ctrl_thread_id = sceKernelCreateThread("ctrl", main_ctrl_loop, 0x40, 0x1000, 0, 0, 0);

  if(g_ctrl_thread_id >= 0)
    sceKernelStartThread(g_ctrl_thread_id, 0, 0);

  return 0;
}

int deinitialize_threading()
{
  //deinitialize dump status poll thread if last dump status poll was successfull
  //if it was canceled - it will be already deinitialized
  deinitialize_dump_status_poll_threading();

  sceKernelDeleteMutex(g_app_running_mutex_id);
  g_app_running_mutex_id = -1;

  sceKernelDeleteMutex(g_redraw_request_mutex_id);
  g_redraw_request_mutex_id = -1;

  sceKernelDeleteMutex(g_file_position_mutex_id);
  g_file_position_mutex_id = -1;

  sceKernelDeleteMutex(g_max_file_position_mutex_id);
  g_max_file_position_mutex_id = -1;

  sceKernelDeleteMutex(g_selected_iso_mutex_id);
  g_selected_iso_mutex_id = -1;

  sceKernelDeleteMutex(g_insertion_state_mutex_id);
  g_insertion_state_mutex_id = -1;

  sceKernelDeleteMutex(g_content_id_mutex_id);
  g_content_id_mutex_id = -1;

  sceKernelDeleteMutex(g_dump_state_poll_running_state_mutex_id);
  g_dump_state_poll_running_state_mutex_id = -1;

  sceKernelDeleteMutex(g_total_sectors_mutex_id);
  g_total_sectors_mutex_id = -1;

  sceKernelDeleteMutex(g_progress_sectors_mutex_id);
  g_progress_sectors_mutex_id = -1;

  sceKernelDeleteMutex(g_physical_ins_state_mutex_id);
  g_physical_ins_state_mutex_id = -1;

  if(g_ctrl_thread_id >= 0)
  {
    int waitRet = 0;
    sceKernelWaitThreadEnd(g_ctrl_thread_id, &waitRet, 0);
  
    sceKernelDeleteThread(g_ctrl_thread_id);
    g_ctrl_thread_id = -1;
  }
  
  return 0;
}

int set_dir(const char* path)
{  
  strncpy(g_current_directory, path, 256);

  set_max_file_position(get_dir_max_file_pos(path));
  set_file_position(0);

  return 0;
}

int set_default_state()
{
  set_app_running(1);
  set_redraw_request(1);

  set_dir(ISO_ROOT_DIRECTORY);

  clear_selected_iso();

  set_driver_mode(DRIVER_MODE_PHYSICAL_MMC);
  set_insertion_state(INSERTION_STATE_REMOVED);

  clear_content_id();

  set_dump_state_poll_running_state(DUMP_STATE_POLL_STOP);
  set_total_sectors(0);
  set_progress_sectors(0);

  set_physical_ins_state(0);

  return 0;
}

int set_state_from_ctx(const psvgamesd_ctx* ctx)
{
  set_app_running(1);
  set_redraw_request(1);

  set_dir(ctx->current_directory);

  set_selected_iso(ctx->selected_iso);

  set_driver_mode(ctx->driver_mode);
  set_insertion_state(ctx->insertion_state);

  clear_content_id();

  set_dump_state_poll_running_state(DUMP_STATE_POLL_STOP);
  set_total_sectors(0);
  set_progress_sectors(0);

  set_physical_ins_state(0);

  return 0;
}

//app_running - should not be saved
//redraw_request - should not be saved
//max_file_position position - should be updated using current_directory
//file_position - should be updated using current_directory
//current_directory - should be saved
//selected_iso - should be saved
//driver_mode - should be saved
//insertion_state - should be saved
//content_id - should be automatically updated by check_insert_update_content_id
//dump_state_poll_running_state - should not be saved because user can not quit app while dumping
//total_sectors - should not be saved because user can not quit app while dumping
//progress_sectors - should not be saved because user can not quit app while dumping
//physical_ins_state - should be automatically updated by check_insert_update_content_id

int save_state_to_kernel()
{
  psvgamesd_ctx ctx;
  memset(&ctx, 0, sizeof(psvgamesd_ctx));

  strncpy(ctx.current_directory, g_current_directory, 256);
  get_selected_iso(ctx.selected_iso);
  ctx.driver_mode = get_driver_mode();
  ctx.insertion_state = get_insertion_state();

  save_psvgamesd_state(&ctx);

  return 0;
}

int load_state_from_kernel()
{
  psvgamesd_ctx ctx;
  memset(&ctx, 0, sizeof(psvgamesd_ctx));

  int res = load_psvgamesd_state(&ctx);
  if(res < 0)
  {
    //default state should be set only if we do
    //not have previous state saved in kernel
    set_default_state(); 
  }
  else
  {
    //if previous state exists - restore it
    set_state_from_ctx(&ctx);
  }

  return 0;
}

int main(int argc, char *argv[]) 
{
  psvDebugScreenInit();

  sceShellUtilInitEvents(0);

  //allow to exit only with triangle button - lock ps button
  //this is requred because we need to save current state to kernel plugin upon exit
  ps_btn_lock();

  load_state_from_kernel();

  initialize_threading();

  initialize_insert_status_poll_threading();

  main_draw_loop();

  deinitialize_insert_status_poll_threading();

  deinitialize_threading();

  save_state_to_kernel();

  //unlock ps button back upon exit
  ps_btn_unlock();

  sceKernelDelayThread(2 * 1000 * 1000);

  sceKernelExitProcess(0);
  return 0;
}
