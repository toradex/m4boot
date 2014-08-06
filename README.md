m4boot
======

Vybrid Cortex-M4 boot utility

Usage:
m4boot [file]

The file requires to be a raw binary file compiled for ARMv7-M architecture,
with a vector table accordint to Cortex-M4/Vybrid reference manual. The
binary file need to begin with the reset vecotor table at offset 0. At
offset 0x400 the entry point start. Text base needs to be 0x1f000400.

This utility loads that binary to 0x3f000000, which is the actual physical
address for the SRAM accessible from the Cortex-A5. The Cortex-M4 has two
addresses to access the same SRAM: 0x1f000000 which uses the code bus,
while 0x3f000000 uses the data bus. Hence, the following configuration
is recommended

/-0x1f000000----------\ <= Start of sysRAM0 for M4 code bus
|                     |    vector table
|-0x1f000400          | <= text base/entry point for Cortex-M4
|                     |
|                     |
|                     |
|                     |
.                     .
.                     .
|-0x3f000000----------| <= Start of sysRAM0 for A5/M4 data bus
|                     |    address used by A5 to load the firmware
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
|-0x3f040000----------| <= Start of sysRAM1
|                     |    recommended place for data/bss
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
\---------------------/ <= End of sysRAM1
                           recommended place for stack
