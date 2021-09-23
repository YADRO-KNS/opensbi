/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 YADRO
 *
 * Authors:
 *   Nikita Shubin <nshubin@yadro.com>
 *
 * derivate: lib/utils/gpio/gpio.c
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_error.h>
#include <sbi_utils/i2c/i2c.h>

#define I2C_ADAPTER_MAX		16

static struct i2c_adapter *i2c_array[I2C_ADAPTER_MAX];

struct i2c_adapter *i2c_adapter_find(int id)
{
	unsigned int i;
	struct i2c_adapter *ret = NULL;

	for (i = 0; i < I2C_ADAPTER_MAX; i++) {
		if (i2c_array[i] && i2c_array[i]->id == id) {
			ret = i2c_array[i];
			break;
		}
	}

	return ret;
}

int i2c_adapter_add(struct i2c_adapter *ia)
{
	int i, ret = SBI_ENOSPC;

	if (!ia)
		return SBI_EINVAL;
	if (i2c_adapter_find(ia->id))
		return SBI_EALREADY;

	for (i = 0; i < I2C_ADAPTER_MAX; i++) {
		if (!i2c_array[i]) {
			i2c_array[i] = ia;
			ret = 0;
			break;
		}
	}

	return ret;
}

void i2c_adapter_remove(struct i2c_adapter *ia)
{
	int i;

	if (!ia)
		return;

	for (i = 0; i < I2C_ADAPTER_MAX; i++) {
		if (i2c_array[i] == ia) {
			i2c_array[i] = NULL;
			break;
		}
	}
}

int i2c_adapter_configure(struct i2c_adapter *ia)
{
	if (!ia)
		return SBI_EINVAL;
	if (!ia->configure)
		return 0;

	return ia->configure(ia);
}

int i2c_adapter_send(struct i2c_adapter *ia, uint8_t addr,
		     uint8_t reg, uint8_t value)
{
	if (!ia)
		return SBI_EINVAL;
	if (!ia->send)
		return SBI_ENOSYS;

	return ia->send(ia, addr, reg, value);
}


int i2c_adapter_read(struct i2c_adapter *ia, uint8_t addr,
		     uint8_t reg, uint8_t *value)
{
	if (!ia)
		return SBI_EINVAL;
	if (!ia->read)
		return SBI_ENOSYS;

	return ia->read(ia, addr, reg, value);
}
