/*
 * acs_ipc_agent_driver.c
 *
 * dsp ipc driver for Mstar T31.
 * this file will register for ipc gpio irq here,
 * ipc gpio irq is used to notify the ipc agent rx thread
 * that there is data that needs to be received.
 *
 * Copyright 2023 Amazon.com, Inc. or its affiliates. All rights reserved.
 * Xuelianq Li        (xuelianq@amazon.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/io.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/seq_file.h>
#include <linux/thermal.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/atomic.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/stddef.h>

#include "acs_ipc_agent_driver.h"
#include "mdrv_mstypes.h"
#include "hifi4dsp_load/hifi4dsp_load.h"
#include "hifi4dsp_spi.h"
#include "adf_status.h"
#include "adf_common.h"

#define TAG "IPC AGENT"
/*
 * default ipc agent irq GPIO number ,
 * if not defined IPC_AGENT_IRQ_GPIO in Makefile,
 * then this marco will be used.
 */
#define DEFAULT_IPC_AGENT_IRQ_GPIO      (10)

/*
 * =============================================================================
 *                     private global members
 * =============================================================================
 */
static unsigned int irq_gpio_num;

static struct platform_device *_pdevice = NULL;
/* Used to prevent duplicate init and deinit */
static bool s_irq_init_status = false;

static aceAud_ipc_irq_cb_t s_ipc_irq_cb = NULL;
static aceAud_ipc_wdt_cb_t s_ipc_wdt_cb = NULL;

/* If wdt triggered , s_is_wdt_triggered will set 1 */
static unsigned int s_is_wdt_triggered = 0;

void ipc_agent_set_wdt_triggered(void)
{
    s_is_wdt_triggered = 1;
}

void ipc_agent_set_wdt_untriggered(void)
{
    s_is_wdt_triggered = 0;
}

static void ipc_gpio_irq_handle(void)
{
    if (s_ipc_irq_cb != NULL) {
        s_ipc_irq_cb();
    }
}

/*
 * when watch dog triggered , this function will be called.
 */
void ipc_agent_wdt_handle(void)
{
    if (s_ipc_wdt_cb != NULL) {
        s_ipc_wdt_cb();
    }
}

irqreturn_t mt8570_gpio_irq_handler(int irq, void *dev_id)
{
    ipc_gpio_irq_handle();
    return IRQ_NONE;
}

static int __init mt8570_ipc_probe(struct platform_device *pdev)
{
    int err = -ENODEV;

    if (strstr(saved_command_line, "farfield.dsp.name=mt8570") == NULL) {
        pr_info("mt8570 is not supported\n");
        return -EINVAL;
    }

    pr_notice("%s", __func__);

#if defined(IPC_AGENT_IRQ_GPIO)
    irq_gpio_num = IPC_AGENT_IRQ_GPIO;
#else
    irq_gpio_num = DEFAULT_IPC_AGENT_IRQ_GPIO;
#endif

    /* IRQF_TRIGGER_RISING/IRQF_TRIGGER_FALLING
     * IRQF_TRIGGER_HIGH/IRQF_TRIGGER_LOW
     */
    err = request_gpio_irq(irq_gpio_num, mt8570_gpio_irq_handler, IRQF_TRIGGER_RISING, &pdev->dev);
    if (err != 0) {
        pr_err("%s: failed to request irq %d(err:%d)\n", __func__, irq_gpio_num, err);
        return err;
    }
    s_irq_init_status = true;


    pr_notice("%s finish\n", __func__);

    return 0;
}

