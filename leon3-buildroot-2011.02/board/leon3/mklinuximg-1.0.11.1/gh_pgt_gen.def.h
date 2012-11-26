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

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define KERNBASE 0xf0000000
#define SRMMU_PGDIR_MASK (~(SRMMU_PGDIR_SIZE-1))
#define SRMMU_PGDIR_SIZE (1UL << SRMMU_PGDIR_SHIFT)
#define SRMMU_PGDIR_SHIFT 24
#define SRMMU_ET_PTE 0x2
#define SRMMU_PRIV 0x1c
#define SRMMU_VALID 0x02
#define SRMMU_CACHE 0x80
#define LOAD_ADDR 0x4000
