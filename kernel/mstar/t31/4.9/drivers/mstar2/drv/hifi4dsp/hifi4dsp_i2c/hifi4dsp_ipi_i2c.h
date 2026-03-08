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

#ifndef HIFI4DSP_IPI_I2C_H
#define HIFI4DSP_IPI_I2C_H

#include <linux/of.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#define MAX_I2C_IPI_MSG_BUF_SIZE	(272) /* SHARE_BUF_SIZE - 16 */
#define IPI_MSG_I2C_MAGIC_NUMBER	0x9321
#define IPI_I2C_TRANSFER_MAX_NUMBER	0x10

enum i2c_ipi_mode {
	WRITE_MODE_IPI = 0,
	READ_MODE_IPI,
	WRITE_READ_MODE_IPI,
};

struct i2c_ipi_msg_t {
	uint16_t magic;
	uint16_t i2c_ipi_mode;
	uint8_t bus_num;
	uint8_t device_addr;
	uint32_t speed_khz;
	uint16_t write_len;
	uint16_t read_len;
	uint8_t write_buffer[16];
	uint8_t read_buffer[16];
};

int mtk_i2c_read(uint8_t bus_num, uint8_t device_addr, uint32_t speed_khz,
		 uint8_t *buffer, uint16_t len);

int mtk_i2c_write(uint8_t bus_num, uint8_t device_addr, uint32_t speed_khz,
		  uint8_t *buffer, uint16_t len);

int mtk_i2c_write_read(uint8_t bus_num, uint8_t device_addr, uint32_t speed_khz,
		       uint8_t *write_buffer, uint8_t *read_buffer,
		       uint16_t write_len, uint16_t read_len);

void i2c_messenger_ipi_init(void);

#endif /* end of HIFI4DSP_IPI_I2C_H */