static int mt8570_ipc_remove(struct platform_device *pdev)
{
    free_gpio_irq(irq_gpio_num, &pdev->dev);
    s_irq_init_status = false;
    pr_notice("%s\n", __func__);
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id audio_ipc_of_ids[] = {
    { .compatible = "mediatek,mt8570_ipc", },
    {}
};
#endif

static int audio_ipc_pm_resume(struct device *device)
{
    pr_notice("%s is resume!\n",__func__);
    int ret;
    ret = request_gpio_irq(irq_gpio_num, mt8570_gpio_irq_handler, IRQF_TRIGGER_RISING, device);
    if (ret != 0) {
        pr_err(" %s: failed to request irq %d(err:%d)\n", __func__, irq_gpio_num, ret);
    }
    s_irq_init_status = true;

    return 0;
}

static int audio_ipc_pm_suspend(struct device *device)
{
    pr_notice("%s is suspend!\n",__func__);
    free_gpio_irq(irq_gpio_num, device);
    s_irq_init_status = false;
    return 0;
}

static void ipc_agent_gpio_irq_suspend(void)
{
    if ((_pdevice != NULL) && (s_irq_init_status != false)) {
        audio_ipc_pm_suspend(&_pdevice->dev);
    }
}

static void ipc_agent_gpio_irq_resume(void)
{
    if ((_pdevice != NULL) && (s_irq_init_status != true))  {
        audio_ipc_pm_resume(&_pdevice->dev);
    }
}

static int32_t ipc_remote_write(uint32_t addr, uint8_t *data, uint32_t len)
{
#ifdef CONFIG_MTK_HIFI4DSP_WDT_RECOVER_SUPPORT
    if (s_is_wdt_triggered)
        return -EPERM;
#endif
    return dsp_spi_write(addr, data, len, SPI_SPEED_HIGH);
}

static int32_t ipc_remote_read(uint32_t addr, uint8_t *data, uint32_t len)
{
#ifdef CONFIG_MTK_HIFI4DSP_WDT_RECOVER_SUPPORT
    if (s_is_wdt_triggered)
        return -EPERM;
#endif
    return dsp_spi_read(addr, data, len, SPI_SPEED_HIGH);
}

static void ipc_register_ipc_irq_cb(aceAud_ipc_irq_cb_t cb)
{
    s_ipc_irq_cb = cb;
}

static void ipc_register_ipc_wdt_cb(aceAud_ipc_wdt_cb_t cb)
{
    s_ipc_wdt_cb = cb;
}

static void ipc_gpio_irq_resume(void)
{
    ipc_agent_gpio_irq_resume();
}

static void ipc_gpio_irq_suspend(void)
{
    ipc_agent_gpio_irq_suspend();
}

static void ipc_spi_read_register(void * data)
{
    if (data == NULL) {
        ADF_LOG_E(TAG, "[%s]: data ptr is NULL !!!\n", __func__);
        return;
    }
    spi_read_register(ADSP_CORE_0_COMMON_INFO_ADDR, data, SPI_SPEED_HIGH);
}

static const aceAudioIPC_HW_API_t s_hw_api = {
    ipc_remote_write,
    ipc_remote_read,
    ipc_register_ipc_irq_cb,
    ipc_gpio_irq_resume,
    ipc_gpio_irq_suspend,
    ipc_spi_read_register,
    ipc_register_ipc_wdt_cb,
};

const aceAudioIPC_HW_API_t* aceAudioIPC_getHwApi(void)
{
    return &s_hw_api;
}

EXPORT_SYMBOL_GPL(aceAudioIPC_getHwApi);

struct dev_pm_ops const audio_ipc_pm_ops = {
    .suspend = audio_ipc_pm_suspend,
    .resume = audio_ipc_pm_resume,
};

static struct platform_driver audio_ipc_driver_probe = {
    .probe = mt8570_ipc_probe,
    .remove = mt8570_ipc_remove,
    .driver = {
        .name = "MT8570 IPC AGENT",
        .owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = audio_ipc_of_ids,
#endif
        .pm = &audio_ipc_pm_ops,
    },
};

static int __init mt8570_ipc_driver_init(void)
{
    int ret = 0;

    _pdevice = platform_device_alloc("MT8570 IPC AGENT", -1);
    if (!_pdevice) {
        pr_err("%s: platform_device_alloc failed\n", __func__);
        return -ENOMEM;
    }

    ret = platform_device_add(_pdevice);
    if (0 != ret) {
        platform_device_put(_pdevice);
        pr_err("%s: platform_device_add failed, ret = %d\n",
            __func__, ret);
        return ret;
    }

    pr_notice("%s\n", __func__);
    ret = platform_driver_register(&audio_ipc_driver_probe);
    if (ret)
        pr_notice("%s register fail, ret 0x%x!\n", __func__, ret);

    return ret;
}

static void __exit mt8570_ipc_driver_exit(void)
{
    if (_pdevice != NULL) {
        platform_device_unregister(_pdevice);
        _pdevice = NULL;
    }

    platform_driver_unregister(&audio_ipc_driver_probe);
    pr_notice("%s\n", __func__);
}

module_init(mt8570_ipc_driver_init);
module_exit(mt8570_ipc_driver_exit);

MODULE_DESCRIPTION("MT8570 IPC Driver");
