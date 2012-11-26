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

#ifndef CONFIG_KERNEL_INIT_PATH
#define CONFIG_KERNEL_INIT_PATH "/sbin/init"
#endif
#ifndef CONFIG_KERNEL_COMMAND_LINE
#define CONFIG_KERNEL_COMMAND_LINE "console=ttyS0,38400"
#endif
#ifndef CONFIG_KERNEL_ROOTMEM_INITRAMFS
#define CONFIG_KERNEL_ROOTMEM_INITRAMFS 0
#endif

/*moved to gh_prom_stage2.c 
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
*/

#include "gh_prom_stage2.def.h"
#include "gh_prom_stage2.pre.h"

/* AMBA Plug and Play Base address for first bus (scanning starting here) */
#define LEON3_IO_AREA 0xfff00000

#ifndef IPI_NUM
#define IPI_NUM 0 /* Use Default Linux IRQ number */
#elif ((IPI_NUM > 14) || (IPI_NUM < 0))
#error IPI must be a vector IRQ (1..14) and can not be shared with other IRQs
#endif

#ifndef LINUX_VERSION_CODE
#define LINUX_VERSION_CODE 0x00020624 /* Default to Linux 2.6.36 */
#endif

#define LINVER_MAJOR ((LINUX_VERSION_CODE >> 16) & 0xff)
#define LINVER_MINOR ((LINUX_VERSION_CODE >> 8) & 0xff)
#define LINVER_REV (LINUX_VERSION_CODE & 0xff)

#define KERNEL_VER(major, minor, rev) (((major) << 16) | ((minor) << 8) | (rev))

/* for __va */
#define phys_base LEONSETUP_MEM_BASEADDR

#define _va_call(f,typ) ((typ)(*(int *)(__va(&(f)))))

/* do a physical address bypass load, i.e. for 0x80000000 */
static inline unsigned long load_reg(unsigned long paddr)
{
#define ASI_LEON_BYPASS 0x1c
	unsigned long retval;
	__asm__ __volatile__("lda [%1] %2, %0\n\t":
			     "=r"(retval):"r"(paddr), "i"(ASI_LEON_BYPASS));
	return retval;
}

#define BYPASS_LOAD_PA(x)	    (load_reg((unsigned long)(x)))

/* do a physical address bypass write, i.e. for 0x80000000 */
static inline void store_reg(unsigned long paddr, unsigned long value)
{
	__asm__ __volatile__("sta %0, [%1] %2\n\t"::"r"(value), "r"(paddr),
			     "i"(ASI_LEON_BYPASS):"memory");
}

#define BYPASS_STORE_PA(x, v) (store_reg((unsigned long)(x), (unsigned long)(v)))

struct short_property {
	char *name;
	char *value;
	int length;
};

struct amba_short_property {
	char *name;
	char *value;
	int length;
};

struct amba_core {
	unsigned int device, vendor, version, device_mask, userdef[6],
	    userdefcnt, freq[3], freq_cnt, index, ampopts;
	int conf[3], conf_b[3], irq[3], regs[32 * 2], regscnt, conf_freq_khz[3];
	/* needs to be insync with pnames */
	/*0 */ struct amba_short_property name,
	    /*1 */ vendor_p,
	    /*2 */ device_p,
	    /*3 */ version_p,
	    /*4 */ device_mask_p,
	    /*5 */ interrupts,
	    /*6 */ reg,
	    /*7 */ userdef_p,
	    /*8 */ freq_p,
	    /*9 */ index_p,
	    /*10 */ ampopts_p;

	char _name[64];
};

char *pnames[] = {
	/*0 */ "name",
	/* needs to be insync with amba_core */
	/*1 */ "vendor",
	/*2 */ "device",
	/*3 */ "version",
	/*4 */ "device_mask",
	/*5 */ "interrupts",
	/*6 */ "reg",
	/*7 */ "userdef",
	/*8 */ "freq",
	/*9 */ "index",
	/*10 */ "ampopts",
	/*11 */ 0
};

#define PNAMES_LAST 10

struct amp_opt {
	short idx, val;
} amp[] = {
#include "amp_opts.c"
	{
	-1, -1}
};

struct gaisler_cores {
	int device;
	char *n;
} gcores[] = {
	{
	GAISLER_IRQMP, "GAISLER_IRQMP"}, {
	GAISLER_GPTIMER, "GAISLER_GPTIMER"}, {
	GAISLER_APBUART, "GAISLER_APBUART"}, {
	GAISLER_ETHMAC, "GAISLER_ETHMAC"}, {
	0, 0}
};

typedef int (*amba_call) (unsigned int *, unsigned int, unsigned int, int, int,
			  void *);

static inline int lo_strnlen(const char *s, int count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */ ;
	return sc - s;
}

/* avoid udiv reference */
static inline unsigned int int_div(unsigned int v, unsigned int d)
{
	unsigned int s = d, a = 1, r = 0, i;
	if (d == 0)
		return 0;
	for (i = 0; i < 32; i++) {
		if (s >= v)
			break;
		s = s << 1;
		a = a << 1;
	}
	while (s >= d) {
		while (s <= v) {
			v -= s;
			r += a;
		}
		s = s >> 1;
		a = a >> 1;
	}
	return r;
}

/* avoid umul reference */
static inline unsigned int int_mul(unsigned int v, unsigned int m)
{
	unsigned int r = 0, i;
	for (i = 0; i < 32; i++) {
		if (m & 1)
			r += v;
		m = m >> 1;
		v = v << 1;
	}
	return r;
}

static int leon_nbputchar(int c);

#define DO_CACHING
#define DEBUG_PROMSTAGE_UART 0x80000100
#undef  DEBUG_PROMSTAGE
#define DEBUG_PROMSTAGE

#define DBG_PRINTF_1(fmt) DBG_PRINTF(fmt, 0, 1, 2, 3, 4, 5, 6)
#define DBG_PRINTF_2(fmt,a0) DBG_PRINTF(fmt, a0, 1, 2, 3, 4, 5, 6)
#define DBG_PRINTF_3(fmt,a0,a1) DBG_PRINTF(fmt, a0, a1, 2, 3, 4, 5, 6)
#define DBG_PRINTF_4(fmt,a0,a1,a2) DBG_PRINTF(fmt, a0, a1, a2, 3, 4, 5, 6)
#define DBG_PRINTF_5(fmt,a0,a1,a2,a3) DBG_PRINTF(fmt, a0, a1, a2, a3, 4, 5, 6)
#define DBG_PRINTF_6(fmt,a0,a1,a2,a3,a4) DBG_PRINTF(fmt, a0, a1, a2, a3, a4, 5, 6)

#ifdef DEBUG_PROMSTAGE
#undef DO_CACHING

int _dbg_printf(const char *fmt, int a0, int a1, int a2, int a3, int a4, int a5,
		int a6);
typedef int (*_dbg_printf_fn) (const char *fmt, int a0, int a1, int a2, int a3,
			       int a4, int a5, int a6);
_dbg_printf_fn _dbg_printf_p = _dbg_printf;
#define DBG_PRINTF(fmt, a0, a1, a2, a3, a4, a5, a6) _va_call(_dbg_printf_p,_dbg_printf_fn)(STR_VA(fmt), (int)(a0), (int)(a1), (int)(a2), (int)(a3), (int)(a4), (int)(a5), (int)(a6))

