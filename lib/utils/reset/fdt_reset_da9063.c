/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 YADRO
 *
 * Authors:
 *   Nikita Shubin <nshubin@yadro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_system.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/i2c/fdt_i2c.h>
#include <sbi_utils/reset/fdt_reset.h>

#define DA9063_REG_PAGE_CON		0x00
#define DA9063_REG_CONTROL_A		0x0e
#define DA9063_REG_CONTROL_C		0x10
#define DA9063_REG_CONTROL_F		0x13
#define DA9063_REG_DEVICE_ID		0x81

#define DA9063_CONTROL_A_CP_EN		(1 << 7)
#define DA9063_CONTROL_A_M_POWER1_EN	(1 << 6)
#define DA9063_CONTROL_A_M_POWER_EN	(1 << 5)
#define DA9063_CONTROL_A_M_SYSTEM_EN	(1 << 4)
#define DA9063_CONTROL_A_STANDBY	(1 << 3)
#define DA9063_CONTROL_A_POWER1_EN	(1 << 2)
#define DA9063_CONTROL_A_POWER_EN	(1 << 1)
#define DA9063_CONTROL_A_SYSTEM_EN	(1 << 0)

#define DA9063_CONTROL_F_WAKEUP		(1 << 2)
#define DA9063_CONTROL_F_SHUTDOWN	(1 << 1)
#define DA9063_CONTROL_F_WATCHDOG	(1 << 0)

#define DA9063_CONTROL_C_DEF_SUPPLY	(1 << 7)
#define DA9063_CONTROL_C_SLEW_RATE	(1 << 4)
#define DA9063_CONTROL_C_OTPREAD_EN	(1 << 3)
#define DA9063_CONTROL_C_AUTO_BOOT	(1 << 2)
#define DA9063_CONTROL_C_DEBOUNCING	(1 << 0)

#define PMIC_CHIP_ID_DA9063		0x61

static struct {
	struct i2c_adapter *adapter;
	uint32_t reg;
	int last_error;
} da9063;



static int da9063_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		return 1;
	}

	return 0;
}

static inline int da9063_sanity_check(struct i2c_adapter *adap, uint32_t reg)
{
	uint8_t val;
	int rc = i2c_adapter_send(adap, reg, DA9063_REG_PAGE_CON, 0x02);

	if (rc)
		return rc;

	/* check set page*/
	rc = i2c_adapter_read(adap, reg, 0x0, &val);
	if (rc)
		return rc;

	if (val != 0x02)
		return SBI_ENODEV;

	/* read and check device id */
	rc = i2c_adapter_read(adap, reg, DA9063_REG_DEVICE_ID, &val);
	if (rc)
		return rc;

	if (val != PMIC_CHIP_ID_DA9063)
		return SBI_ENODEV;

	return 0;
}

static inline int da9063_shutdown(struct i2c_adapter *adap, uint32_t reg)
{
	int rc = i2c_adapter_send(adap, da9063.reg, DA9063_REG_PAGE_CON, 0x00);

	if (rc)
		return rc;

	return i2c_adapter_send(adap, da9063.reg,
				DA9063_REG_CONTROL_F, DA9063_CONTROL_F_SHUTDOWN);
}

static inline int da9063_reset(struct i2c_adapter *adap, uint32_t reg)
{
	int rc = i2c_adapter_send(adap, da9063.reg, DA9063_REG_PAGE_CON, 0x00);

	if (rc)
		return rc;

	rc = i2c_adapter_send(adap, da9063.reg,
			      DA9063_REG_CONTROL_F, DA9063_CONTROL_F_WAKEUP);
	if (rc)
		return rc;

	return i2c_adapter_send(adap, da9063.reg,
				DA9063_REG_CONTROL_A,
				DA9063_CONTROL_A_M_POWER1_EN |
				DA9063_CONTROL_A_M_POWER_EN |
				DA9063_CONTROL_A_STANDBY);
}

static void da9063_system_reset(u32 type, u32 reason)
{
	struct i2c_adapter *adap = da9063.adapter;
	uint32_t reg = da9063.reg;
	int rc;

	if (adap) {
		/* may include clock init */
		i2c_adapter_configure(adap);

		/* sanity check */
		rc = da9063_sanity_check(adap, reg);
		if (rc) {
			sbi_printf("%s: chip is not da9063 PMIC\n", __func__);
			goto skip_reset;
		}

		switch (type) {
		case SBI_SRST_RESET_TYPE_SHUTDOWN:
			da9063_shutdown(adap, reg);
			break;
		case SBI_SRST_RESET_TYPE_COLD_REBOOT:
		case SBI_SRST_RESET_TYPE_WARM_REBOOT:
			da9063_reset(adap, reg);
			break;
		}

		while
			(1);

skip_reset:
		sbi_hart_hang();
	}
}

static struct sbi_system_reset_device da9063_reset_i2c = {
	.name = "da9063-reset",
	.system_reset_check = da9063_system_reset_check,
	.system_reset = da9063_system_reset
};

static int da9063_reset_init(void *fdt, int nodeoff,
			   const struct fdt_match *match)
{
	int rc, i2c_bus;
	struct i2c_adapter *adapter;
	uint64_t addr;

	/* find i2c bus parent node */
	i2c_bus = fdt_parent_offset(fdt, nodeoff);
	if (i2c_bus < 0)
		return i2c_bus;

	/* i2c adapter get */
	rc = fdt_i2c_adapter_get(fdt, i2c_bus, &adapter);
	if (rc)
		return rc;

	da9063.adapter = adapter;

	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &addr, NULL);
	if (rc)
		return rc;

	da9063.reg = addr;

	sbi_system_reset_set_device(&da9063_reset_i2c);

	return 0;
}

static const struct fdt_match da9063_reset_match[] = {
	{ .compatible = "dlg,da9063", .data = (void *)TRUE },
	{ },
};

struct fdt_reset fdt_reset_da9063 = {
	.match_table = da9063_reset_match,
	.init = da9063_reset_init,
};
