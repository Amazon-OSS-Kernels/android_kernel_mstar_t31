#
# Makefile for MStar localdimming device drivers.
CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))
ifdef CONFIG_MSTAR_PROJECT_NAME
    CONFIG_MSTAR_PROJECT_NAME := $(subst ",,$(CONFIG_MSTAR_PROJECT_NAME))
endif

#cloud this path need modify by different OS



EXTRA_CFLAGS      += -I$(M)/drv
EXTRA_CFLAGS      += -I$(M)/hal
EXTRA_CFLAGS      += -I$(M)/include
EXTRA_CFLAGS      += -I$(M)/../include
EXTRA_CFLAGS      += -Idrivers/mstar2/include


EXTRA_CFLAGS      +=  -DRED_LION
LD_SRC_PATH       := ldm_86_sys
LD_RELEASE_TAG    := $(shell cd $(LD_SRC_PATH);git describe --always 2>/dev/null)
LD_RELEASE_DATE   := $(shell cd $(LD_SRC_PATH);git log -1 --format="%ci")
LD_BUILD_DATE     := $(shell date)
EXTRA_CFLAGS      += -DLD_RELEASE_NAME=\""commit:$(LD_RELEASE_TAG) release_time:$(LD_RELEASE_DATE) build_time:$(LD_BUILD_DATE)"\"


# build moudle

obj-$(CONFIG_HAS_LDM) += kdrv_ldm_sys.o
kdrv_ldm_sys-objs +=	mdrv_ldm_sys.o
kdrv_ldm_sys-objs +=	mdrv_ldm_stdlib.o
kdrv_ldm_sys-objs +=	mdrv_ldm_config_file.o
kdrv_ldm_sys-objs +=	mdrv_ldm_interrupt.o
kdrv_ldm_sys-objs +=	mdrv_ldm_lock.o



