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

#define u8 unsigned char 
#define s8 signed char 
#define u32 unsigned int 
#define s32 signed int 

struct leon3_irqctrl_regs_map {
 u32 ilevel;
 u32 ipend;
 u32 iforce;
 u32 iclear;
 u32 mpstatus;
 u32 mpbroadcast;
 u32 notused02;
 u32 notused03;
 u32 ampctrl;
 u32 icsel[2];
 u32 notused13;
 u32 notused20;
 u32 notused21;
 u32 notused22;
 u32 notused23;
 u32 mask[16];
 u32 force[16];

 u32 intid[16];
 u32 unused[(0x1000-0x100)/4];
};

struct leon3_apbuart_regs_map {
 u32 data;
 u32 status;
 u32 ctrl;
 u32 scaler;
};

struct leon3_gptimerelem_regs_map {
 u32 val;
 u32 rld;
 u32 ctrl;
 u32 unused;
};

struct leon3_gptimer_regs_map {
 u32 scalar;
 u32 scalar_reload;
 u32 config;
 u32 unused;
 struct leon3_gptimerelem_regs_map e[8];
};

struct idprom {
 u8 id_format;
 u8 id_machtype;
 u8 id_ethaddr[6];
 s32 id_date;
 u32 id_sernum:24;
 u8 id_cksum;
 u8 reserved[16];
};

struct linux_nodeops {
 int (*no_nextnode)(int node);
 int (*no_child)(int node);
 int (*no_proplen)(int node, const char *name);
 int (*no_getprop)(int node, const char *name, char *val);
 int (*no_setprop)(int node, const char *name, char *val, int len);
 char * (*no_nextprop)(int node, char *name);
};

struct linux_mlist_v0 {
 struct linux_mlist_v0 *theres_more;
 unsigned int start_adr;
 unsigned num_bytes;
};

struct linux_mem_v0 {
 struct linux_mlist_v0 **v0_totphys;
 struct linux_mlist_v0 **v0_prommap;
 struct linux_mlist_v0 **v0_available;
};

struct linux_bootargs_v2 {
 char **bootpath;
 char **bootargs;
 int *fd_stdin;
 int *fd_stdout;
};

struct linux_arguments_v0 {
 char *argv[8];
 char args[100];
 char boot_dev[2];
 int boot_dev_ctrl;
 int boot_dev_unit;
 int dev_partition;
 char *kernel_file_name;
 void *aieee1;
};

struct linux_dev_v0_funcs {
 int (*v0_devopen)(char *device_str);
 int (*v0_devclose)(int dev_desc);
 int (*v0_rdblkdev)(int dev_desc, int num_blks, int blk_st, char *buf);
 int (*v0_wrblkdev)(int dev_desc, int num_blks, int blk_st, char *buf);
 int (*v0_wrnetdev)(int dev_desc, int num_bytes, char *buf);
 int (*v0_rdnetdev)(int dev_desc, int num_bytes, char *buf);
 int (*v0_rdchardev)(int dev_desc, int num_bytes, int dummy, char *buf);
 int (*v0_wrchardev)(int dev_desc, int num_bytes, int dummy, char *buf);
 int (*v0_seekdev)(int dev_desc, long logical_offst, int from);
};

struct linux_dev_v2_funcs {
 int (*v2_inst2pkg)(int d);
 char * (*v2_dumb_mem_alloc)(char *va, unsigned sz);
 void (*v2_dumb_mem_free)(char *va, unsigned sz);


 char * (*v2_dumb_mmap)(char *virta, int which_io, unsigned paddr, unsigned sz);
 void (*v2_dumb_munmap)(char *virta, unsigned size);

 int (*v2_dev_open)(char *devpath);
 void (*v2_dev_close)(int d);
 int (*v2_dev_read)(int d, char *buf, int nbytes);
 int (*v2_dev_write)(int d, char *buf, int nbytes);
 int (*v2_dev_seek)(int d, int hi, int lo);


 void (*v2_wheee2)(void);
 void (*v2_wheee3)(void);
};

struct device_node;

struct linux_romvec {

 unsigned int pv_magic_cookie;
 unsigned int pv_romvers;
 unsigned int pv_plugin_revision;
 unsigned int pv_printrev;
 struct linux_mem_v0 pv_v0mem;
 struct linux_nodeops *pv_nodeops;
 char **pv_bootstr;
 struct linux_dev_v0_funcs pv_v0devops;
 char *pv_stdin;
 char *pv_stdout;
 int (*pv_getchar)(void);
 void (*pv_putchar)(int ch);
 int (*pv_nbgetchar)(void);
 int (*pv_nbputchar)(int ch);
 void (*pv_putstr)(char *str, int len);
 void (*pv_reboot)(char *bootstr);
 void (*pv_printf)(__const__ char *fmt, ...);
 void (*pv_abort)(void);
 __volatile__ int *pv_ticks;
 void (*pv_halt)(void);
 void (**pv_synchook)(void);
 union {
  void (*v0_eval)(int len, char *str);
  void (*v2_eval)(char *str);
 } pv_fortheval;

 struct linux_arguments_v0 **pv_v0bootargs;
 unsigned int (*pv_enaddr)(int d, char *enaddr);
 struct linux_bootargs_v2 pv_v2bootargs;
 struct linux_dev_v2_funcs pv_v2devops;

 int filler[15];

 void (*pv_setctxt)(int ctxt, char *va, int pmeg);
 int (*v3_cpustart)(unsigned int whichcpu, int ctxtbl_ptr,
		    int thiscontext, char *prog_counter);
 int (*v3_cpustop)(unsigned int whichcpu);
 int (*v3_cpuidle)(unsigned int whichcpu);
 int (*v3_cpuresume)(unsigned int whichcpu);
};

#undef u8
#undef s8
#undef u32
#undef s32

static inline __attribute__((always_inline)) void srmmu_set_mmureg(unsigned long regval)
{
 __asm__ __volatile__("sta %0, [%%g0] %1\n\t" : :
        "r" (regval), "i" (0x19) : "memory");

}

static inline __attribute__((always_inline)) void srmmu_set_ctable_ptr(unsigned long paddr)
{
 paddr = ((paddr >> 4) & 0xfffffff0);
 __asm__ __volatile__("sta %0, [%1] %2\n\t" : :
        "r" (paddr), "r" (0x00000100),
        "i" (0x19) :
        "memory");
}

static inline __attribute__((always_inline)) void srmmu_set_context(int context)
{
 __asm__ __volatile__("sta %0, [%1] %2\n\t" : :
        "r" (context), "r" (0x00000200),
        "i" (0x19) : "memory");
}

static inline __attribute__((always_inline)) void sparc_leon3_enable_snooping(void)
{
 __asm__ __volatile__ ("lda [%%g0] 2, %%l1\n\t"
     "set 0x800000, %%l2\n\t"
     "or  %%l2, %%l1, %%l2\n\t"
     "sta %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
};

extern inline int hard_smpleon_processor_id(void)
{
        int cpuid;
        __asm__ __volatile__("rd     %%asr17,%0\n\t"
                             "srl    %0,28,%0" :
                             "=&r" (cpuid) : );
        return cpuid;
}
