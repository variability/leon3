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

#include "generated/autoconf.h"
#include "linux/kernel.h"
#include "linux/version.h"
#include "asm/prom.h"
#include "asm/asi.h"
#include "asm/pgtsrmmu.h"
#include "asm/leon.h"
#include "asm/leon_amba.h"
#include "asm/page.h"
#include "asm/head.h"
#include "linux/kdev_t.h"
#include "linux/major.h"
#include "linux/root_dev.h"
#include "linux/fs.h"
#include "linux/string.h"
#include "linux/init.h"
#include "linux/of.h"
#include "asm/oplib.h"
#include "asm/idprom.h"
#include "asm/machines.h" 
#include "asm/contregs.h"
#include "asm/openprom.h"

