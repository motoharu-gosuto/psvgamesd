#include <stdio.h>
#include <malloc.h>

#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>

#include <psvgamesd_api.h>

#include "debugScreen.h"

//---

//original mode
#define DRIVER_MODE_PHYSICAL_MMC 0
//emulate mmc
#define DRIVER_MODE_VIRTUAL_MMC 1
//physical sd card with patches
#define DRIVER_MODE_PHYSICAL_SD 2

#define DRIVER_MODE_NAME_PHYSICAL_MMC "physical mmc"
#define DRIVER_MODE_NAME_VIRTUAL_MMC "virtual mmc"
#define DRIVER_MODE_NAME_PHYSICAL_SD "physical sd"

//---

#define INSERTION_STATE_REMOVED 0
#define INSERTION_STATE_INSERTED 1

//---

char g_current_directory[256] = {0};

int get_dir_max_file_pos(char* path)
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

//---

SceUID g_app_running_mutex_id = 0;

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

SceUID g_redraw_request_mutex_id = 0;

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

SceUID g_file_position_mutex_id = 0;

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

SceUID g_max_file_position_mutex_id = 0;

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

SceUID g_selected_iso_mutex_id = 0;

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

//---

SceUID g_driver_mode_mutex_id = 0;

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

SceUID g_insertion_state_mutex_id = 0;

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

//insert iso
int SCE_CTRL_START_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_START\n");

  if(get_driver_mode() == DRIVER_MODE_VIRTUAL_MMC)
  {
    char sel_iso[256];
    get_selected_iso(sel_iso);
    if(strnlen(sel_iso, 256) > 0)
    {
      uint32_t prev_state = get_insertion_state();

      set_insertion_state(INSERTION_STATE_INSERTED);
      set_redraw_request(1);

      if(prev_state != get_insertion_state())
      {
        insert_card();
      }
    }
  }

  return 0;
}

//remove iso
int SCE_CTRL_SELECT_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_SELECT\n");

  if(get_driver_mode() == DRIVER_MODE_VIRTUAL_MMC)
  {
    char sel_iso[256];
    get_selected_iso(sel_iso);
    if(strnlen(sel_iso, 256) > 0)
    {
      uint32_t prev_state = get_insertion_state();

      set_insertion_state(INSERTION_STATE_REMOVED);
      set_redraw_request(1);

      if(prev_state != get_insertion_state())
      {
        remove_card();
      }
    }
  }

  return 0;
}

//navigate files
int SCE_CTRL_UP_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_UP\n");

  sceKernelLockMutex(g_file_position_mutex_id, 1, 0);
  if(g_file_position > 0)
    g_file_position--;
  sceKernelUnlockMutex(g_file_position_mutex_id, 1);

  set_redraw_request(1);

  return 0;
}

//navigate files
int SCE_CTRL_DOWN_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_DOWN\n");

  sceKernelLockMutex(g_file_position_mutex_id, 1, 0);
  if(g_file_position < get_max_file_position())
    g_file_position++;
  sceKernelUnlockMutex(g_file_position_mutex_id, 1);

  set_redraw_request(1);

  return 0;
}

int select_driver_mode(uint32_t prev_mode, uint32_t new_mode)
{
  if(new_mode != DRIVER_MODE_VIRTUAL_MMC)
  {
    if(get_insertion_state() == INSERTION_STATE_INSERTED)
    {
      remove_card();
    }

    set_iso_path("");
  }

  switch(prev_mode)
  {
    case DRIVER_MODE_PHYSICAL_MMC:
    deinitialize_physical_mmc();
    break;
    case DRIVER_MODE_VIRTUAL_MMC:
    deinitialize_virtual_mmc();
    break;
    case DRIVER_MODE_PHYSICAL_SD:
    deinitialize_physical_sd();
    break;
    default:
    break;
  }

  switch(new_mode)
  {
    case DRIVER_MODE_PHYSICAL_MMC:
    initialize_physical_mmc();
    break;
    case DRIVER_MODE_VIRTUAL_MMC:
    initialize_virtual_mmc();
    break;
    case DRIVER_MODE_PHYSICAL_SD:
    initialize_physical_sd();
    break;
    default:
    break;
  }

  return 0;
}

