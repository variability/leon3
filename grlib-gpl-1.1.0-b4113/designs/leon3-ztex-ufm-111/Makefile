include .config
GRLIB=../..
TOP=leon3mp
BOARD=ztex-ufm-111
include $(GRLIB)/boards/$(BOARD)/Makefile.inc
DEVICE=$(PART)-$(PACKAGE)$(SPEED)
UCF=$(TOP).ucf
#ISEMAPOPT=-timing
QSF=$(GRLIB)/boards/$(BOARD)/$(TOP).qsf
EFFORT=high
XSTOPT="-uc leon3mp.xcf"
SYNPOPT="set_option -pipe 1; set_option -retiming 1; set_option -write_apr_constraint 0"
# Set ZTEX variable to SDK directory in your shell
# or modify the variable below:
#ZTEX=~/ztex/ztex

VHDLOPTSYNFILES = \
	mig37/mig_37/user_design/rtl/iodrp_controller.vhd \
	mig37/mig_37/user_design/rtl/iodrp_mcb_controller.vhd \
	mig37/mig_37/user_design/rtl/mcb_raw_wrapper.vhd \
	mig37/mig_37/user_design/rtl/mcb_soft_calibration.vhd \
	mig37/mig_37/user_design/rtl/mcb_soft_calibration_top.vhd \
	mig37/mig_37/user_design/rtl/memc3_infrastructure.vhd \
	mig37/mig_37/user_design/rtl/memc3_wrapper.vhd \
	mig37/mig_37/user_design/rtl/mig_37.vhd

VHDLSYNFILES = config.vhd ahbrom.vhd ahb2mig_ztex.vhd leon3mp.vhd

VHDLSIMFILES=testbench.vhd

SIMTOP=testbench
SDCFILE=$(GRLIB)/boards/$(BOARD)/default.sdc
BITGEN=$(GRLIB)/boards/$(BOARD)/default.ut
CLEAN=soft-clean

TECHLIBS = secureip unisim

LIBSKIP = core1553bbc core1553brm core1553brt gr1553 corePCIF \
	tmtc openchip ihp gleichmann usbhc spw
DIRSKIP = b1553 pcif leon2 leon2ft crypto satcan pci leon3ft ambatest can \
	usb grusbhc spacewire ata haps coremp7 ascs slink hcan \
	leon4 leon4b64 l2cache pwm gr1553b iommu
FILEADD = MCB.vhd
FILESKIP = grcan.vhd

include $(GRLIB)/bin/Makefile
include $(GRLIB)/software/leon3/Makefile

##################  project specific targets ##########################

mig:
	coregen -b mig37/mig.xco -p mig37
	patch -p0 < mig.patch

migclean:
	-rm -rf mig37/mig_37* mig37/tmp

sudo-ztex-upload:
	sudo $(ZTEX)/java/FWLoader -c -rf -uf $(TOP).bit

sudo-ztex-upload-fw:
	sudo $(ZTEX)/java/FWLoader -c \
	-uu $(ZTEX)/examples/usb-fpga-1.11/standalone/standalone.ihx \
	-rf -uf $(TOP).bit

ztex-upload:
	$(ZTEX)/java/FWLoader -c -rf -uf $(TOP).bit

ztex-upload-fw:
	$(ZTEX)/java/FWLoader -c \
	-uu $(ZTEX)/examples/usb-fpga-1.11/standalone/standalone.ihx \
	-rf -uf $(TOP).bit

