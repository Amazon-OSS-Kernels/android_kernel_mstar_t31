/*
 * Copyright (C) 2019 MediaTek Inc.
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

#include <adsp_ipi.h>
#include <linux/module.h>		/* needed by all modules */
#include <linux/init.h>			/* needed by module macros */
#include "hifi4dsp_ipi_i2c.h"

unsigned char read_buff_array[IPI_I2C_TRANSFER_MAX_NUMBER];
struct completion read_complete;	/* read transfer stop */

static int check_msg_format(const struct i2c_ipi_msg_t *p_ipi_msg,
			    unsigned int len)
{
	if (p_ipi_msg->magic != IPI_MSG_I2C_MAGIC_NUMBER) {
		pr_notice("magic 0x%x error!!\n", p_ipi_msg->magic);
		return -1;
	}
	if (sizeof(struct i2c_ipi_msg_t) > len) {
		pr_notice("len 0x%x error!!\n", len);
		return -2;
	}
	return 0;
}

/** @ingroup IP_group_i2c_external_function
 * @par Description
 *     Initialize struct mtk_i2c and i2c_msg, then read data from\n
 *     slave device.
 * @param[in]
 *     bus_num: i2c bus number.
 * @param[in]
 *     device_addr: slave device 7bits address.
 * @param[in]
 *     speed_khz: i2c transfer speed.
 * @param[out]
 *     buffer: read data buffer pointer.
 * @param[in]
 *     len: read data length.
 * @return
 *     0, i2c transfer successfully.\n
 *     error code from mtk_i2c_transfer().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If mtk_i2c_transfer() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int mtk_i2c_read(uint8_t bus_num, uint8_t device_addr, uint32_t speed_khz,
		 uint8_t *buffer, uint16_t len)
{
	struct i2c_ipi_msg_t *p_ipi_msg;
	uint32_t ipi_msg_len = 0;
	int send_status = -1;
	uint8_t i = 0;

	p_ipi_msg  = kzalloc(sizeof(struct i2c_ipi_msg_t), GFP_KERNEL);
	if (p_ipi_msg == NULL)
		return -1;

	p_ipi_msg->magic = IPI_MSG_I2C_MAGIC_NUMBER;
	p_ipi_msg->i2c_ipi_mode = READ_MODE_IPI;
	p_ipi_msg->bus_num = bus_num;
	p_ipi_msg->device_addr = device_addr;
	p_ipi_msg->speed_khz = speed_khz;
	p_ipi_msg->read_len = len;
	ipi_msg_len = sizeof(struct i2c_ipi_msg_t);

	if (check_msg_format(p_ipi_msg, ipi_msg_len) != 0) {
		pr_info("[%s], drop msg due to ipi fmt err", __func__);
		goto exit;
	}

	init_completion(&read_complete);

	send_status = adsp_ipi_send(ADSP_IPI_I2C, p_ipi_msg, ipi_msg_len, 0, 0);
	if (send_status == 0) {
		pr_info("[%s], set msg ok\n", __func__);
		if (wait_for_completion_timeout(&read_complete,
						 msecs_to_jiffies(2000))) {
			for (i = 0; i < len; i++)
				buffer[i] = read_buff_array[i];
		} else {
			pr_info("[%s], read wait timeout!\n", __func__);
		}
	} else {
		pr_info("[%s], i2c_ipi_send error %d\n", __func__, send_status);
	}

exit:
	kfree(p_ipi_msg);
	return (send_status == 0) ? 0 : -1;
}

/** @ingroup IP_group_i2c_external_function
 * @par Description
 *     Initialize struct mtk_i2c and i2c_msg, then write data to\n
 *     slave device.
 * @param[in]
 *     bus_num: i2c bus number.
 * @param[in]
 *     device_addr: slave device 7bits address.
 * @param[in]
 *     speed_khz: i2c transfer speed.
 * @param[in]
 *     buffer: write data buffer pointer.
 * @param[in]
 *     len: write data length.
 * @return
 *     0, i2c transfer successfully.\n
 *     error code from mtk_i2c_transfer().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If mtk_i2c_transfer() fails, return its error code.\n
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int mtk_i2c_write(uint8_t bus_num, uint8_t device_addr, uint32_t speed_khz,
		  uint8_t *buffer, uint16_t len)
{
	struct i2c_ipi_msg_t *p_ipi_msg;
	uint32_t ipi_msg_len = 0;
	uint8_t i = 0;

	int send_status = -1;

	p_ipi_msg  = kzalloc(sizeof(struct i2c_ipi_msg_t), GFP_KERNEL);
	if (p_ipi_msg == NULL)
		return -1;

	p_ipi_msg->magic = IPI_MSG_I2C_MAGIC_NUMBER;
	p_ipi_msg->i2c_ipi_mode = WRITE_MODE_IPI;
	p_ipi_msg->bus_num = bus_num;
	p_ipi_msg->device_addr = device_addr;
	p_ipi_msg->speed_khz = speed_khz;
	p_ipi_msg->write_len = len;
	ipi_msg_len = sizeof(struct i2c_ipi_msg_t);

	for (i = 0; i < len; i++)
		p_ipi_msg->write_buffer[i] = buffer[i];

	if (check_msg_format(p_ipi_msg, ipi_msg_len) != 0) {
		pr_info("[%s], drop msg due to ipi fmt err\n", __func__);
		goto exit;
	}

	send_status = adsp_ipi_send(ADSP_IPI_I2C, p_ipi_msg, ipi_msg_len, 0, 0);

	if (send_status == 0)
		pr_info("[%s], set msg ok\n", __func__);
	else
		pr_info("[%s], i2c_ipi_send error %d\n", __func__, send_status);

exit:
	kfree(p_ipi_msg);
	return (send_status == 0) ? 0 : -1;
}

/** @ingroup IP_group_i2c_external_function
 * @par Description
 *     Initialize struct mtk_i2c and i2c_msg, first write data to\n
 *     slave device then read data from slave device.
 * @param[in]
 *     bus_num: i2c bus number.
 * @param[in]
 *     device_addr: slave device 7bits address.
 * @param[in]
 *     speed_khz: i2c transfer speed.
 * @param[in]
 *     write_buffer: write data buffer pointer.
 * @param[out]
 *     read_buffer: read data buffer pointer.
 * @param[in]
 *     write_len: write data length.
 * @param[in]
 *     read_len: read data length.
 * @return
 *     0, i2c transfer successfully.\n
 *     error code from mtk_i2c_transfer().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     If mtk_i2c_transfer() fails, return its error code.\n
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int mtk_i2c_write_read(uint8_t bus_num, uint8_t device_addr, uint32_t speed_khz,
		       uint8_t *write_buffer, uint8_t *read_buffer,
		       uint16_t write_len, uint16_t read_len)
{
	struct i2c_ipi_msg_t *p_ipi_msg;
	uint32_t ipi_msg_len = 0;
	uint8_t i = 0;

	int send_status = -1;

	p_ipi_msg  = kzalloc(sizeof(struct i2c_ipi_msg_t), GFP_KERNEL);
	if (p_ipi_msg == NULL)
		return -1;

	p_ipi_msg->magic = IPI_MSG_I2C_MAGIC_NUMBER;
	p_ipi_msg->i2c_ipi_mode = WRITE_READ_MODE_IPI;
	p_ipi_msg->bus_num = bus_num;
	p_ipi_msg->device_addr = device_addr;
	p_ipi_msg->speed_khz = speed_khz;
	p_ipi_msg->write_len = write_len;
	p_ipi_msg->read_len = read_len;
	ipi_msg_len = sizeof(struct i2c_ipi_msg_t);

	for (i = 0; i < write_len; i++)
		p_ipi_msg->write_buffer[i] = write_buffer[i];

	if (check_msg_format(p_ipi_msg, ipi_msg_len) != 0) {
		pr_info("[%s], drop msg due to ipi fmt err\n", __func__);
		goto exit;
	}

	init_completion(&read_complete);

	send_status = adsp_ipi_send(ADSP_IPI_I2C, p_ipi_msg, ipi_msg_len, 0, 0);
	if (send_status == 0) {
		pr_info("[%s], set msg ok\n", __func__);
		if (wait_for_completion_timeout(&read_complete,
						 msecs_to_jiffies(2000))) {
			for (i = 0; i < read_len; i++)
				read_buffer[i] = read_buff_array[i];
		} else {
			pr_info("[%s], read wait timeout!\n", __func__);
		}
	} else {
		pr_info("[%s], i2c_ipi_send error %d\n", __func__, send_status);
	}

exit:
	kfree(p_ipi_msg);
	return (send_status == 0) ? 0 : -1;
}

static void i2c_ipi_msg_dispatcher(int id, void *data, unsigned int len)
{
	int i;
	struct i2c_ipi_msg_t *p_ipi_msg = NULL;

	if (!data) {
		pr_info("%s(), drop msg due to data = NULL\n", __func__);
		return;
	}
	if (len > MAX_I2C_IPI_MSG_BUF_SIZE) {
		pr_info("%s(), drop msg due to len(%x) error!!\n",
			__func__, len);
		return;
	}

	p_ipi_msg = (struct i2c_ipi_msg_t *)data;
	check_msg_format(p_ipi_msg, len);

	if (p_ipi_msg->i2c_ipi_mode == READ_MODE_IPI ||
	    p_ipi_msg->i2c_ipi_mode == WRITE_READ_MODE_IPI) {
		for (i = 0; i < p_ipi_msg->read_len; i++)
			read_buff_array[i] = p_ipi_msg->read_buffer[i];
		complete(&read_complete);
	} else {
		pr_info("[%s], Wrong set OP, i2c_ipi_mode:0x%x\n",
			__func__, p_ipi_msg->i2c_ipi_mode);
	}
}

void i2c_messenger_ipi_init(void)
{
	int retval = 0;

	retval = adsp_ipi_registration(ADSP_IPI_I2C,
			(ipi_handler_t)i2c_ipi_msg_dispatcher, "i2c_ipi");

	if (retval != 0) {
		pr_info("%s(), i2c ipi register fail. retval=0x%x.\n",
			__func__, retval);
	} else {
		pr_info("%s(), i2c ipi register OK.\n", __func__);
	}
}

static int __init i2c_ipi_driver_init(void)
{
	i2c_messenger_ipi_init();

	return 0;
}

static void __exit i2c_ipi_driver_exit(void)
{
	pr_info("%s() OK.\n", __func__);
}

module_init(i2c_ipi_driver_init);
module_exit(i2c_ipi_driver_exit);
