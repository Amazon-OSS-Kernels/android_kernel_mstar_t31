#
# Copyright (C) 2019 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

ifneq ($(filter y,$(CONFIG_MTK_HIFI4DSP_SUPPORT)),)
### include
ccflags-y += \
-I$(srctree)/drivers/mstar2/drv/hifi4dsp/hifi4dsp_ipi/mt8570 \
-I$(srctree)/drivers/mstar2/drv/hifi4dsp/hifi4dsp_ipi/common
obj-y				+= hifi4dsp_ipi_i2c.o
endif
