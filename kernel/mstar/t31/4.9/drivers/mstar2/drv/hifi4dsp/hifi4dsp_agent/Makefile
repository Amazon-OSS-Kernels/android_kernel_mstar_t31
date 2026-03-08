#
# Copyright (C) 2023 Amazon Inc.
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

EXTRA_CFLAGS      +=  -Idrivers/mstar2/include

subdir-ccflags-y += -I$(src)/common/

ifneq ($(filter y,$(CONFIG_MTK_HIFI4DSP_SUPPORT)),)
subdir-ccflags-y += -I$(src)/mt8570/
obj-y += mt8570/
endif

ifeq ($(CONFIG_AMAZON_DSP_IPC_AGENT), y)
subdir-ccflags-y         +=  -DIPC_AGENT_IRQ_GPIO=10
endif
