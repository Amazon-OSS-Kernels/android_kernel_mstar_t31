/*
 * acs_ipc_agent_driver.h
 *
 * dsp ipc driver header file for platforms
 *
 * Copyright 2023 Amazon.com, Inc. or its affiliates. All rights reserved.
 * Xueliang Li        (xuelianq@amazon.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ACS_IPC_AGENT_DRIVER_H
#define __ACS_IPC_AGENT_DRIVER_H

#define MT8570_ADSP_ID                      (0x85708570)
#define DSP0_REG_BASE                       (0x1D062000)
#define DSP0_GRP1F                          (DSP0_REG_BASE + 0x00ac)
#define ADSP_CORE_0_COMMON_INFO_ADDR        (DSP0_GRP1F)

typedef void (*aceAud_ipc_irq_cb_t)(void);
typedef void (*aceAud_ipc_wdt_cb_t)(void);

typedef struct {
    int32_t (*remote_write)(uint32_t addr, uint8_t *data, uint32_t len);
    int32_t (*remote_read)(uint32_t addr, uint8_t *data, uint32_t len);
    void (*register_ipc_irq_cb)(aceAud_ipc_irq_cb_t cb);
    void (*ipc_irq_resume)(void);
    void (*ipc_irq_suspend)(void);
    void (*ipc_read_register)(void *data);
    void (*register_ipc_wdt_cb)(aceAud_ipc_wdt_cb_t cb);
} aceAudioIPC_HW_API_t;

void ipc_agent_wdt_handle(void);
void ipc_agent_set_wdt_triggered(void);
void ipc_agent_set_wdt_untriggered(void);

#endif // __ACS_IPC_AGENT_DRIVER_H
