# PS Vita - Virtual Game Card

User application and kernel plugin that allow to produce game card dumps and run them.

# Introduction

This application is designed for two main tasks:
- Produce 1 to 1 dumps of game cards.
- Introduce functionality that allows to run these dumps.

Aplication consists of user application that manages the driver settings and kernel plugin
that is used to emulate game card.

# Dumper comparsion (Thanks to Yifan)

https://gist.github.com/yifanlu/bcc8ebb102704ae136c650a912c6f70c

# Requirements

- henkaku
- taihen

# Legal Disclaimer

- The removal and distribution of DRM content and/or circumventing copy protection mechanisms for any other purpose than archiving/preserving games you own licenses for is illegal.
- This software is meant to be strictly reserved for your own **PERSONAL USE**.
- The author does not take any responsibility for your actions using this software.

# Relation to Cobra Blackfin

I see that lots of people are trying to relate my research and project to the Cobra Blackfin. This happens in two major ways. 

## Relation to Cobra Blackfin research

Some are trying to relate to so called "leak" that was recently published here: http://wololo.net/2017/09/13/vita-reverse-engineering-leak-cobra-blackfin-part-1/
I can claim that there is nothing new for me in that "leak". My research was always open to the public and is completely independent from Cobra Blackfin. During the developement I have stated and explained this many times. You can also check it here in my first publication that happened year ago: https://github.com/motoharu-gosuto/psvcd. Please read closely the Introduction.

## Relation to Cobra Blackfin .psv format

Others are trying to relate to the .psv format of Cobra Blackfin. I can claim that the format that we have develped with Yifan and devnoname120 is a completely independent format and has nothing to do with Cobra Blackfin. I hope you can make the differences by reading this: https://github.com/motoharu-gosuto/psvgamesd/blob/master/driver/psv_types.h As well as checking this developement thread: https://gist.github.com/yifanlu/d546e687f751f951b1109ffc8dd8d903

# Compatibility with sd2vita adapter

This plugin is not compatible with sd2vita adapter and corresponding gamesd.skprx plugin.

This plugin is not compatible with any other kernel plugin that is derived from gamesd.skprx.

If you are going to use this plugin - disable gamesd.skprx

Ideally it is recommented to remove sd2vita adapter. However when inserted - it should be ignored by this plugin.

**Integration with sd2vita is planned for next release.**

# Compatibility with game cards that have grw0: partition

There are some game cards that have write partition. It allows to write on game card.

Currently such game cards are not supported. This is planned for next release.

If you want to help me with this issue, please drop a comment at: https://github.com/motoharu-gosuto/psvgamesd/issues/10

# Installation

## Install kernel plugin

- Copy psvgamesd.skprx to the location where you have kernel plugins.
- Add the path to config.txt.
- Reboot your vita.

## Install user app

- Install psvgc.vpk.
- Virtual GC bubble will appear.

# User Interface / Controls description

Upon launching user app you will be presented with console UI.

- Press "Left" or "Right" on d-pad to switch between different modes of the driver.
- Current mode is indicated on line "driver mode:"

## Physical MMC mode - Producing Game Card Dumps
- Press "Up" or "Down" to navigate through dump files
- Press "Cross" to start dumping the came card.
  This option is only available if real game card is inserted.
  Insertion status is shown on line "content id:".
  Dump progress is shown on line "dump progress:".
  Dump file is stored at ux0:iso folder.
- Press "Square" to stop dumping the came card.
  This options is only available when dump process is started.
- Press "Triangle" to exit application.

## Virtual MMC mode / Virtual SD mode - Running Game Card Dump
- Press "Up" or "Down" to navigate through dump files
- Press "Circle" to select game of interest.
  Selection status is shown on line "selected iso:".
- Press "Start" to insert game card.
  Insertion status is shown on line "insertion state:".
- Press "Select" to remove game card.
  Insertion status is shown on line "insertion state:".
- Press "Triangle" to exit application.  

## Physical SD mode - Running Game Card Dump
- Press "Up" or "Down" to navigate through dump files
- Press "Triangle" to exit application.

# Use cases

## Typical Emulation use case
- Use physical mmc mode to produce 1:1 dump of the game card.
- Use virtual mmc or virtual sd mode to run the game.

## Typical Physical Storage use case
- Use physical mmc mode to produce 1:1 dump of the game card.
- Use your favorite tool or hex editor to burn this dump to SD card.
- Use physical sd mode to run the game.

# Huge dump size, trimming, compression etc

## Trimming zeroes

I see quite a lot of complaints about size of the .psv files. 
I guess I should have explained it from the start.
Initial design of the format assumes that 1 to 1 dumps are produced.
However you have a total freedom to derive a trimmed version of the dump from the original file.
All you have to do is to remove zeroes in the end of the file using any hex editor.
Keep in mind that data is 512 byte aligned. Then set corresponding FLAG_TRIMMED flag in the header of the file.

## Compression

At this point in time compression does not make much sense since data in the dump is encrypted.

# Reporting issues

Feel free to report bugs/issues here:
https://github.com/motoharu-gosuto/psvgamesd/issues

# Legacy ways to dump the came card

## Custom board

Use custom board. You can check how it can be built here:
https://github.com/motoharu-gosuto/psvcd

There are two modes that can be used there.
- Use PS Vita as black box (cmd56 handshake is done by PS Vita)
- Use PS Vita F00D as black box (cmd56 handshake is done by board)

For second approach you will need kirk plugins that are located here
https://github.com/motoharu-gosuto/psvkirk

Code for second approach is not finished. It only does cmd56 handshake.
However you can use the code from first approach to dump the card.
Only little changes are required.

## PSVEMMC client

https://github.com/motoharu-gosuto/psvemmc

Unfortunately dumping client is not finished. 
It is only capable of doing a dump per partition.
It does not dump SCEI MBR and does not combine partition dumps into single binary file.
However only little changes are required to make it dump SCEI MBR.
You can then combine dumps with your favorite hex editor.

# CMD56 handshake

Data from the handshake is required to pass signature checks and obtain klicensee. It is integrated into dump file. When game card is inserted in physical mmc mode signature and keys are intercepted and saved to dump file later.

# Special thanks
- Thanks to Team molecule for HENkaku and thanks to Yifan Lu for taiHEN.
- Thanks to Proxima for explaining klicensee theory.
- Thanks to SonyUSA for helping with initial testing.
