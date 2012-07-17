dfrplayer
=========

This is an attempt to develop an alternative, more functional firmware for the DFRduino Player.

Features:

- Arduino Compitable: once a suitable bootloader has been flased to the board, the firmware can be developed and flashed using the Arduino IDE.
- FAT32 and SDHC compatible ( 1, 2, 4 and 8GB media tested )
- Supports multi level directory structure for sounds
- Current directory aware (ability to 'cd' to directories and play files in the current context)
- Access to audio chip test functions

TODO: commands and capabilities

History:

When I originally receieved this board I was unsatisfied with the capabilities of the included firmware:
- no FAT32 support
- no SDHC support
- no source
- etc.

When I began the development, the original firmware source had not been released (this has subsequently changed) so I set out to develop my own i.e. this firmware is a clean room implementation.
The original firware was locked down on the board (no backup was possible) so I started by writing a small patch to the standard Arduino Bootloader.
This allowed me to use the Arduino IDE to flash the player firmware and to simply and rapidly prototype new versions.

TODO: bootloader patch source
TODO: FTDI board connections
TODO: auto-reset circuit