static inline int lo_vsnprintf(char *buf, int size, const char *fmt, int *args)
{
	int len;
	int vac = 0;
	unsigned long long num;
	int i, j, n;
	char *str, *end, c;
	const char *s;
	int flags;
	int field_width;
	int precision;
	int qualifier;
	int filler;

	str = buf;
	end = buf + size - 1;

	if (end < buf - 1) {
		/* end = ((void *) -1); */
		/* size = end - buf + 1; */
		*buf = 0;
		return 0;
	}

	for (; *fmt; ++fmt) {
		if (*fmt != '%') {
			if (str <= end)
				*str = *fmt;
			++str;
			continue;
		}

		/* process flags */
		flags = 0;
		/* get field width */
		field_width = 0;
		/* get the precision */
		precision = -1;
		/* get the conversion qualifier */
		qualifier = 'l';
		filler = ' ';

		++fmt;

		/* default base */
		switch (*fmt) {
		case 's':
			s = (char *)args[vac++];
			if (!s)
				s = "<NULL>";

			len = lo_strnlen(s, precision);

			for (i = 0; i < len; ++i) {
				if (str <= end)
					*str = *s;
				++str;
				++s;
			}
			while (len < field_width--) {
				if (str <= end)
					*str = ' ';
				++str;
			}
			continue;

		case '%':
			if (str <= end)
				*str = '%';
			++str;
			continue;

		case 'p':
			break;
		case 'x':
			break;
		case 'd':
			break;

		default:
			if (str <= end)
				*str = '%';
			++str;
			if (*fmt) {
				if (str <= end)
					*str = *fmt;
				++str;
			} else {
				--fmt;
			}
			continue;
		}
		num = args[vac++];

		{
			int l;
			char *b = str;
			for (j = 0, i = 0; i < 8 && str <= end; i++) {
				if ((n =
				     ((unsigned long)(num &
						      (0xf0000000ul >>
						       (i * 4)))) >> ((7 -
								       i) * 4))
				    || j != 0) {
					j = 1;
					if (n >= 10)
						n += 'a' - 10;
					else
						n += '0';
					*str = n;
					++str;
				}
			}
			l = (str - b);
			if (l < field_width) {
				for (i = 0; i < (field_width - l) && str <= end;
				     i++) {
					for (j = 0; j < l; j++) {
						str[-j] = str[-(j + 1)];
					}
					str[-l] = filler;
					str++;
				}
				j = 1;
			}

		}
		if (j == 0 && str <= end) {
			*str = '0';
			++str;
		}
	}
	if (str <= end)
		*str = '\0';
	else if (size > 0)
		/* don't write out a null byte if the buf size is zero */
		*end = '\0';
	/* the trailing null byte doesn't count towards the total
	 * ++str;
	 */
	return str - buf;
}

static inline void outbyte(unsigned int c)
{
	((int (*)(int))__va(&leon_nbputchar)) (c);
}

int _dbg_printf(const char *fmt, int a0, int a1, int a2, int a3, int a4, int a5,
		int a6)
{
	unsigned int ch, i;
	char printk_buf[1024];
	int args[7];
	int printed_len, ret_len;
	char *p = printk_buf;

	args[0] = a0;
	args[1] = a1;
	args[2] = a2;
	args[3] = a3;
	args[4] = a4;
	args[5] = a5;
	args[6] = a6;

	/* Emit the output into the temporary buffer */
	ret_len = printed_len =
	    lo_vsnprintf(printk_buf, sizeof(printk_buf), fmt, args);

	for (i = 0; i < printed_len; i++) {
		outbyte(ch = printk_buf[i]);
		if (ch == '\n') {
			outbyte('\r');
		}
	}

	return ret_len;
}
#else
#define DBG_PRINTF(fmt, a0, a1, a2, a3, a4, a5, a6)
#endif

/* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29) */
/* /\* from 2.6.29 on explicit leon2 support is missing (only macro CONFIG_LEON remain) *\/ */
#define CONFIG_LEON_3
/* #else */
/* # error "No support for kernels < 2.6.29" */
/* #endif */

#if (CONFIG_KERNEL_ROOTMEM_ROMFS == 1)
/* from arch/sparc/kernel/setup.c */
#define RAMDISK_LOAD_FLAG 0x4000
extern unsigned short root_flags;
extern unsigned short root_dev;
extern unsigned short ram_flags;
extern unsigned int sparc_ramdisk_image;
extern unsigned int sparc_ramdisk_size;
extern int root_mountflags;
extern char initrd_end, initrd_start;
#endif

#undef memcpy
#define memcpy no_inline_memcpy
#undef strcpy
#define strcpy no_inline_strcpy
#undef memset
#define memset no_inline_memset
#undef strcmp
#define strcmp no_inline_strcmp

#define STR_VA(v) ((char *)__va(((char *)(v))))

struct amba_scan_io {
	struct amba_scan_io *p;
	unsigned int ioarea;
};

void _no_inline_memcpy(char *d, char *s, int len);
void _no_inline_strcpy(char *d, char *s);
void _no_inline_memset(void *d, int v, int len);
int _no_inline_strcmp(char *d, char *s);
int _amba_scan(struct amba_scan_io *io, unsigned int freq_khz, int virtual,
	       int *nextid, amba_call func, void *funcarg);
int _find_core(int id, struct amba_core *a);
int _find_name_idx(char *name);

typedef void (*_no_inline_memcpy_fn) (char *d, char *s, int len);
typedef void (*_no_inline_strcpy_fn) (char *d, char *s);
typedef void (*_no_inline_memset_fn) (void *d, int v, int len);
typedef int (*_no_inline_strcmp_fn) (char *d, char *s);
typedef int (*_amba_scan_fn) (struct amba_scan_io *, unsigned int, int, int *,
			      amba_call, void *);
typedef int (*_find_core_fn) (int id, struct amba_core * a);
typedef int (*_find_name_idx_fn) (char *name);

_no_inline_memcpy_fn _no_inline_memcpy_p = 0;
_no_inline_strcpy_fn _no_inline_strcpy_p = 0;
_no_inline_memset_fn _no_inline_memset_p = 0;
_no_inline_strcmp_fn _no_inline_strcmp_p = 0;
_amba_scan_fn _amba_scan_p = _amba_scan;
_find_core_fn _find_core_p = _find_core;
_find_name_idx_fn _find_name_idx_p = _find_name_idx;

#define no_inline_memcpy(d,s,len) _va_call(_no_inline_memcpy_p,_no_inline_memcpy_fn)(d,s,len)
#define no_inline_strcpy(d,s) _va_call(_no_inline_strcpy_p,_no_inline_strcpy_fn)(d,s)
#define no_inline_memset(d,v,len) _va_call(_no_inline_memset_p,_no_inline_memset_fn)(d,v,len)
#define no_inline_strcmp(d,s) _va_call(_no_inline_strcmp_p,_no_inline_strcmp_fn)(d,s)
#define amba_scan(a,b,c,d,e,f) _va_call(_amba_scan_p,_amba_scan_fn)(a,b,c,d,e,f)
#define find_core(a,b) _va_call(_find_core_p,_find_core_fn)(a,b)
#define find_name_idx(a) _va_call(_find_name_idx_p,_find_name_idx_fn)(a)

void _no_inline_memcpy(char *d, char *s, int len)
{
	int i = 0;
	for (i = 0; i < len; i++) {
		d[i] = s[i];
	}
}

void _no_inline_strcpy(char *d, char *s)
{
	int i = 0;
	while (d[i] = s[i]) {
		i++;
	};
}

