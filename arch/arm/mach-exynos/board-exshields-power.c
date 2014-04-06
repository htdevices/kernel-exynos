/* Derived from
 * linux/arch/arm/mach-exynos/board-smdk5250-power.c
 *
 * Copyright (c) 2014 High Technology Devices LLC.
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/s5m87xx/s5m-core.h>
#include <linux/mfd/s5m87xx/s5m-pmic.h>

#include <asm/system_misc.h>

#include <plat/iic.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-pmu.h>

#include "board-exshields.h"

#include "common.h"

#define EXSHIELDS_PMIC_EINT	IRQ_EINT(26)

#define REBOOT_MODE_PREFIX	0x12345670
#define REBOOT_MODE_NONE	0
#define REBOOT_MODE_RECOVERY	4
#define REBOOT_MODE_FAST_BOOT	7

#define REBOOT_MODE_NO_LPM	0x12345678
#define REBOOT_MODE_LPM		0

/* S5M8767 Regulator */
static struct regulator_consumer_supply s5m8767_buck1_consumer =
	REGULATOR_SUPPLY("vdd_mif", NULL);

static struct regulator_consumer_supply s5m8767_buck2_consumer =
	REGULATOR_SUPPLY("vdd_arm", NULL);

static struct regulator_consumer_supply s5m8767_buck3_consumer =
	REGULATOR_SUPPLY("vdd_int", NULL);

static struct regulator_consumer_supply s5m8767_buck4_consumer =
	REGULATOR_SUPPLY("vdd_g3d", NULL);

static struct regulator_consumer_supply s5m8767_ldo4_consumer =
	REGULATOR_SUPPLY("vdd_ldo4", NULL);

static struct regulator_consumer_supply s5m8767_ldo9_consumer =
	REGULATOR_SUPPLY("vdd_ldo9", NULL);

static struct regulator_consumer_supply s5m8767_ldo11_consumer =
	REGULATOR_SUPPLY("vdd_ldo11", NULL);

static struct regulator_consumer_supply s5m8767_ldo14_consumer =
	REGULATOR_SUPPLY("vdd_ldo14", NULL);

static struct regulator_consumer_supply s5m8767_ldo17_consumer =
	REGULATOR_SUPPLY("5m_core_1.5v", NULL);

static struct regulator_consumer_supply s5m8767_ldo18_consumer =
	REGULATOR_SUPPLY("cam_io_1.8v", NULL);

static struct regulator_consumer_supply s5m8767_ldo19_consumer =
	REGULATOR_SUPPLY("vt_cam_1.8v", NULL);

static struct regulator_consumer_supply s5m8767_ldo24_consumer =
	REGULATOR_SUPPLY("cam_af_2.8v", NULL);

