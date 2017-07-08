# psvgamesd
Set of tools to run PS Vita game dumps from SD card or from binary image

# Introduction

Just a small note before you will try to use these tools.
Currently game dumps can be run both from SD card and from binary dump.
Binary dumps are now run by emulating MMC card hardware (kinda iso driver). Some tweaks are required as well.
SD card is no longer required to run binary dump.
There should be no freezes glitches and graphical artifacts unless your dump is damaged.

To use kernel plugin you need to obtain one of these:
- binary file with dump of game card.
- sd card which would contain binary dump and serve as physical copy.

# Obtaining binary dump of game card

This can be done in different ways:

1. Use custom board. You can check how it can be built here:
   https://github.com/motoharu-gosuto/psvcd
   
   There are two modes that can be used there.
   - Use PS Vita as black box (cmd56 handshake is done by PS Vita)
   - Use PS Vita F00D as black box (cmd56 handshake is done by board)
   For second approach you will need kirk plugins that are located here
   https://github.com/motoharu-gosuto/psvkirk
   
   Code for second approach is not finished. It only does cmd56 handshake.
   However you can use the code from first approach to dump the card.
   Only little changes are required.
   
2. Use emmc dumping client. You can find it here:
   https://github.com/motoharu-gosuto/psvemmc
   
   Unfortunately dumping client is not finished. 
   It is only capable of doing a dump per partition.
   It does not dump SCEI MBR and does not combine partition dumps into single binary file.
   Only little changes are required to make it dump SCEI MBR.
   You can then combine dumps with your favorite hex editor.
   
# Creating physical copy with SD card
   
After binary dump is obtained you can optionally create SD card physical copy by using these:

1. For Windows you can use sdioctl tool from this repo. 
   I do not have time to add cmake support to generate Visual Studio solution right now.
   You will have to do configuration yourself.
   
2. For Linux you can use your lovely dd.

I have confirmed that these types of card work:

- SDHC
- micro SDHC (requires low speed patch)

# Putting it all together

After you have obtained binary file or SD card copy you can use psvgamesd kernel plugin.

It has several options for compilation:

#define ENABLE_SD_PATCHES - this define enables all SD patches. When disabled - MMC mode is used.
In MMC mode you can run original game cards. You will need this mode because prior to running
game from SD card you will need to obtain cmd56 handshake key. I intentionally did not add cmd56_key.c file.

#define ENABLE_SD_LOW_SPEED_PATCH - this define enables low speed mode for SD card.
I did not test this a lot. I know that SD initialization should work. However I do not know how game will behave.
This define should be used for SD, SDHC cards. SDXC cards should be ok and do not require this to be enabled.

It has several options for reading:

By default all SD read/write operations will be redirected to file with binary dump.

#define ENABLE_SEPARATE_READ_THREAD - this define enables separate thread that is used to redirect SD read/write operations
to file with binary dump.

#define ENABLE_READ_THROUGH - this define enables redirection to SD card


#define ENABLE_MMC_READ - enable mmc read hooks

#define ENABLE_MMC_SEPARATE_READ_THREAD - redirect MMC read operations to separate thread

#define ENABLE_MMC_READ_THROUGH - redirect MMC read operations to MMC card

#define OVERRIDE_COMMANDS_DEBUG - hook MMC command execution and produce debug output

#define OVERRIDE_COMMANDS_EMU - redirect MMC commands to MMC card emulator (kinda iso driver)

#define OVERRIDE_CMD56_HANDSHAKE - override cmd56 handshare with custom handler if you know the keys (keys are dumped by default)

#define ENABLE_DUMP_THREAD - starts separate thread that dumps -entire mmc device

# Emulating MMC card

Emulation is pretty simple. All that is required is to emulate card initialization according to MMC protocol.

There could be another way to do things - just hook initialization subroutine all together and return success result.

This way it is not required to emulate each and every MMC command. I am not sure about side effects in this scenario though.

Current issues in emulation include:

- CMD17 is glitchy and goes to infinite loop after second command. This can be fixed by hooking read operation subroutine all together. However I do not like this approach.

- CMD18 / CMD23 is not tested.

- Need to add software emulation of card insert/remove. Currently I am connecting INS and GND pins with DIP switch.

# Next milestones

- Remove requirement for cmd56 handshake data to be present. 
  This is related to NpDrm and requires significant amount of patches in Iofilemgr and PfsMgr.
  I am already working on it.
