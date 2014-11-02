m4boot
======

Vybrid Cortex-M4 boot utility to boot Linux

Usage:
m4boot IMAGE [INITRD] [DTB] [BOOTARGS]

IMAGE - XIP Linux Kernel image
INITRD - initramfs (optionally compressed)
DTB - Binary device tree  file
BOOTARGS - Linux kernel boot args

The utility loads the file in fixed locations of the physical memory of
the SoC and starts the secondary Cortex-M4 CPU. The default addresses are:
IMAGE: 0x8f000000
INITRD: 0x89000000
DTB: 0x8fff0000

Hence the upper 128MiB of memory on the external DDR RAM should be free
for the Cortex-M4 (use mem=128M to restrict memory usage of the kernel
running on the Cortex-A5 CPU).

The utility makes sure the bootargs and the initrd start/end address end
up in the device tree file as the Linux kernel expects it (/chosen nodes).
