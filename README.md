# psvgamesd
Set of tools to run PS Vita game dumps from SD card or from binary image

Just a small note before you will try to use these tools.

Currently game dumps can be run both from SD card and from binary dump.

However you may encounter freezes during video cutscenes, glitches and graphical artifacts.

To use kernel plugin you need to obtain one of these:
- binary file with dump of game card.
- sd card which would contain binary dump and serve as physical copy.

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
   
After binary dump is obtained you can optionally create SD card physical copy by using these:

1. For Windows you can use sdioctl tool from this repo. 
   I do not have time to add cmake support to generate Visual Studio solution right now.
   You will have to do configuration yourself.
   
2. For Linux you can use your lovely dd.

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

Next milestones:

- Emulate not only CMD17 and CMD18 commands (read/write) but all other SD commands. 
  This is possible and will allow to avoid using SD card.
- Remove requirement for cmd56 handshake data to be present. 
  This is related to NpDrm and requires significant amount of patches in Iofilemgr and PfsMgr.
  I am already working on it.
- Investigate why there is a problem with freezes, glitches and artifacts.
