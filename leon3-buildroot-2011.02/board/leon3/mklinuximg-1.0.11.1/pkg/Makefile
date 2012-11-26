TOP:=$(abspath ../../../..)
-include $(TOP)/common.mk

ifneq ($(strip $(subst ",,$(LB_MKLI_PATH))),)
 MKLI=$(strip $(subst ",,$(LB_MKLI_PATH)))/mklinuximg
else
 MKLI=../mklinuximg
endif

MKLI_FREQ=
MKLI_BASE=
MKLI_CMDLINE=
MKLI_ETHMAC=
MKLI_AMP=
MKLI_SMP=

ifneq ($(LB_MKLI_FREQ),)
 MKLI_FREQ= -freq $(LB_MKLI_FREQ)
endif

ifneq ($(LB_MKLI_BASE),)
 MKLI_BASE= -base $(LB_MKLI_BASE)
endif

ifneq ($(LB_MKLI_CMDLINE),)
 MKLI_CMDLINE= -cmdline $(LB_MKLI_CMDLINE)
endif

ifneq ($(LB_MKLI_ETHMAC),)
 MKLI_ETHMAC= -ethmac $(LB_MKLI_ETHMAC)
endif

ifneq ($(LB_MKLI_IPI_NUM),)
 ifneq ($(LB_MKLI_IPI_NUM),"0")
  MKLI_IPI= -ipi $(LB_MKLI_IPI_NUM)
 endif
endif

.PHONY: install preconfig pre_postconfig postconfig clean build default

default: build

install:
	if [ -f install.sh ]; then \
	  sh install.sh; \
	  fi; \

preconfig:

pre_postconfig:

postconfig:

clean:

# Wrap in the Linux RAM Image using the mklinuximg (the "wrapper") into
# a runnable Linux Image for RAM. GRMON, TSIM or MKPROM can boot this image.
build:
	@if [ ! -f $(MKLI) ]; then \
	 echo ;\
	 echo "!!! Linux mklinuximg is not installed or not in path" ; \
	 echo ; \
	 exit 1; \
	 fi;
	$(MKLI) $(TOP)/output/images/image $(TOP)/output/images/image.ram \
	        $(MKLI_FREQ) $(MKLI_BASE) $(MKLI_CMDLINE) $(MKLI_AMP) \
	        $(MKLI_ETHMAC) $(MKLI_IPI)
