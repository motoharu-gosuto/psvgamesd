#pragma once

//enables all sd patches
//#define ENABLE_SD_PATCHES

//enables patch for low speed cards
//#define ENABLE_SD_LOW_SPEED_PATCH 

//enables reading image in separate thread
//#define ENABLE_SEPARATE_READ_THREAD

//enables reading operations from actual card
//#define ENABLE_READ_THROUGH

//enables reading operations from image in MMC mode
//not compatible with sd reading
#define ENABLE_MMC_READ

#define ENABLE_MMC_SEPARATE_READ_THREAD
//#define ENABLE_MMC_READ_THROUGH

//#define OVERRIDE_COMMANDS_DEBUG

#define OVERRIDE_COMMANDS_EMU

#define OVERRIDE_CMD56_HANDSHAKE

//#define ENABLE_DUMP_THREAD