void _no_inline_memset(void *d, int v, int len)
{
	int i = 0;
	for (i = 0; i < len; i++) {
		((char *)d)[i] = v;
	}
}

int _no_inline_strcmp(char *d, char *s)
{
	while (1) {
		if (*d != *s)
			return 1;
		if (*d == 0 || *s == 0)
			return 0;
		d++;
		s++;
	}
	return 1;
}

struct node {
	int level;
	struct short_property *properties;
};

static void leon_reboot(char *bcommand);
static void leon_halt(void);
static int leon_nbputchar(int c);
static int leon_nbgetchar(void);
static int no_nextnode(int node);
static int no_child(int node);
static int no_proplen(int node, char *name);
static int no_getprop(int node, char *name, char *value);
static int no_setprop(int node, char *name, char *value, int len);
static char *no_nextprop(int node, char *name);

/* a self contained prom info structure */
struct leon_prom_info {
	int freq_khz;
	int leon_nctx;
	unsigned long amba_systemid;
	unsigned long amba_ioarea;
	int ipi_num;
	int mids[8];
	int baudrates[2];
	struct short_property root_properties[4];
	struct short_property serial_properties[3];
	struct short_property ambapp_properties[6];
	struct short_property cpu_properties[7];
#undef  CPUENTRY
#define CPUENTRY(idx) struct short_property cpu_properties##idx[4];
	 CPUENTRY(1)
	    CPUENTRY(2)
	    CPUENTRY(3) CPUENTRY(4) CPUENTRY(5) CPUENTRY(6) CPUENTRY(7)
	struct idprom idprom;
	struct linux_nodeops nodeops;
	struct linux_mlist_v0 *totphys_p;
	struct linux_mlist_v0 totphys;
	struct linux_mlist_v0 *avail_p;
	struct linux_mlist_v0 avail;
	struct linux_mlist_v0 *prommap_p;
	void (*synchook) (void);
	struct linux_arguments_v0 *bootargs_p;
	struct linux_arguments_v0 bootargs;
	struct linux_romvec romvec;
	struct node nodes[37];
	char s_device_type[12];
	char s_cpu[4];
	char s_serial[7];
	char s_ambapp[7];
	char s_ambapp0[8];
	char s_serial_name[3];
	char s_name[5];
	char s_mid[4];
	char s_idprom[7];
	char s_compatability[14];
/* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29) */
	char s_leon[5];
/* #else */
	/* char s_leon2[6]; */
/* #endif */
	char s_mmu_nctx[9];
	char s_frequency[16];
	char s_uart1_baud[11];
	char s_uart2_baud[11];
	char s_systemid[9];
	char s_ioarea[7];
	char s_ipi_num[8];
	char s_pv_a[2];
	char arg[];
};

/* static prom info */
static struct leon_prom_info spi = {
	0,			/* freq_khz */
	256,			/* leon_nctx */
	0,			/* AMBA System ID (autodetect) */
	LEON3_IO_AREA,		/* Primary AMBA bus PnP Base address */
	IPI_NUM,		/* Inter Process IRQ Number: 1..14 */
	{
#undef  CPUENTRY
#define CPUENTRY(idx)  idx,
	 CPUENTRY(0)
	 CPUENTRY(1)
	 CPUENTRY(2)
	 CPUENTRY(3)
	 CPUENTRY(4)
	 CPUENTRY(5)
	 CPUENTRY(6)
	 CPUENTRY(7)
	 },
	{			/* baudrates */
	 38400, 38400},
	{			/* root_properties */
	 {__va(spi.s_device_type), __va(spi.s_idprom), 4},
	 {__va(spi.s_idprom), (char *)__va(&spi.idprom), sizeof(struct idprom)},
/* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29) */
	 {__va(spi.s_compatability), __va(spi.s_leon), 4},
/* #else */
/* 	  {__va(spi.s_compatability), __va(spi.s_leon2), 5}, */
/* #endif */
	 {NULL, NULL, -1}
	 },
	{			/* serial_properties */
	 {__va(spi.s_device_type), __va(spi.s_serial), 7},
	 {__va(spi.s_name), __va(spi.s_serial_name), 3},
	 {NULL, NULL, -1}
	 },
	{			/* ambapp_properties */
	 {__va(spi.s_device_type), __va(spi.s_ambapp), 7},
	 {__va(spi.s_name), __va(spi.s_ambapp0), 8},
	 {__va(spi.s_systemid), __va(&spi.amba_systemid), 4},
	 {__va(spi.s_ioarea), __va(&spi.amba_ioarea), 4},
	 {__va(spi.s_ipi_num), __va(&spi.ipi_num), 4},
	 {NULL, NULL, -1}
	 },
	{			/* cpu_properties */
	 {__va(spi.s_device_type), __va(spi.s_cpu), 4},
	 {__va(spi.s_mid), __va(&spi.mids[0]), 4},
	 {__va(spi.s_mmu_nctx), (char *)__va(&spi.leon_nctx), 4},
	 {__va(spi.s_frequency), (char *)__va(&spi.freq_khz), 4},
	 {__va(spi.s_uart1_baud), (char *)__va(&spi.baudrates[0]), 4},
	 {__va(spi.s_uart2_baud), (char *)__va(&spi.baudrates[1]), 4},
	 {NULL, NULL, -1}
	 },
#undef  CPUENTRY
#define CPUENTRY(idx)							\
	{ /* cpu_properties */						\
	  {__va(spi.s_device_type), __va(spi.s_cpu), 4},		\
	  {__va(spi.s_mid), __va(&spi.mids[idx]), 4},			\
	  {__va(spi.s_frequency), (char *)__va(&spi.freq_khz), 4},	\
	  {NULL, NULL, -1}						\
	},								\