static struct regulator_init_data s5m8767_buck1_data = {
	.constraints	= {
		.name		= "vdd_mif range",
		.min_uV		=  950000,
		.max_uV		= 1300000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.always_on = 1,
		.boot_on = 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_buck1_consumer,
};

static struct regulator_init_data s5m8767_buck2_data = {
	.constraints	= {
		.name		= "vdd_arm range",
		.min_uV		=  800000,
		.max_uV		= 1350000,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.always_on = 1,
		.boot_on = 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_buck2_consumer,
};

static struct regulator_init_data s5m8767_buck3_data = {
	.constraints	= {
		.name		= "vdd_int range",
		.min_uV		=  900000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.boot_on = 1,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_buck3_consumer,
};

static struct regulator_init_data s5m8767_buck4_data = {
	.constraints	= {
		.name		= "vdd_g3d range",
		.min_uV		=  700000,
		.max_uV		= 1300000,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.boot_on = 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_buck4_consumer,
};

static struct regulator_init_data s5m8767_ldo4_data = {
	.constraints	= {
		.name		= "vdd_ldo4 range",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_ldo4_consumer,
};

static struct regulator_init_data s5m8767_ldo9_data = {
	.constraints	= {
		.name		= "vdd_ldo9 range",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_ldo9_consumer,
};

static struct regulator_init_data s5m8767_ldo11_data = {
	.constraints	= {
		.name		= "vdd_ldo11 range",
		.min_uV		= 1900000,
		.max_uV		= 1900000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_ldo11_consumer,
};

static struct regulator_init_data s5m8767_ldo14_data = {
	.constraints	= {
		.name		= "vdd_ldo14 range",
		.min_uV		= 1900000,
		.max_uV		= 1900000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_ldo14_consumer,
};

static struct regulator_init_data s5m8767_ldo17_data = {
	.constraints	= {
		.name		= "5m_core_1.5v",
		.min_uV		= 1500000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_ldo17_consumer,
};

static struct regulator_init_data s5m8767_ldo18_data = {
	.constraints	= {
		.name		= "cam_io_1.8v",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_ldo18_consumer,
};

static struct regulator_init_data s5m8767_ldo19_data = {
	.constraints	= {
		.name		= "vt_cam_1.8v",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_ldo19_consumer,
};

static struct regulator_init_data s5m8767_ldo24_data = {
	.constraints	= {
		.name		= "cam_af_2.8v",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &s5m8767_ldo24_consumer,
};

static struct s5m_regulator_data exynos_regulators[] = {
	{S5M8767_BUCK1, &s5m8767_buck1_data},
	{S5M8767_BUCK2, &s5m8767_buck2_data},
	{S5M8767_BUCK3, &s5m8767_buck3_data},
	{S5M8767_BUCK4, &s5m8767_buck4_data},
	{S5M8767_LDO4, &s5m8767_ldo4_data},
	{S5M8767_LDO9, &s5m8767_ldo9_data},
	{S5M8767_LDO11, &s5m8767_ldo11_data},
	{S5M8767_LDO14, &s5m8767_ldo14_data},
	{S5M8767_LDO17, &s5m8767_ldo17_data},
	{S5M8767_LDO18, &s5m8767_ldo18_data},
	{S5M8767_LDO19, &s5m8767_ldo19_data},
	{S5M8767_LDO24, &s5m8767_ldo24_data},
};

struct s5m_opmode_data s5m8767_opmode_data[S5M8767_REG_MAX] = {
	[S5M8767_BUCK1] = {S5M8767_BUCK1, S5M_OPMODE_STANDBY},
	[S5M8767_BUCK2] = {S5M8767_BUCK2, S5M_OPMODE_STANDBY},
	[S5M8767_BUCK3] = {S5M8767_BUCK3, S5M_OPMODE_STANDBY},
	[S5M8767_BUCK4] = {S5M8767_BUCK4, S5M_OPMODE_STANDBY},
	[S5M8767_LDO4] = {S5M8767_LDO4, S5M_OPMODE_STANDBY},
	[S5M8767_LDO9] = {S5M8767_LDO9, S5M_OPMODE_STANDBY},
	[S5M8767_LDO11] = {S5M8767_LDO11, S5M_OPMODE_STANDBY},
	[S5M8767_LDO14] = {S5M8767_LDO14, S5M_OPMODE_STANDBY},
	[S5M8767_LDO17] = {S5M8767_LDO17, S5M_OPMODE_STANDBY},
	[S5M8767_LDO18] = {S5M8767_LDO18, S5M_OPMODE_STANDBY},
	[S5M8767_LDO19] = {S5M8767_LDO19, S5M_OPMODE_STANDBY},
	[S5M8767_LDO24] = {S5M8767_LDO24, S5M_OPMODE_STANDBY},
};

static int s5m_cfg_irq(void)
{
	unsigned int pin = irq_to_gpio(EXSHIELDS_PMIC_EINT);

	s3c_gpio_cfgpin(pin, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(pin, S3C_GPIO_PULL_UP);

	return 0;
}

static struct s5m_platform_data exshields_s5m8767_pdata = {
	.device_type		= S5M8767X,
	.irq_base		= IRQ_BOARD_START,
	.num_regulators		= ARRAY_SIZE(exynos_regulators),
	.regulators		= exynos_regulators,
	.cfg_pmic_irq		= s5m_cfg_irq,
	.wakeup			= 1,
	.opmode_data		= s5m8767_opmode_data,

	.buck_default_idx	= 1,
	.buck_gpios[0]		= EXYNOS5_GPD1(0),
	.buck_gpios[1]		= EXYNOS5_GPD1(1),
	.buck_gpios[2]		= EXYNOS5_GPD1(2),

	.buck_ramp_delay	= 25,
	.buck2_ramp_enable	= 1,
	.buck3_ramp_enable	= 1,
	.buck4_ramp_enable	= 1,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
	{
		I2C_BOARD_INFO("s5m87xx", 0xCC >> 1),
		.platform_data	= &exshields_s5m8767_pdata,
		.irq		= EXSHIELDS_PMIC_EINT,
	},
};

static void exshields_power_off(void)
{
	local_irq_disable();

	writel(readl(EXYNOS_PS_HOLD_CONTROL) & ~BIT(8),
		EXYNOS_PS_HOLD_CONTROL);

	exynos5_restart(0, 0);
}

static void exshields_reboot(char str, const char *cmd)
{
	local_irq_disable();

	writel(REBOOT_MODE_NO_LPM, EXYNOS_INFORM2); /* Don't enter lpm mode */
	writel(REBOOT_MODE_PREFIX | REBOOT_MODE_NONE, EXYNOS_INFORM3);

	if (cmd) {
		if (!strcmp(cmd, "recovery"))
			writel(REBOOT_MODE_PREFIX | REBOOT_MODE_RECOVERY,
			       EXYNOS_INFORM3);
		else if (!strcmp(cmd, "bootloader"))
			writel(REBOOT_MODE_PREFIX | REBOOT_MODE_FAST_BOOT,
			       EXYNOS_INFORM2);
	}

	exynos5_restart(str, cmd); /* S/W reset: INFORM0~3:  Keep its value */
}

void __init exynos5_exshields_power_init(void)
{
	pm_power_off = exshields_power_off;
	arm_pm_restart = exshields_reboot;

	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
}
