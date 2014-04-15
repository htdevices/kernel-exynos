/*
 * Derived from
 * arch/arm/mach-exynos/board-smdk5250.h
 * 
 * Copyright (c) 2014 High Technology Devices LLC.
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_EXYNOS_BOARD_EXSHIELDS_H
#define __MACH_EXYNOS_BOARD_EXSHIELDS_H

#include <asm/system_info.h>

#define EXSHIELDS_REV_0_0		0x0
#define EXSHIELDS_REV_0_1		0x1
#define EXSHIELDS_REV_0_2		0x2
#define EXSHIELDS_REV_MASK		0xf

#define EXSHIELDS_REGULATOR_S5M8767	0x2
#define EXSHIELDS_REGULATOR_SHIFT	16
#define EXSHIELDS_REGULATOR_MASK	0xf

static inline int get_exshields_rev(void)
{
	return system_rev & EXSHIELDS_REV_MASK;
}

static inline int get_exshields_regulator(void)
{
	return (system_rev >> EXSHIELDS_REGULATOR_SHIFT) \
		& EXSHIELDS_REGULATOR_MASK;
}

void exynos5_exshields_mmc_init(void);
void exynos5_exshields_usb_init(void);
void exynos5_exshields_power_init(void);
void exynos5_exshields_media_init(void);

#endif
