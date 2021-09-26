/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 YADRO
 *
 * Authors:
 *   Nikita Shubin <nshubin@yadro.com>
 */

#ifndef __I2C_H__
#define __I2C_H__

#include <sbi/sbi_types.h>

/** Representation of a I2C adapter */
struct i2c_adapter {
	/** Pointer to I2C driver owning this I2C adapter */
	void *driver;

	/** Uniquie ID of the I2C adapter assigned by the driver */
	int id;

	/**
	 * Configure I2C adapter
	 *
	 * Enable, set dividers, etc...
	 *
	 * @return 0 on success and negative error code on failure
	 */
	int (*configure)(struct i2c_adapter *ia);

	/**
	 * Send byte to given address, register
	 *
	 * @return 0 on success and negative error code on failure
	 */
	int (*send)(struct i2c_adapter *ia, uint8_t addr, uint8_t reg, uint8_t value);

	/**
	 * Read byte from given address, register
	 *
	 * @return 0 on success and negative error code on failure
	 */
	int (*read)(struct i2c_adapter *ia, uint8_t addr, uint8_t reg, uint8_t *value);
};

/** Find a registered I2C adapter */
struct i2c_adapter *i2c_adapter_find(int id);

/** Register I2C adapter */
int i2c_adapter_add(struct i2c_adapter *ia);

/** Un-register I2C adapter */
void i2c_adapter_remove(struct i2c_adapter *ia);

/** Configure I2C adapter prior to send/read */
int i2c_adapter_configure(struct i2c_adapter *ia);

/** Send to device on I2C adapter bus */
int i2c_adapter_send(struct i2c_adapter *ia, uint8_t addr, uint8_t reg, uint8_t value);

/** Read from device on I2C adapter bus */
int i2c_adapter_read(struct i2c_adapter *ia, uint8_t addr, uint8_t reg, uint8_t *value);

#endif