//select driver mode
int SCE_CTRL_RIGHT_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_RIGHT\n");

  uint32_t prev_driver_mode = get_driver_mode();

  sceKernelLockMutex(g_driver_mode_mutex_id, 1, 0);

  if(g_driver_mode == DRIVER_MODE_PHYSICAL_SD)
    g_driver_mode = DRIVER_MODE_PHYSICAL_MMC;
  else
    g_driver_mode++;

  if(g_driver_mode != DRIVER_MODE_VIRTUAL_MMC)
  {
    if(get_insertion_state() == INSERTION_STATE_INSERTED)
    {
      set_insertion_state(INSERTION_STATE_REMOVED);
    }

    set_selected_iso("");
  }

  sceKernelUnlockMutex(g_driver_mode_mutex_id, 1);

  set_redraw_request(1);

  select_driver_mode(prev_driver_mode, get_driver_mode());

  return 0;
}

//select driver mode
int SCE_CTRL_LEFT_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_LEFT\n");

  uint32_t prev_driver_mode = get_driver_mode();

  sceKernelLockMutex(g_driver_mode_mutex_id, 1, 0);

  if(g_driver_mode == DRIVER_MODE_PHYSICAL_MMC)
    g_driver_mode = DRIVER_MODE_PHYSICAL_SD;
  else
    g_driver_mode--;

  if(g_driver_mode != DRIVER_MODE_VIRTUAL_MMC)
  {
    if(get_insertion_state() == INSERTION_STATE_INSERTED)
    {
      set_insertion_state(INSERTION_STATE_REMOVED);
    }

    set_selected_iso("");
  }

  sceKernelUnlockMutex(g_driver_mode_mutex_id, 1);

  set_redraw_request(1);

  select_driver_mode(prev_driver_mode, get_driver_mode());

  return 0;
}

int SCE_CTRL_LTRIGGER_callback()
{
  psvDebugScreenPrintf("psvgamesd: SCE_CTRL_LTRIGGER\n");
  return 0;
}

int SCE_CTRL_RTRIGGER_callback()
{
  psvDebugScreenPrintf("psvgamesd: SCE_CTRL_RTRIGGER\n");
  return 0;
}

//exit app
int SCE_CTRL_TRIANGLE_callback()
{
  psvDebugScreenPrintf("psvgamesd: SCE_CTRL_TRIANGLE\n");

  set_app_running(0);

  return 0;
}

//select iso
int SCE_CTRL_CIRCLE_callback()
{
  //psvDebugScreenPrintf("psvgamesd: SCE_CTRL_CIRCLE\n");

  if(get_driver_mode() == DRIVER_MODE_VIRTUAL_MMC)
  {
    if(get_insertion_state() == INSERTION_STATE_INSERTED)
    {
      set_insertion_state(INSERTION_STATE_REMOVED);
    }

    char filepath[256];
    int found = get_dir_filename_at_pos(g_current_directory, get_file_position(), filepath);
    if(found >= 0)
    {
      char prev_iso[256];
      get_selected_iso(prev_iso);

      set_selected_iso(filepath);
      set_redraw_request(1);

      if(strncmp(prev_iso, filepath, 256) != 0)
      {
        char full_path[256];
        memset(full_path, 0, 256);
        strncpy(full_path, g_current_directory, 256);
        strcat(full_path, "/");
        strcat(full_path, filepath);

        set_iso_path(full_path);
      }
    }
  }

  return 0;
}

int SCE_CTRL_CROSS_callback()
{
  psvDebugScreenPrintf("psvgamesd: SCE_CTRL_CROSS\n");
  return 0;
}

int SCE_CTRL_SQUARE_callback()
{
  psvDebugScreenPrintf("psvgamesd: SCE_CTRL_SQUARE\n");
  return 0;
}

