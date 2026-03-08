/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <hifi4dsp_load/hifi4dsp_load.h>

#define DRV_NAME        "mtk-dsp_wdt"

struct mtk_dsp_wdt_dev {
    void __iomem *dsp_wdt_base;;
    unsigned int dsp_wdt_irq_id;
    u32 dsp_wdt_gpio;
};

static struct workqueue_struct *dsp_wdt_queue;
static struct work_struct dsp_wdt_work;
static struct platform_device *gpdev = NULL;

BLOCKING_NOTIFIER_HEAD(wdt_notifier_list);

static void dump_adsp_log_buf(void)
{
    u32 tmp;
    char *log_buf = NULL;
    u32 log_size = 0;
    int ret = 0;
    int i;
    char tmp_buf[512];
    u32 tmp_log_start, tmp_log_size;

    /* disable watchdog */
    spi_read_register(0x1D010000, &tmp, SPI_SPEED_HIGH);
    tmp &= ~(1 << 0);
    tmp |= (0x22000000);
    spi_write_register(0x1D010000, tmp, SPI_SPEED_HIGH);

    spi_read_register(0x1D010000, &tmp, SPI_SPEED_HIGH);

    pr_notice("%s, watchdog mode:0x%x\n", __func__, tmp);

    ret = hifi4dsp_get_log_buf_size(&log_size);
    if (ret) {
        pr_info("Dump DSP log fail, ret=%d\n", ret);
        goto _end;
    }

    log_buf = kzalloc(log_size, GFP_KERNEL);
    if (!log_buf) {
        ret = -ENOMEM;
        goto _end;
    }

    ret = hifi4dsp_get_log_buf(log_buf, log_size);
    if (ret) {
        pr_info("Read log_buf fail, ret=%d\n", ret);
        goto _end;
    }

    pr_info("log_buf size: %d\n", log_size);

    pr_info("==========================\n");
    pr_info("=== dump dsp log start ===\n");
    pr_info("==========================\n");
    for (i = 0, tmp_log_start = 0; i < log_size; i++) {
        if (log_buf[i] == '\n' || i == log_size - 1) {
            tmp_log_size = i - tmp_log_start + 1;
            strncpy(tmp_buf, log_buf + tmp_log_start, tmp_log_size);
            tmp_buf[tmp_log_size] = '\0';
            pr_info("%s", tmp_buf);
            tmp_log_start = i + 1;
        }

    }
    pr_info("==========================\n");
    pr_info("=== dump dsp log end  ====\n");
    pr_info("==========================\n");
_end:
    kfree(log_buf);
    WARN_ON(1);
}

#ifdef CONFIG_MTK_HIFI4DSP_WDT_RECOVER_SUPPORT
static int notify_rst_adsp(struct notifier_block *this,
                 unsigned long code, void *unused)
{
    hifi4dsp_rst();

    return NOTIFY_DONE;
}
#endif

static int
dbg_notify_show_adsp_log(struct notifier_block *this,
                 unsigned long code, void *unused)
{
    if (hifi4dsp_run_status())
        dump_adsp_log_buf();
    else
        pr_info("DSP is not ready!\n");
    return NOTIFY_DONE;
}

static struct notifier_block dbg_show_log_notifier = {
    .notifier_call  = dbg_notify_show_adsp_log,
    .priority       = 100,
};

#ifdef CONFIG_MTK_HIFI4DSP_WDT_RECOVER_SUPPORT
static struct notifier_block adsp_rst_notifier = {
    .notifier_call  = notify_rst_adsp,
    .priority       = 99,
};
#endif

