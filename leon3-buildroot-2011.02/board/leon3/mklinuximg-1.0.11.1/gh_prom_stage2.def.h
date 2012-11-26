/*
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

#define GAISLER_IRQMP 0x00D
#define GAISLER_GPTIMER 0x011
#define GAISLER_APBUART 0x00C
#define GAISLER_ETHMAC 0x01D
#define PAGE_OFFSET 0xf0000000
#define phys_base LEONSETUP_MEM_BASEADDR
#define M_LEON 0x30
#define M_LEON3_SOC 0x02

#define __va(x) ((void *)((unsigned long) (x) - phys_base + PAGE_OFFSET))

#define LEON3_CONF_AREA 0xff000
#define LEON3_AHB_SLAVE_CONF_AREA (1 << 11)
#define LEON3_AHB_SLAVES 16
#define VENDOR_GAISLER 1
#define GAISLER_APBMST 0x006
#define GAISLER_AHB2AHB 0x020
#define LEON3_APB_CONF_WORDS 2
#define LEON3_AHB_CONF_WORDS 8
#define AMBA_MAXAPB_DEVS_PERBUS 16
#define ASI_LEON_DFLUSH 0x11
#define ASI_LEON_MMUFLUSH 0x18
#define LEON_REG_UART_CTRL_TE 0x00000002
#define LEON_REG_UART_STATUS_THE 0x00000004
#define AMBA_MAXAPB_DEVS_PERBUS 16
#define KERNBASE 0xf0000000
#define LOAD_ADDR 0x4000
#define NULL ((void *)0)
#define LEON3_IRQMPSTATUS_CPUNR 28

#define BYPASS_LOAD_PA(x) (load_reg((unsigned long)(x)))
#define BYPASS_STORE_PA(x,v) (store_reg((unsigned long)(x), (unsigned long)(v)))
