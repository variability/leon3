GRLIB=../..
TOP=leon3mp
BOARD=xilinx-ml510-xc5vfx130t
include $(GRLIB)/boards/$(BOARD)/Makefile.inc
DEVICE=$(PART)-$(PACKAGE)$(SPEED)
UCF=leon3mp_mig.ucf
XSTOPT="-uc leon3mp.xcf"
QSF=$(GRLIB)/boards/$(BOARD)/$(TOP).qsf
EFFORT=high
ISEMAPOPT="-m -timing"
SYNPOPT="set_option -pipe 1; set_option -retiming 1; set_option -write_apr_constraint 0"
VHDLOPTSYNFILES= \
	mig_36_1/user_design/rtl/ddr2_chipscope.vhd \
	mig_36_1/user_design/rtl/ddr2_ctrl.vhd \
	mig_36_1/user_design/rtl/ddr2_idelay_ctrl.vhd \
	mig_36_1/user_design/rtl/ddr2_infrastructure.vhd \
	mig_36_1/user_design/rtl/ddr2_mem_if_top.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_calib.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_ctl_io.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_dm_iob.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_dq_iob.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_dqs_iob.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_init.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_io.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_top.vhd \
	mig_36_1/user_design/rtl/ddr2_phy_write.vhd \
	mig_36_1/user_design/rtl/ddr2_top.vhd \
	mig_36_1/user_design/rtl/ddr2_usr_addr_fifo.vhd \
	mig_36_1/user_design/rtl/ddr2_usr_rd.vhd \
	mig_36_1/user_design/rtl/ddr2_usr_top.vhd \
	mig_36_1/user_design/rtl/ddr2_usr_wr.vhd \
	mig_36_1/user_design/rtl/mig_36_1.vhd 
VHDLSYNFILES= config.vhd \
	ahb2mig_ml510.vhd ahbrom.vhd \
	svga2ch7301c.vhd leon3mp.vhd
VHDLSIMFILES=testbench.vhd
SIMTOP=testbench
SDCFILE=default.sdc
BITGEN=$(GRLIB)/boards/$(BOARD)/default.ut
CLEAN=soft-clean

include $(GRLIB)/software/leon3/Makefile
XLDFLAGS=-L./ lib3tests.a -Ttext=0x40000000

TECHLIBS = unisim
LIBSKIP = core1553bbc core1553brm core1553brt gr1553 corePCIF \
	tmtc openchip ihp gleichmann usbhc spw micron
DIRSKIP = b1553 pcif leon2 leon2ft crypto satcan can ata leon3ft \
	grusbhc usb spacewire hcan leon4 leon4b64 l2cache \
	slink ascs pwm haps coremp7 gr1553b iommu ac97 ambatest atf

FILESKIP = grcan.vhd simple_spi_top.v

include $(GRLIB)/bin/Makefile


##################  project specific targets ##########################

mig_36_1 mig:
	cp mig_rdimm.prj mig.prj
	coregen -b mig.xco -p .
	patch -p0 < mig.diff

migclean:
	-rm -rf mig_36* tmp coregen.log