static irqreturn_t mtk_dsp_wdt_isr(int irq, void *dev_id)
{
    u16 sts;
    //check interrupt source
    sts = REG_ADDR((0x000F<<9) + (0x01<<2)) & BIT(8);
    if(sts)
    {
        pr_info("%s() %d.\n", __func__,__LINE__);
        queue_work(dsp_wdt_queue, &dsp_wdt_work);
        //clear INTERRUPT for PAD_PM_GPIO_1
        REG_ADDR((0x000F<<9) + (0x01<<2)) |= BIT(6);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

void hifi4dsp_wdt_handler(void)
{
    blocking_notifier_call_chain(&wdt_notifier_list, 0, NULL);
}

void dsp_wdt_work_handler(struct work_struct *unused)
{
    char data[32], *envp[] = { data, NULL };
    pr_notice("[%s] ADSP happens exception!\n", __func__);

    clr_hifi4dsp_run_status();

    snprintf(data, sizeof(data), "ACTION=DSP_WTD_WHOLE");
    kobject_uevent_env(&gpdev->dev.kobj, KOBJ_CHANGE, envp);
    pr_warning("[%s][DSP_HANG]\n", __func__);

}

int register_adsp_wdt_notifier(struct notifier_block *nb)
{
    return blocking_notifier_chain_register(&wdt_notifier_list, nb);
}

int unregister_adsp_wdt_notifier(struct notifier_block *nb)
{
    return blocking_notifier_chain_unregister(&wdt_notifier_list, nb);
}

static int mtk_dsp_wdt_probe(struct platform_device *pdev)
{

	if (strstr(saved_command_line, "farfield.dsp.name=mt8570") == NULL) {
		pr_info("mt8570 is not supported\n");
		return -EINVAL;
    }

    int err;
    struct mtk_dsp_wdt_dev *mtk_dsp_wdt;

    pr_info("%s() enter.\n", __func__);

    mtk_dsp_wdt = devm_kzalloc(&pdev->dev,
                sizeof(*mtk_dsp_wdt), GFP_KERNEL);
    if (!mtk_dsp_wdt)
        return -ENOMEM;

    //Bootmode as WDT. /*fix should modify in dst for ANND, idx = 7 */
    if (!of_property_read_u32(pdev->dev.of_node, "dsp-wdt-gpio", &mtk_dsp_wdt->dsp_wdt_gpio)) {
    	pr_info("hifi4dsp: dsp-wdt-gpio is %d \n", mtk_dsp_wdt->dsp_wdt_gpio);
    } else {
    	pr_err("hifi4dsp: dsp-wdt-gpio is not defined \n");
    	mtk_dsp_wdt->dsp_wdt_gpio = 7;
    }
    if (!of_property_read_u32(pdev->dev.of_node, "interrupts",&mtk_dsp_wdt->dsp_wdt_irq_id)) {
    	pr_info("hifi4dsp: interrupts is %d \n", mtk_dsp_wdt->dsp_wdt_irq_id);
    } else {
    	pr_err("hifi4dsp: interrupts is not defined \n");
    	mtk_dsp_wdt->dsp_wdt_irq_id = 33;
    }
    err = request_irq(mtk_dsp_wdt->dsp_wdt_irq_id, mtk_dsp_wdt_isr,
        IRQF_TRIGGER_RISING|IRQF_SHARED, DRV_NAME, mtk_dsp_wdt);
    if (err != 0) {
        pr_notice("hifi4dsp: %s: failed to request irq %d(err:%d)\n", __func__, mtk_dsp_wdt->dsp_wdt_irq_id, err);
        return err;
    }
    //set as gpio input mode PAD_PM_GPIO_1 //gpio index 7
    REG_ADDR((0x000F<<9) + (0x01<<2)) &= ~(BIT(1));
    REG_ADDR((0x000F<<9) + (0x01<<2)) &= ~(BIT(2));
    REG_ADDR((0x000F<<9) + (0x01<<2)) |= (BIT(0));
    REG_ADDR((0x000F<<9) + (0x01<<2)) |= BIT(6);
    // set falling trigger
    REG_ADDR((0x000F<<9) + (0x01<<2)) |= BIT(7);
    //Enable interrupt mask
    REG_ADDR((0x000F<<9) + (0x01<<2)) &= ~(BIT(4));


    dsp_wdt_queue = create_singlethread_workqueue("dsp_wdt_kworker");
    INIT_WORK(&dsp_wdt_work, dsp_wdt_work_handler);

    register_adsp_wdt_notifier(&dbg_show_log_notifier);
#ifdef CONFIG_MTK_HIFI4DSP_WDT_RECOVER_SUPPORT
    register_adsp_wdt_notifier(&adsp_rst_notifier);
#endif

   gpdev = pdev;

    return 0;
}

static const struct of_device_id mtk_dsp_wdt_dt_ids[] = {
    { .compatible = "mediatek,hifi4dsp-wdt" },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, mtk_dsp_wdt_dt_ids);

static struct platform_driver mtk_dsp_wdt_driver = {
    .probe        = mtk_dsp_wdt_probe,
    .driver        = {
        .name        = DRV_NAME,
        .of_match_table    = mtk_dsp_wdt_dt_ids,
    },
};

module_platform_driver(mtk_dsp_wdt_driver);

