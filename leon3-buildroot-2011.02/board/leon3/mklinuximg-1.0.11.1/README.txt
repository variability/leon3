Introduction
------------

When running a LINUX build inside the LINUX source-tree for SPARC the
output is an image <linux-base>/arch/sparc/boot/image. This image however
cannot be run directly and it is linked to virtual addresses. The image
expects among other things an initalized romvector and the MMU to be switched
on. The script mklinuximg sets up such an enviroment. The outut of mklinuximg
can be uploaded using GRMON, simulated in TSIM or serve as input to MKPROM2
for PROM bootloader generation.

Invocation
----------

mklinuximg <linux-image> <out-file> [-freq <frequency>] [-base <baseaddr>] [-cmdline <string>] [-amp <string>] [-ethmac <string>] [-smp]
  <linux-image> linux image, normally <linux-dir>/arch/sparc/boot/image.
  <out-file>    output image that can be uploaded using GRMON or run in TSIM.

 optional parameters:
  -base <baseaddr>  optional baseaddress. The default is 0x40000000.
  -cmdline <string> kernel parameter string. Default is "console=ttyS0,38400".
  -freq <frequency> optional frequency parameter in case if it cannot be retrived from the Timer scalar.
  -amp <string>     optional strng of format <idx0>=<val0>:<idx1>=<val1>:... that sets for core index
                    n property ampopts to value n. Example 0=1:1:0 will set core index 0's ampopts to 1
		    and core index 1's ampopts to 0.
  -ethmac <string>  set the ethernet mac address as 12 dgt hex. Default: 00007ccc0145
  -smp              set if wrapping a SMP Linux kernel

Files
-----

 mklinuximg    the script itself
 mklinuximg.S  the linker script template. 
 pgt_gen.c     helperprogram that generates a root page table.
 prom_stage2.c the setup program that intalizes the system.
 

Linux
-----

The LINUX version has to be newer than 2.6.36.