	CPUENTRY(1)
	    CPUENTRY(2)
	    CPUENTRY(3)
	    CPUENTRY(4)
	    CPUENTRY(5)
	    CPUENTRY(6)
	    CPUENTRY(7) {	/* idprom */
			 0x01,	/* format */
			 M_LEON | M_LEON3_SOC,	/* machine type */
#define ETHMACPART(v,s) (((v) >> s) & 0xff)
			 {ETHMACPART(ETHMACDEF, 40),
			  ETHMACPART(ETHMACDEF, 32),
			  ETHMACPART(ETHMACDEF, 24),
			  ETHMACPART(ETHMACDEF, 16),
			  ETHMACPART(ETHMACDEF, 8),
			  ETHMACPART(ETHMACDEF, 0)},	/* eth */
			 0,	/* date */
			 0,	/* sernum */
			 0,	/* checksum */
			 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}	/* reserved */
			 },
	{			/* nodeops */
	 __va(no_nextnode),
	 __va(no_child),
	 __va(no_proplen),
	 __va(no_getprop),
	 __va(no_setprop),
	 __va(no_nextprop)
	 },
	__va(&spi.totphys),	/* totphys_p */
	{			/* totphys */
	 NULL,
	 (int)(char *)LEONSETUP_MEM_BASEADDR,
	 0,
	 },
	__va(&spi.avail),	/* avail_p */
	{			/* avail */
	 NULL,
	 (int)(char *)LEONSETUP_MEM_BASEADDR,
	 0,
	 },
	NULL,			/* prommap_p */
	NULL,			/* synchook  */
	__va(&spi.bootargs),	/* bootargs_p */
	{			/* bootargs */
	 {NULL, __va(spi.arg), NULL /*... */ },
	 /*... */
	 },
	{			/* romvec */
	 0,
	 0,			/* sun4c v0 prom */
	 0, 0,
	 {__va(&spi.totphys_p), __va(&spi.prommap_p), __va(&spi.avail_p)},
	 __va(&spi.nodeops),
	 NULL, {NULL /* ... */ },
	 (char *)__va(&spi.s_pv_a[0]), (char *)__va(&spi.s_pv_a[0]),	/* PROMDEV_TTYA, PROMDEV_TTYA */
	 NULL, NULL,		/* pv_getchar, pv_putchar */
	 __va(leon_nbgetchar), __va(leon_nbputchar),
	 NULL,
	 __va(leon_reboot),
	 NULL,
	 NULL,
	 NULL,
	 __va(leon_halt),
	 __va(&spi.synchook),
	 {NULL},
	 __va(&spi.bootargs_p)
	 /*... */
	 },
	{			/* nodes */
	 { /*0: */ 0, __va(spi.root_properties + 3) /* NULL, NULL, -1 */ },
	 { /*1: */ 0, __va(spi.root_properties)},
	 { /*2: */ 1, __va(spi.cpu_properties)},	/* cpu 0, must be spi.nodes[2] see leon_prom_init() */
	 { /*3: */ 1, __va(spi.serial_properties)},
	 { /*4: */ 1, __va(spi.ambapp_properties)},

#undef  CPUENTRY
#define CPUENTRY(idx) \
	  { 1, __va(spi.cpu_properties##idx) },	/* cpu <idx> */
	 CPUENTRY(1)
	 CPUENTRY(2)
	 CPUENTRY(3)
	 CPUENTRY(4)
	 CPUENTRY(5)
	 CPUENTRY(6)
	 CPUENTRY(7) {-1, __va(spi.root_properties + 3) /* NULL, NULL, -1 */ }
	 },
	"device_type",
	"cpu",
	"serial",
	"ambapp",
	"ambapp0",
	"a:",
	"name",
	"mid",
	"idprom",
	"compatability",
/* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29) */
	"leon",
/* #else */
/* 	"leon2", */
/* #endif */
	"mmu-nctx",
	"clock-frequency",
	"uart1_baud",
	"uart2_baud",
	"systemid",
	"ioarea",
	"ipi_num",
	"\001",
/*#if !(defined(CONFIG_VT) && defined(CONFIG_VGA_CONSOLE))
	"console=ttyS0,38400 root=/dev/hda4"
#else
	"console=tty" 
#endif*/
	CONFIG_KERNEL_COMMAND_LINE
#if (CONFIG_KERNEL_ROOTMEM_INITRAMFS  == 1)
	    " rdinit="
#else
	    " init="
#endif
	CONFIG_KERNEL_INIT_PATH
};

/* node ops */
#define bnodes ((struct node *)__va(&spi.nodes))

#define BIT_FREE(d,id) (!(d->freevec[(id)/32] & (1 << ((id) & 0x1f))))
#define BIT_SET(d,id) d->freevec[(id)/32] |= (1 << ((id) & 0x1f))
#define amba_vendor(x) (((x) >> 24) & 0xff)
#define amba_device(x) (((x) >> 12) & 0xfff)
#define amba_irq(conf) ((conf) & 0x1f)
#define amba_ver(conf) (((conf)>>5) & 0x1f)
#define amba_iobar_start(base, iobar) ((base) | ((((iobar) & 0xfff00000)>>12) & (((iobar) & 0xfff0)<<4)) )
#define amba_membar_mask(mbar) (((mbar) >> 4) & 0xfff)
#define amba_apb_mask(iobar) ((~(amba_membar_mask(iobar)<<8) & 0x000fffff) + 1)
#define amba_membar_start(mbar) (((mbar) & 0xfff00000) & (((mbar) & 0xfff0) << 16))
#define amba_membar_type(mbar) ((mbar) & 0xf)
#define amba_ahbio_adr(addr,base_ioarea) ((unsigned int)(base_ioarea) | ((addr) >> 12))

#define APB_SLAVE 0
#define AHB_SLAVE 1
#define AHB_MASTER 1

#define AMBA_TYPE_APBIO 0x1
#define AMBA_TYPE_MEM   0x2
#define AMBA_TYPE_AHBIO 0x3

static struct short_property *find_property(int node, char *name,
					    struct short_property *p);

/* GAISLER AHB2AHB Version 1 Bridge Definitions */
#define AHB2AHB_V1_FLAG_FFACT     0x0f0	/* Frequency factor against top bus */
#define AHB2AHB_V1_FLAG_FFACT_DIR 0x100	/* Factor direction, 0=down, 1=up */
#define AHB2AHB_V1_FLAG_MBUS      0x00c	/* Master bus number mask */
#define AHB2AHB_V1_FLAG_SBUS      0x003	/* Slave bus number mask */

#define GAISLER_L2CACHE      0x04b

int _amba_scan(struct amba_scan_io *io, unsigned int freq_khz, int virtual,
	       int *nextid, amba_call func, void *funcarg)
{
	unsigned int ioarea = io->ioarea;
	unsigned int *cfg_area, *cfg_area_slv;
	unsigned int apbmst, mbar, conf, custom, userconf, ffact;
	int i, j, k, r = 0, dir;

	cfg_area = (unsigned int *)(ioarea | LEON3_CONF_AREA);
	cfg_area_slv =
	    (unsigned int *)(ioarea | LEON3_CONF_AREA |
			     LEON3_AHB_SLAVE_CONF_AREA);

	for (i = 0; i < LEON3_AHB_SLAVES && !r; i++) {

		if (BYPASS_LOAD_PA(cfg_area_slv)) {

			/*DBG_PRINTF_3("    SLV: %x:%x\n",amba_vendor(conf), amba_device(conf)); */

			/* ------------- */
			if (func)
				r = func(cfg_area_slv, ioarea, freq_khz,
					 *nextid, AHB_SLAVE, funcarg);
			(*nextid)++;
			/* ------------- */

			conf = BYPASS_LOAD_PA(cfg_area_slv + (0));
			mbar = BYPASS_LOAD_PA(cfg_area_slv + (4 + 0));

			if ((amba_vendor(conf) == VENDOR_GAISLER) &&
			    (amba_device(conf) == GAISLER_AHB2AHB
			     || amba_device(conf) == GAISLER_L2CACHE)) {

				unsigned int freq_khz_new = freq_khz, isrec = 0;
				struct amba_scan_io io2, *search = io;

				/* Found AHB->AHB bus bridge custom config 1 contains ioarea. */
				custom = BYPASS_LOAD_PA(cfg_area_slv + (1 + 1));
				io2.p = io;
				io2.ioarea = custom;

				while (search) {
					if (search->ioarea == custom) {
						isrec = 1;
						break;
					}
					search = search->p;
				}

				if (amba_ver(conf) > 2) {
					/* todo */
				} else {
					userconf =
					    BYPASS_LOAD_PA(cfg_area_slv + (1));
					ffact =
					    (userconf & AHB2AHB_V1_FLAG_FFACT)
					    >> 4;
					if (ffact > 1) {
						if ((dir =
						     (userconf &
						      AHB2AHB_V1_FLAG_FFACT_DIR)))
						{
							freq_khz_new =
							    int_mul(freq_khz,
								    ffact);
						} else {
							freq_khz_new =
							    int_div(freq_khz,
								    ffact);
						}
					}
				}
				if (!isrec) {
					if (virtual) {
						r = amba_scan(&io2,
							      freq_khz_new,
							      virtual, nextid,
							      func, funcarg);
					} else {
						r = _amba_scan(&io2,
							       freq_khz_new,
							       virtual, nextid,
							       func, funcarg);
					}
				}
			} else if ((amba_vendor(conf) == VENDOR_GAISLER) &&
				   (amba_device(conf) == GAISLER_APBMST)) {

				apbmst = amba_membar_start(mbar);
				cfg_area =
				    (unsigned int *)(apbmst | LEON3_CONF_AREA);

				for (k = 0; k < AMBA_MAXAPB_DEVS_PERBUS && !r;
				     k++) {

					unsigned int vendor, device;
					unsigned int apb_conf =
					    BYPASS_LOAD_PA(cfg_area);
					unsigned int iobar =
					    BYPASS_LOAD_PA(cfg_area + 1);
					if ((vendor = amba_vendor(apb_conf))
					    && (device = amba_device(apb_conf))) {

						/*DBG_PRINTF_3("APB-SLV: %x:%x\n",amba_vendor(apb_conf), amba_device(apb_conf)); */

						/* ------------- */
						if (func)
							r = func(cfg_area,
								 apbmst,
								 freq_khz,
								 *nextid,
								 APB_SLAVE,
								 funcarg);
						(*nextid)++;
						/* ------------- */
					}
					cfg_area += LEON3_APB_CONF_WORDS;
				}
			}
		}
		cfg_area_slv += LEON3_AHB_CONF_WORDS;
	}
	return r;
}

struct find_data {
	/* for find_core_free and find_core_pair */
	unsigned int freevec[1024 / 32];
	struct amba_core *c;
	/* for leon_prom_amba_init */
	int found_timer, found_mpirq, found_uart;
};

/* callback for _amba_scan: get the first free SLV, MST, APBSLV that is free */
int find_core_free(unsigned int a, unsigned int b, unsigned int f, int id,
		   int typ, struct find_data *d)
{
	unsigned int conf;
	unsigned int vendor, device;

	if (BIT_FREE(d, id)) {
		BIT_SET(d, id);
		d->c->conf[typ] = a;
		d->c->conf_b[typ] = b;
		d->c->conf_freq_khz[typ] = f;

		conf = BYPASS_LOAD_PA(a);
		d->c->irq[typ] = amba_irq(conf);
		d->c->vendor = amba_vendor(conf);
		d->c->device = amba_device(conf);
		d->c->version = amba_ver(conf);

		/*DBG_PRINTF_6(" f %x:%x %x:%x %x\n",typ, id, d->c->vendor, d->c->device, a); */
		return 1;
	}
	return 0;
}

/* callback for _amba_scan: pair the device id returned by find_core_free with SLV, MST, APBSLV of same id */
int find_core_pair(unsigned int a, unsigned int b, unsigned int f, int id,
		   int typ, struct find_data *d)
{
	unsigned int conf, device, vendor, version;
	if (BIT_FREE(d, id)) {

		conf = BYPASS_LOAD_PA(a);
		vendor = amba_vendor(conf);
		device = amba_device(conf);
		version = amba_ver(conf);

		if ((d->c->conf[typ] == -1) &&
		    (d->c->vendor == vendor) &&
		    (d->c->device == device) && (d->c->version == version)) {
			d->c->conf[typ] = a;
			d->c->conf_b[typ] = b;
			d->c->conf_freq_khz[typ] = f;
			d->c->irq[typ] = amba_irq(conf);
			BIT_SET(d, id);

			/*DBG_PRINTF_5(" p %x:%x %x:%x\n",typ, id, vendor, device); */
		}
	}
	return 0;
}

#ifdef DO_CACHING
int last_id = -1;
struct amba_core last_c;
#endif

/*
  name:         Device Name char string, variable length. If name not found
                a Hex string is created VENDOR_DEVICE, where VENDOR is a hex
                string of fixed length 2 chars, and DEVICE is a hex string of
                fixed length 3 chars, example: 01_045
  vendor:       1 word vendor number
  device:       1 word device number
  version:      1 word version number
  device_mask:  1 word device present bit mask: AHB-MST=0x4, AHB-SLV=0x2, APB-SLV=0x1
  interrupts:   0..3 words IRQ numbers:
                IDX=0: APB-SLV IRQ PnP Field (if IRQ non-zero)
		IDX=1: AHB-SLV IRQ PnP Field (if IRQ non-zero)
		IDX=2: AHB-MST IRQ PnP Field (if IRQ non-zero)
  reg:          APB-SLV I/O space and AHB-SLV Memory and I/O space and mask.
                0 to 5 address/mask pairs:
                IDX=0: APB-SLV I/O Space (if present)
		IDX=1: AHB-SLV MEM or I/O Space [0] (if present)
		IDX=2: AHB-SLV MEM or I/O Space [1] (if present)
		IDX=3: AHB-SLV MEM or I/O Space [2] (if present)
		IDX=4: AHB-SLV MEM or I/O Space [3] (if present)
   userdef:     Only present if AHB-MST or AHB-SLV is present
                IDX=0..2: 3 x words AHB-SLV User-Def PnP words
                IDX=3..5: 3 x words AHB-MST User-Def PnP words
   freq:        Frequency of the AMBA devices, for a core having multiple
                interfaces the frequency might be different for different
		devices, the frequency is represented by one 32-bit word. The
		frequency is given in Hz.
		IDX=0: APB-SLV Frequency (present only if APB-SLV is present)
		IDX=1: AHB-SLV Frequency (present only if AHB-SLV is present)
		IDX=2: AHB-MST Frequency (present only if AHB-MST is present)
   index:       Core AMBA Plug & Play order Index. Unique Core ID.
   ampopts:     AMP options, used to tell Linux drivers AMP spcific options.
                The 32-bit option are interpreted differently depending on
                driver, some default values exists:
                 - non-existing property = no options, device can be used
                 - zero = device should not be given to driver at all
                 - non-zero = let driver interpret value, see driver manual
*/

/* <node> parameter: if msb is set the lowword is an index of
   an amba_core in scanorder. Otherwise it is an index into the romstruct's nodes */
int _find_core(int node, struct amba_core *a)
{
	int id = node & 0x7fff, v, d;
	char *n;
	struct amp_opt *_amp = (struct amp_opt *)__va(&amp);
	struct gaisler_cores *gc = (struct gaisler_cores *)__va(&gcores);

	int irqcnt = 0;
	int i, j, nextid = 0;
	struct find_data f;
	if (!(node & 0x80000000)) {
		return 0;
	}
#ifdef DO_CACHING
	/* speed up using last cached node */
	if ((*(int *)__va(&last_id)) == id) {
		no_inline_memcpy((char *)a, (char *)__va(&last_c),
				 sizeof(last_c));
		return 1;
	}
#endif

	memset(&f, 0, sizeof(f));
	f.c = a;

	/* search for amba_core index id: free-and-pair <id> times, last retreved obj is the desired one */
	for (i = 0; i <= id; i++) {
		struct amba_scan_io io;
		io.p = 0;
		io.ioarea = LEON3_IO_AREA;
		memset(a, 0, sizeof(struct amba_core));
		memset(a->conf, -1, sizeof(a->conf));
		nextid = 0;
		amba_scan(&io, *(unsigned int *)__va(&spi.freq_khz), 1, &nextid,
			  (amba_call) __va(find_core_free), &f);
		if (a->conf[0] == -1 && a->conf[1] == -1 && a->conf[2] == -1) {
			return 0;
		}
		nextid = 0;
		amba_scan(&io, *(unsigned int *)__va(&spi.freq_khz), 1, &nextid,
			  (amba_call) __va(find_core_pair), &f);
	}

	/* property name */
	/*a->name.name = STR_VA("name"); */
	a->name.value = n = (char *)&a->_name;
	while (gc->device != 0) {
		if (a->vendor == VENDOR_GAISLER && a->device == gc->device) {
			char *n = STR_VA(gc->n);
			no_inline_memcpy((char *)&a->_name, n,
					 lo_strnlen(n, 128) + 1);
			break;
		}
		gc++;
	}

	if (!gc->device) {
		v = a->vendor;
		d = a->device;
#define DIGIT(n,v) *n++ = (((v) & 0xf) > 9 ? ((v) & 0xf) - 10 + 'a' :  ((v) & 0xf) + '0')
		DIGIT(n, (v >> 4));
		DIGIT(n, (v >> 0));
		*n++ = '_';
		DIGIT(n, (d >> 8));
		DIGIT(n, (d >> 4));
		DIGIT(n, (d >> 0));
		*n++ = 0;
	}
	a->name.length = lo_strnlen(a->name.value, 128) + 1;

	/* property vendor */
	/*a->vendor_p.name = STR_VA("vendor"); */
	a->vendor_p.value = (char *)&(a->vendor);
	a->vendor_p.length = 4;

	/* property device */
	/*a->device_p.name = STR_VA("device"); */
	a->device_p.value = (char *)&(a->device);
	a->device_p.length = 4;

	/* property version */
	/*a->version_p.name = STR_VA("version"); */
	a->version_p.value = (char *)&(a->version);
	a->version_p.length = 4;

	/* property regs */
	a->regscnt = 0;
	a->userdefcnt = 0;
	for (i = 0; i < 3; i++) {
		if (a->conf[i] != -1) {
			unsigned int mbar, iobar, cfg_area, addr, type, mask;
			a->device_mask |= 1 << i;
			a->freq[a->freq_cnt++] =
			    int_mul(a->conf_freq_khz[i], 1000);
			if (a->irq[i] != 0)
				a->irq[irqcnt++] = a->irq[i];
			if (i == APB_SLAVE) {
				iobar = BYPASS_LOAD_PA(a->conf[i] + 4);
				a->regs[a->regscnt++] =
				    amba_iobar_start(a->conf_b[i], iobar);
				a->regs[a->regscnt++] = amba_apb_mask(iobar);
			} else {
				cfg_area = a->conf[i];
				for (j = 1; j < 4; j++) {
					mbar =
					    BYPASS_LOAD_PA(cfg_area +
							   ((j) * 4));
					a->userdef[a->userdefcnt++] = mbar;
				}
				for (j = 0; j < 4; j++) {

					mbar =
					    BYPASS_LOAD_PA(cfg_area +
							   ((j + 4) * 4));

					addr = amba_membar_start(mbar);
					type = amba_membar_type(mbar);
					if (type == AMBA_TYPE_AHBIO) {
						addr =
						    amba_ahbio_adr(addr,
								   a->
								   conf_b[i] &
								   0xfff00000);
						mask = (((unsigned int)
							 (amba_membar_mask
							  ((~mbar)) << 8) |
							 0xff)) + 1;
					} else {
						/* AHB memory area, absolute address */
						mask = (~((unsigned int)
							  (amba_membar_mask
							   (mbar) << 20))) + 1;
					}
					if (mbar) {
						a->regs[a->regscnt++] = addr;
						a->regs[a->regscnt++] = mask;
					}
				}
			}
		}
	}

	/* property interrupts */
	/*a->interrupts.name = STR_VA("interrupts"); */
	a->interrupts.value = (char *)&(a->irq[0]);
	a->interrupts.length = sizeof(int) * irqcnt;

	/*a->reg.name = STR_VA("reg"); */
	a->reg.value = (char *)&(a->regs[0]);
	a->reg.length = a->regscnt * sizeof(int);

	/* property device_nask */
	/*a->device_mask_p.name = STR_VA("device_mask"); */
	a->device_mask_p.value = (char *)&(a->device_mask);
	a->device_mask_p.length = 4;

	/* property userdef */
	/*a->userdef_p.name = STR_VA("userdef"); */
	a->userdef_p.value = (char *)&(a->userdef);
	a->userdef_p.length = a->userdefcnt * 4;

	/* property freq */
	/*a->freq_p.name = STR_VA("freq"); */
	a->freq_p.value = (char *)&(a->freq);
	a->freq_p.length = a->freq_cnt * sizeof(int);

	/* property index */
	a->index = id;
	/*a->index_p.name = STR_VA("index"); */
	a->index_p.value = (char *)&(a->index);
	a->index_p.length = 4;

	/* property ampopts */
	/*a->ampopts_p.name = STR_VA("ampopts"); */
	a->ampopts_p.value = (char *)&(a->ampopts);
	a->ampopts_p.length = 0;

	while (_amp->idx != -1) {
		if (_amp->idx == id) {
			a->ampopts = _amp->val;
			a->ampopts_p.length = 4;
		}
		_amp++;
	}

	/*
	   DBG_PRINTF_2("n: %s\n",a->name.value);
	   for(i = 0; i < a->regscnt; i+=2) {
	   DBG_PRINTF_3(" 0x%x:0x%x\n",a->regs[i],a->regs[i+1]);
	   } */

#ifdef DO_CACHING
	/* cache last node */
	*(int *)__va(&last_id) = id;
	no_inline_memcpy((char *)__va(&last_c), (char *)a, sizeof(last_c));
	a = __va(&last_c);
	a->name.value = n = (char *)&a->_name;
	a->vendor_p.value = (char *)&(a->vendor);
	a->device_p.value = (char *)&(a->device);
	a->version_p.value = (char *)&(a->version);
	a->interrupts.value = (char *)&(a->irq[0]);
	a->reg.value = (char *)&(a->regs[0]);
	a->device_mask_p.value = (char *)&(a->device_mask);
	a->userdef_p.value = (char *)&(a->userdef);
	a->freq_p.value = (char *)&(a->freq);
	a->index_p.value = (char *)&(a->index);
	a->ampopts_p.value = (char *)&(a->ampopts);
#endif

	return 1;
}

int _find_name_idx(char *name)
{
	int idx = 0;
	char **pp = __va(pnames);
	while (*pp) {
		if (!no_inline_strcmp(name, __va(*pp))) {
			return idx;
		}
		idx++;
		pp++;
	}
	return -1;
}

static struct short_property *find_property(int node, char *name,
					    struct short_property *p)
{
	struct short_property *prop, *r;
	struct amba_core c;
	struct amba_short_property *ap;

	memset(p, 0, sizeof(*p));

	/*DBG_PRINTF_3(" =>find_property(0x%x,\"%s\")\n",node, name); */

	/* ambapp case: use return value as bool */
	if (find_core(node, &c)) {
		int idx;
		if (-1 != (idx = _find_name_idx(name))) {
			ap = &(((struct amba_short_property *)&c.name)[idx]);
			p->name = 0;
			p->value = ap->value;
			p->length = ap->length;
		} else {
			return 0;
		}

		/*DBG_PRINTF_1(" =>found\n"); */
		return p;
	}

	/* standard case: use return value in nextprop */
	prop = &bnodes[node].properties[0];

	while (prop && prop->name) {
		if (no_inline_strcmp(prop->name, name) == 0) {
			no_inline_memcpy((void *)p, (void *)prop, sizeof(*p));
			return prop;
		}
		prop++;
	}
	return NULL;
}

static int no_nextnode(int node)
{
	struct amba_core c;
	/*DBG_PRINTF_2("+nn(0x%x)\n",node); */
	if (find_core(node, &c)) {
		int n2 = ((node & 0x7fff) + 1) | 0x80000000;
		n2 = (find_core(n2, &c)) ? n2 : -1;
		return n2;
	}

	if (bnodes[node].level == bnodes[node + 1].level)
		return node + 1;
	return -1;
}

static int no_child(int node)
{
	struct amba_core c;
	/*DBG_PRINTF_2("+c(0x%x)\n",node); */
	if (find_core(node, &c)) {
		return -1;
	}
	if (node == 4) {
		return 0x80000000;
	}
	if (bnodes[node].level == bnodes[node + 1].level - 1)
		return node + 1;
	return -1;
}

static int no_proplen(int node, char *name)
{
	struct short_property prop, *f;
	/*DBG_PRINTF_3("+l(0x%x,%s)\n",node, name); */

	f = find_property(node, name, &prop);
	if (f) {
		/*DBG_PRINTF_2("=0x%x\n",prop.length); */
		return prop.length;
	}

	return -1;
}

static int no_getprop(int node, char *name, char *value)
{
	struct short_property prop, *f;
	/*DBG_PRINTF_3("+g(0x%x,%s)\n",node, name); */

	f = find_property(node, name, &prop);
	if (f) {
		/*DBG_PRINTF_2("=0x%x\n",prop.length); */
		no_inline_memcpy(value, prop.value, prop.length);
		return 1;
	}
	return -1;
}

static int no_setprop(int node, char *name, char *value, int len)
{
	return -1;
}

static char *no_nextprop(int node, char *name)
{
	struct short_property prop, *f;
	struct amba_core c;

	/*DBG_PRINTF_3("+n(0x%x,\"%s\")\n",node,name); */

	if (find_core(node, &c)) {
		int idx;
		char *n = 0;
		if (!name || !name[0]) {
			n = "name";
		} else if (-1 != (idx = _find_name_idx(name))) {
			struct amba_short_property *prop =
			    &(((struct amba_short_property *)&c.name)[idx]);
			do {
				if (idx == PNAMES_LAST)
					return 0;
				idx++;
				prop++;
			} while (!prop->length);

			n = ((char **)__va(&pnames))[idx];
		} else {
			return 0;
		}

		/*DBG_PRINTF_2("=\"%s\"\n",STR_VA(n)); */
		return n ? __va(n) : 0;
	}
	if (!name || !name[0]) {
		return bnodes[node].properties[0].name;
	}
	f = find_property(node, name, &prop);
	if (f)
		return f[1].name;
	return NULL;
}

/* todo: -...... */
void __loop(char *v)
{
	while (1) ;
}

#define printk __loop

static void leon_reboot(char *bcommand)
{
	while (1) {
		printk(__va("Can't reboot\n"));
	};
}

static void leon_halt(void)
{
	while (1) {
		printk(__va("Halt\n"));
	};
}

/* get single char, don't care for blocking*/
static int leon_nbgetchar(void)
{
	return -1;
}

struct leon3_apbuart_regs_map *uart_addr =
    (struct leon3_apbuart_regs_map *)DEBUG_PROMSTAGE_UART;
/* From Linux kernel 2.6.37 (23bcbf1b63350ed529f7dfb8a5c459e6e0c1a3ca) the
 * return argument interpretation has changed, however fixing this would
 * loose the backward compatability so we still need the blocking write
 * function even though it should be non-blocking.
 *
 * If you have a more recent kernel 
 */
#if LINUX_VERSION_CODE <= KERNEL_VER(2,6,36)
static int leon_nbputchar(int c)
{
	int timeout = 100000, ctrl;
	struct leon3_apbuart_regs_map *b =
	    (struct leon3_apbuart_regs_map *)(*(int *)__va(&uart_addr));
	ctrl = BYPASS_LOAD_PA(&b->ctrl);
	BYPASS_STORE_PA(&b->ctrl, ctrl | LEON_REG_UART_CTRL_TE);
	while (timeout
	       && (BYPASS_LOAD_PA(&b->status) & LEON_REG_UART_STATUS_THE) == 0)
		timeout--;
	if (timeout) {
		BYPASS_STORE_PA(&b->data, c & 0xff);
	}
	BYPASS_STORE_PA(&b->ctrl, ctrl);
	return 0;
}
#else
/* for kernels 2.6.37* and onwards */
static int leon_nbputchar(int c)
{
	int ctrl;
	struct leon3_apbuart_regs_map *b =
	    (struct leon3_apbuart_regs_map *)(*(int *)__va(&uart_addr));

	ctrl = BYPASS_LOAD_PA(&b->ctrl);
	BYPASS_STORE_PA(&b->ctrl, ctrl | LEON_REG_UART_CTRL_TE);

	if ((BYPASS_LOAD_PA(&b->status) & LEON_REG_UART_STATUS_THE) == 0) {
		BYPASS_STORE_PA(&b->ctrl, ctrl);
		return 0; /* comeback again */
	}

	BYPASS_STORE_PA(&b->data, c & 0xff);
	BYPASS_STORE_PA(&b->ctrl, ctrl);
	return 1; /* One char transmitted */
}
#endif

static inline void mark()
{
	__asm__ __volatile__("sethi	%%hi(%0), %%l0    \n\t"
			     "st    %%g0,[%%lo(%0)+%%l0]\n\t"::"i"
			     (LEONSETUP_MEM_BASEADDR):"l0");
}

static inline void set_cache(unsigned long regval)
{
	asm volatile ("sta %0, [%%g0] %1\n\t"::"r" (regval), "i"(2):"memory");
}

static inline void load_tbr(unsigned long regval)
{
	asm volatile ("mov %0, %%tbr\n\t"::"r" (regval):"memory");
}

extern unsigned short bss_start, bss_end;

extern void (*prom_build_more) (struct device_node * dp,
				struct device_node *** nextp);

void leon_flush_cache_all(void)
{
	__asm__ __volatile__(" flush ");	/*iflush */
	__asm__ __volatile__("sta %%g0, [%%g0] %0\n\t"::
			     "i"(ASI_LEON_DFLUSH):"memory");
}

void leon_flush_tlb_all(void)
{
	leon_flush_cache_all();
	__asm__ __volatile__("sta %%g0, [%0] %1\n\t"::"r"(0x400),
			     "i"(ASI_LEON_MMUFLUSH):"memory");
}

/* callback for _amba_scan: get freq and number of cores and init romstruct with it, baseaddress is still mapped 1-1 */
int leon_prom_amba_init(unsigned int a, unsigned int b, unsigned int f, int id,
			int typ, struct find_data *d)
{
	unsigned int conf;
	unsigned int j, vendor, device;
	unsigned int mbar, iobar;
	conf = BYPASS_LOAD_PA(a);
	iobar = BYPASS_LOAD_PA(a + 4);
	vendor = amba_vendor(conf);
	device = amba_device(conf);
	if (typ == APB_SLAVE &&
	    vendor == VENDOR_GAISLER && device == GAISLER_GPTIMER) {
		struct leon3_gptimer_regs_map *regs =
		    (struct leon3_gptimer_regs_map *)amba_iobar_start(b, iobar);
		if (!d->found_timer) {
			d->found_timer = 1;
			if (regs) {
				if (!
				    (spi.freq_khz =
				     int_mul(BYPASS_LOAD_PA
					     (&(regs->scalar_reload)) + 1,
					     1000))) {
					if (!
					    (spi.freq_khz =
					     (BOOTLOADER_freq / 1000)))
						spi.freq_khz = (40 * 1000);
					spi.freq_khz = (40 * 1000);
					/* DBG_PRINTF_2("Found timer freq 0x%x\n", spi.freq_khz); */
				}
			}
		}
	} else if (typ == APB_SLAVE &&
		   vendor == VENDOR_GAISLER && device == GAISLER_IRQMP) {
		struct leon3_irqctrl_regs_map *regs =
		    (struct leon3_irqctrl_regs_map *)amba_iobar_start(b, iobar);
		if (!d->found_mpirq) {
			d->found_mpirq = 1;
#ifdef CONFIG_SMP
			j = 1;
			if ((regs)) {
				j = ((BYPASS_LOAD_PA(&(regs->mpstatus)) >>
				      LEON3_IRQMPSTATUS_CPUNR) & 0xf) + 1;
			}
			spi.nodes[4 + j].level = -1;
			spi.nodes[4 + j].properties =
			    __va(spi.root_properties + 3);
			/* DBG_PRINTF_2("Found mpirq 0x%x cpus\n",j); */
#endif
		}
#if 0 /* Linux clears IRQ controller in newer kernels, besides must clear only for booting cpu. */
		/* quite irq */
		if (regs)
			BYPASS_STORE_PA(&regs->mask[0], 0);
#endif

	} else if (typ == APB_SLAVE &&
		   vendor == VENDOR_GAISLER && device == GAISLER_APBUART) {
		struct leon3_apbuart_regs_map *regs =
		    (struct leon3_apbuart_regs_map *)amba_iobar_start(b, iobar);
		if (!d->found_uart) {
			d->found_uart = 1;
			if (regs) {
				uart_addr = regs;
				/* DBG_PRINTF_2("Found uart 0x%x \n",regs); */
			}
		}
	}

	return 0;
}

static inline void leon_prom_init()
{
	unsigned long i;
	unsigned char cksum, *ptr;

	unsigned long memctrl1 = 0;
	unsigned long memctrl2 = 0;
	unsigned long sp, banks = 1;

#define GETREGSP(sp) __asm__ __volatile__("mov %%sp, %0" : "=r" (sp))

	GETREGSP(sp);

	/* other sheme? Note that leon_prom_amba_init will overwrite with scalar reload of first timer */
	if (!(spi.freq_khz = (BOOTLOADER_freq / 1000)))
		spi.freq_khz = (40 * 1000);

#ifndef CONFIG_SMP
	{
		int j = 1;
		spi.nodes[4 + j].level = -1;
		spi.nodes[4 + j].properties = __va(spi.root_properties + 3);
	}
#endif

	/* figure out ram size */
	spi.totphys.num_bytes = 0;

	sp = sp - LEONSETUP_MEM_BASEADDR;
	spi.totphys.num_bytes = (sp + 0x1000) & ~(0xfff);

	spi.avail.num_bytes = spi.totphys.num_bytes;

	ptr = (unsigned char *)&spi.idprom;
	for (i = cksum = 0; i <= 0x0E; i++)
		cksum ^= *ptr++;
	spi.idprom.id_cksum = cksum;

	/* Get AMBA System ID */
	spi.amba_systemid = *(unsigned long *)
				(LEON3_IO_AREA | LEON3_CONF_AREA | 0xff0);
}

/* mark as section .img.main.text, to be referenced in linker script */
int __attribute__ ((__section__(".img.main.text"))) __main(void)
{

	char *c;
	unsigned long long *l;
	int nextid = 0;
	struct find_data f;
	void (*kernel) (struct linux_romvec *);
	int id = 0;
#ifdef CONFIG_SMP
	id = hard_smpleon_processor_id();
#endif

	/* disable mmu */
	srmmu_set_mmureg(0x00000000);
	__asm__ __volatile__("flush\n\t");

	/* virtual function pointers */
	_amba_scan_p = (_amba_scan_fn) __va((int)_amba_scan);
	_find_core_p = (_find_core_fn) __va((int)_find_core);
	_find_name_idx_p = (_find_name_idx_fn) __va((int)_find_name_idx);
	_no_inline_memcpy_p =
	    (_no_inline_memcpy_fn) __va((int)_no_inline_memcpy);
	_no_inline_strcpy_p =
	    (_no_inline_strcpy_fn) __va((int)_no_inline_strcpy);
	_no_inline_memset_p =
	    (_no_inline_memset_fn) __va((int)_no_inline_memset);
	_no_inline_strcmp_p =
	    (_no_inline_strcmp_fn) __va((int)_no_inline_strcmp);

#ifdef DEBUG_PROMSTAGE
	_dbg_printf_p = (_dbg_printf_fn) __va((int)_dbg_printf);
#endif

	/*set_cache(0); */

	if (id == 0) {
		/* clear bss using std when possible, bss may end on byte
		 * boundary
		 */
		l = (unsigned long long *)&bss_start;
#if 0 /* Temporary removed because of space needed */
		if (((unsigned int)l & 0x7) == 0) {
			while (l < (void *)((unsigned int)&bss_end & ~0x7)) {
				*l = 0;
				l++;
			}
		}
#endif
		/* clear last bytes */
		c = (char *)l;
		while (c < (char *)&bss_end) {
			*c = (char)0;
			c++;
		}

		/* init prom info struct */
		leon_prom_init();

#if  (CONFIG_KERNEL_ROOTMEM_ROMFS  == 1)
		/* boot options */

		root_dev = 0x100;	/* HACK: was Root_RAM0; */
		root_flags = 0x0800 | RAMDISK_LOAD_FLAG;
		root_mountflags |= MS_RDONLY;

		sparc_ramdisk_image = (unsigned long)&initrd_start
		    - LEONSETUP_MEM_BASEADDR;
		sparc_ramdisk_size = &initrd_end - &initrd_start;
#endif
		/* mark as used for bootloader */
#ifndef CONFIG_SMP
		mark();
#endif
	}
#ifdef CONFIG_SMP
	//sparc_leon3_disable_cache();
	sparc_leon3_enable_snooping();
#endif

	/* turn on mmu */
	extern unsigned long _bootloader_ph;
	srmmu_set_ctable_ptr((int)&_bootloader_ph
			     /*LEONSETUP_MEM_BASEADDR + PAGE_SIZE */ );
	srmmu_set_context(0);
	__asm__ __volatile__("flush\n\t");
	srmmu_set_mmureg(0x00000001 /*| (CONFIG_PAGE_SIZE_LEON << 16) */ );
	leon_flush_tlb_all();
	void leon_flush_cache_all();

	if (id == 0) {
		struct amba_scan_io io;
		io.p = 0;
		io.ioarea = LEON3_IO_AREA;
		/* get timer freq and cpu count */
		_no_inline_memset(&f, 0, sizeof(f));
		_amba_scan(&io, 0, 0, &nextid, (amba_call) leon_prom_amba_init,
			   &f);
	}

	/* call kernel */
	kernel = (void (*)(struct linux_romvec *))KERNBASE + LOAD_ADDR;
	load_tbr((int)kernel);
	kernel(__va(&spi.romvec));

	return 1;
}