int main_ctrl_loop(SceSize args, void* argp)
{
  sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	
	while(get_app_running())
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

int set_dir(char* path)
{  
  strncpy(g_current_directory, path, 256);

  set_max_file_position(get_dir_max_file_pos(path));
  set_file_position(0);

  return 0;
}

int draw_dir(char* path)
{
  psvDebugScreenClear(COLOR_BLACK);

  psvDebugScreenPrintf("\e[9%im welcome to psvgamesd\n", 7);

  psvDebugScreenPrintf("\e[9%im directory: %s\n", 7, path);

  char sel_driver_mode[256];
  driver_mode_to_name(get_driver_mode(), sel_driver_mode);
  psvDebugScreenPrintf("\e[9%im driver mode: %s\n", 7, sel_driver_mode);

  if(get_driver_mode() == DRIVER_MODE_VIRTUAL_MMC)
  {
    char sel_iso[256];
    get_selected_iso(sel_iso);
    if(strnlen(sel_iso, 256) > 0)
    {
      psvDebugScreenPrintf("\e[9%im selected iso: %s\n", 7, sel_iso);

      if(get_insertion_state() == INSERTION_STATE_INSERTED)
      {
        psvDebugScreenPrintf("\e[9%im insertion state: %s\n", 7, "inserted");
      }
      else
      {
        psvDebugScreenPrintf("\e[9%im insertion state: %s\n", 7, "removed");
      }
    }
    else
    {
      psvDebugScreenPrintf("\e[9%im selected iso:\n", 7);
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
            psvDebugScreenPrintf("\e[9%im %s\n", 2, dir.d_name);
          }
          else
          {
            psvDebugScreenPrintf("\e[9%im %s\n", 7, dir.d_name);
          }
          
          cur_file_position++;
        }
      }
    }
    while(res > 0);

    sceIoDclose(dirId);
  }
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

SceUID g_ctrl_thread_id = 0;

int initialize_threading()
{
  g_app_running_mutex_id = sceKernelCreateMutex("app_running", 0, 0, 0);

  g_redraw_request_mutex_id = sceKernelCreateMutex("redraw_request", 0, 0, 0);

  g_file_position_mutex_id = sceKernelCreateMutex("file_position", 0, 0, 0);

  g_max_file_position_mutex_id = sceKernelCreateMutex("max_file_position", 0, 0, 0);

  g_selected_iso_mutex_id = sceKernelCreateMutex("selected_iso", 0, 0, 0);

  g_insertion_state_mutex_id = sceKernelCreateMutex("insertion_state", 0, 0, 0);

  g_ctrl_thread_id = sceKernelCreateThread("ctrl", main_ctrl_loop, 0x40, 0x1000, 0, 0, 0);

  if(g_ctrl_thread_id >= 0)
    sceKernelStartThread(g_ctrl_thread_id, 0, 0);

  return 0;
}

int deinitialize_threading()
{
  sceKernelDeleteMutex(g_app_running_mutex_id);

  sceKernelDeleteMutex(g_redraw_request_mutex_id);

  sceKernelDeleteMutex(g_file_position_mutex_id);

  sceKernelDeleteMutex(g_max_file_position_mutex_id);

  sceKernelDeleteMutex(g_selected_iso_mutex_id);

  sceKernelDeleteMutex(g_insertion_state_mutex_id);

  int waitRet = 0;
  sceKernelWaitThreadEnd(g_ctrl_thread_id, &waitRet, 0);

  sceKernelDeleteThread(g_ctrl_thread_id);

  return 0;
}

int main(int argc, char *argv[]) 
{
  psvDebugScreenInit();

  set_app_running(1);
  set_redraw_request(1);
  set_dir("ux0:iso");
  set_driver_mode(DRIVER_MODE_PHYSICAL_MMC);
  set_insertion_state(INSERTION_STATE_REMOVED);

  initialize_threading();

  main_draw_loop();

  deinitialize_threading();

  sceKernelDelayThread(5*1000*1000);

  sceKernelExitProcess(0);
  return 0;
}