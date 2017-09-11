# psvgamesd
Set of tools to run PS Vita game dumps from SD card or from binary image

# Introduction

WARNING: THIS README IS VERY OLD. MOST OF THE ISSUES (IF ANY), THAT ARE DESCRIBED, ARE ALREADY FIXED.
I WILL UPDATE INVORMATION SOON.

Just a small note before you will try to use these tools.
Currently game dumps can be run both from SD card and from binary dump.
Binary dumps are now run by emulating MMC card hardware (kinda iso driver).
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
   
3. Use dump function in psvgamesd when in "physical mmc" mode and dump sdstor0:gcd-lp-ign-entire.
   This will be implemented soon (need to add ui, dump code is ready).
   
# Creating physical copy with SD card
   
After binary dump is obtained you can optionally create SD card physical copy by using these:

1. For Windows you can use sdioctl tool from this repo. 
   I do not have time to add cmake support to generate Visual Studio solution right now.
   You will have to do configuration yourself.
   
2. For Linux you can use your lovely dd.

I have confirmed that these types of card work:

- SDHC
- micro SDHC (does not look that it requires slow speed patch - confirmed by xyz)

# CMD56 handshake

Data from the handshake is required to pass signature checks and obtain klicensee. It will be integrated into dump binary in the future. However it is not at the moment. To obtain the data you need to run the game in "physical mmc" mode. Signature and keys will be dumped into iso directory. You can then switch to "virtual mmc" mode to run iso.

# Putting it all together

After you have obtained binary file or SD card copy you can use psvgamesd kernel plugin.

It has several options for compilation:

#define ENABLE_SD_LOW_SPEED_PATCH - this define enables low speed mode for SD card.
I did not test this a lot. I know that SD initialization should work. However I do not know how game will behave.
This define can be used for SD, SDHC cards. SDXC cards should be ok and do not require this to be enabled.
However xyz reported that SDHC cards work ok and do not require patch.

#define ENABLE_INSERT_EMU - this switches between original card insert/remove code and reimplementation. 
both versions should be fine

# Emulating MMC card

Emulation is pretty simple. All that is required is to emulate card initialization according to MMC protocol.

There could be another way to do things - just hook initialization subroutine all together and return success result.

This way it is not required to emulate each and every MMC command. I am not sure about side effects in this scenario though.

Currently I am also able to emulate insert / remove card operations.

Current issues in emulation include:

- CMD17 is glitchy and goes to infinite loop after second command. This can be fixed by hooking read operation subroutine all together. However I do not like this approach.

- CMD18 / CMD23 is not tested.

- CID, CSD, EXT_CSD registers are hardcoded copy of original registers. Need to figure out some generic values for them.

# User App

I have added minimalistic user app which is still in developement. It will allow to manipulate the driver.

It will include mode selection:

- physical mmc (run original game)
- virtual mmc (emulate mmc card and run iso)
- physical sd (run physical copy on sd card)
- virtual sd (emulate sd card and run iso)

It will also allow to dump iso in "physical mmc" mode.

# Next milestones

- Refactor dump format
- Compress dump
- Check digital games
- Need to go through Sdif driver again and check if there are any other accesses to physical device through DMA mapped memory.
  Previous accesses include: executing command, get card insertion state. Every such access should be emulated.
- Ideally it could be great to reverse all the code that goes under command execution and reimplement it. 
  Currently I am using minimal reimplementation and not sure how stable it is.
- Remove requirement for cmd56 handshake data to be present. 
  This is related to NpDrm and requires significant amount of patches in Iofilemgr and PfsMgr.
  I am already working on it.
