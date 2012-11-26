/*
* Linux wrapper
*
* Copyright (c) 2010 Aeroflex Gaisler AB
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
* MA 02110-1301 USA
*/ 

/*
  moved to gh_pgt_gen.c 
  #include "generated/autoconf.h"
  #include <asm-generic/int-ll64.h>
  #include "linux/kernel.h"
  
  #include "asm/asi.h"
  #include "asm/pgtsrmmu.h"
  #include "asm/head.h"
  #undef __KERNEL__
  #include "asm/leon.h"
*/
#include "gh_pgt_gen.def.h"
#include <stdio.h>

int main(void){ 

	unsigned long mem[LOAD_ADDR+1];
	unsigned int addr, i;

	printf("	.seg \"data\"\n");
	printf("ctx_table:\n");
	//printf("	.fill %i\n", PAGE_SIZE-8);

	/* we are now @ LEON_MEM_BASE_ADDRESS + (PAGE_SIZE*2) */

	/* ctx 0 (kernel) pgd ( SRMMU_ET_PTD | (((unsigned long)pgd) >> 4) ) */
	printf("	.word 0x%x\n", (((LEONSETUP_MEM_BASEADDR +(PAGE_SIZE*2) + 256*4) >> 4) & ~0x3) | 1);

	/* invalidate ctx 1-255 */
	printf("	.fill %i\n", 255*4 );

	printf("pgd_table:\n");
	  
	/* <one on one> for 0 - 0xf0000000 */
	addr = 0;
	for (i = 0; i < (unsigned int)( (KERNBASE&SRMMU_PGDIR_MASK)
					 >>SRMMU_PGDIR_SHIFT); i++) {
		printf("	.word 0x%x\n",  SRMMU_ET_PTE | (addr>>4)
			     | SRMMU_PRIV | SRMMU_VALID | SRMMU_CACHE);
		addr += SRMMU_PGDIR_SIZE;
	}
	
	/* 0xf0000000-0xfC000000 : 192 MB, map to KERNEL_MEM by default
	 * (overwritten by ROMKERNEL) */
	for (i=0; i<12; i++) {
		printf("	.word 0x%x\n", SRMMU_ET_PTE |
			(LEONSETUP_MEM_BASEADDR + 0x1000000*i) >> 4 |
			SRMMU_PRIV | SRMMU_VALID | SRMMU_CACHE);
	}
	//printf("	.fill %i\n", 0x4000-0x1000-0x800+15*4 );
  
	/* 0xfC000000-0xffffffff : 256-48MB, mark invalid map */
	printf("	.fill %i\n", (256-240-12)*4);

	return 0;
}

