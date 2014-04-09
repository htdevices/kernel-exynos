/*
 * Derived from
 * arch/arm/mach-exynos/mach-smdk5250.c
 *
 * Copyright (c) 2014 High Technology Devices LLC.
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/cma.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/ion.h>
#include <linux/i2c.h>
#include <linux/keyreset.h>
#include <linux/mmc/host.h>
#include <linux/memblock.h>
#include <linux/persistent_ram.h>
#include <linux/platform_device.h>
#include <linux/platform_data/exynos_usb3_drd.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/serial_core.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/sys_soc.h>

#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/system_info.h>
#include <asm/system_misc.h>

#include <plat/adc.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/regs-serial.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/iic.h>

#include <mach/map.h>
#include <mach/sysmmu.h>
#include <mach/exynos_fiq_debugger.h>
#include <mach/exynos-ion.h>
#include <mach/dwmci.h>
#include <mach/ohci.h>
#include <mach/tmu.h>
#include <mach/exynos5_bus.h>

#include "../../../drivers/staging/android/ram_console.h"
#include "board-exshields.h"
#include "common.h"
#include "resetreason.h"

#define EXSHIELDS_CPU0_DEBUG_PA	0x10890000
#define EXSHIELDS_CPU1_DEBUG_PA	0x10892000
#define EXSHIELDS_CPU_DBGPCSR	0xa0

static int exshields_hw_rev;
phys_addr_t exshields_bootloader_fb_start;
phys_addr_t exshields_bootloader_fb_size = 2560 * 1600 * 4;
static void __iomem *exshields_cpu0_debug;
static void __iomem *exshields_cpu1_debug;

static int __init s3cfb_bootloaderfb_arg(char *options)
{
	char *p = options;

	exshields_bootloader_fb_start = memparse(p, &p);
	pr_debug("bootloader framebuffer found at %8X\n",
			exshields_bootloader_fb_start);

	return 0;
}
early_param("s3cfb.bootloaderfb", s3cfb_bootloaderfb_arg);

static struct gpio exshields_hw_rev_gpios[] = {
	{EXYNOS5_GPV1(4), GPIOF_IN, "hw_rev0"},
	{EXYNOS5_GPV1(3), GPIOF_IN, "hw_rev1"},
	{EXYNOS5_GPV1(2), GPIOF_IN, "hw_rev2"},
	{EXYNOS5_GPV1(1), GPIOF_IN, "hw_rev3"},
};

int exynos5_exshields_get_revision(void)
{
	return exshields_hw_rev;
}

static char exshields_board_info_string[255];

static void exshields_init_hw_rev(void)
{
	int ret;
	int i;

	ret = gpio_request_array(exshields_hw_rev_gpios,
			ARRAY_SIZE(exshields_hw_rev_gpios));

	BUG_ON(ret);

	for (i = 0; i < ARRAY_SIZE(exshields_hw_rev_gpios); i++) {
		exshields_hw_rev |=
			gpio_get_value(exshields_hw_rev_gpios[i].gpio) << i;
	}

	snprintf(exshields_board_info_string,
		sizeof(exshields_board_info_string) - 1,
		"exshields HW revision: %d, CPU EXYNOS5250 Rev%d.%d",
		exshields_hw_rev,
		samsung_rev() >> 4,
		samsung_rev() & 0xf);

	pr_info("%s\n", exshields_board_info_string);
	mach_panic_string = exshields_board_info_string;
}

static struct ram_console_platform_data ramconsole_pdata;

static struct platform_device ramconsole_device = {
	.name           = "ram_console",
	.id             = -1,
	.dev		= {
		.platform_data = &ramconsole_pdata,
	},
};

static struct platform_device persistent_trace_device = {
	.name           = "persistent_trace",
	.id             = -1,
};

static struct resource persistent_clock_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_RTC, SZ_256),
};

static struct platform_device persistent_clock = {
	.name           = "persistent_clock",
	.id             = -1,
	.num_resources	= ARRAY_SIZE(persistent_clock_resource),
	.resource	= persistent_clock_resource,
};

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define EXSHIELDS_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
		S3C2410_UCON_RXILEVEL |	\
		S3C2410_UCON_TXIRQMODE |	\
		S3C2410_UCON_RXIRQMODE |	\
		S3C2410_UCON_RXFIFO_TOI |	\
		S3C2443_UCON_RXERR_IRQEN)

#define EXSHIELDS_ULCON_DEFAULT	S3C2410_LCON_CS8

#define EXSHIELDS_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
		S5PV210_UFCON_TXTRIG4 |	\
		S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg exshields_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= EXSHIELDS_UCON_DEFAULT,
		.ulcon		= EXSHIELDS_ULCON_DEFAULT,
		.ufcon		= EXSHIELDS_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= EXSHIELDS_UCON_DEFAULT,
		.ulcon		= EXSHIELDS_ULCON_DEFAULT,
		.ufcon		= EXSHIELDS_UFCON_DEFAULT,
	},
	/* Do not initialize hwport 2, it will be handled by fiq_debugger */
	[2] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= EXSHIELDS_UCON_DEFAULT,
		.ulcon		= EXSHIELDS_ULCON_DEFAULT,
		.ufcon		= EXSHIELDS_UFCON_DEFAULT,
	},
};

/* ADC */
static struct s3c_adc_platdata exshields_adc_data __initdata = {
	.phy_init       = s3c_adc_phy_init,
	.phy_exit       = s3c_adc_phy_exit,
};

/* TMU */
static struct tmu_data exshields_tmu_pdata __initdata = {
	.ts = {
		.stop_throttle		= 78,
		.start_throttle		= 80,
		.start_tripping		= 110,
		.start_emergency	= 120,
		.stop_mem_throttle	= 80,
		.start_mem_throttle	= 85,
	},

	.efuse_value	= 80,
	.slope		= 0x10608802,
};

static struct persistent_ram_descriptor exshields_prd[] __initdata = {
	{
		.name = "ram_console",
		.size = SZ_2M,
	},
#ifdef CONFIG_PERSISTENT_TRACER
	{
		.name = "persistent_trace",
		.size = SZ_1M,
	},
#endif
};

static struct persistent_ram exshields_pr __initdata = {
	.descs = exshields_prd,
	.num_descs = ARRAY_SIZE(exshields_prd),
	.start = PLAT_PHYS_OFFSET + SZ_1G + SZ_512M,
#ifdef CONFIG_PERSISTENT_TRACER
	.size = 3 * SZ_1M,
#else
	.size = SZ_2M,
#endif
};

/* defined in arch/arm/mach-exynos/reserve-mem.c */
extern void exynos_cma_region_reserve(struct cma_region *,
		struct cma_region *, size_t, const char *);
extern int kbase_carveout_mem_reserve(phys_addr_t size);

static void __init exynos_reserve_mem(void)
{
	static struct cma_region regions[] = {
	{
		.name = "ion",
#ifdef CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE
		.size = CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE * SZ_1K,
#endif
		{
		.alignment = SZ_1M
		}
	},
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
#ifdef CONFIG_ION_EXYNOS_DRM_MFC_SH
	{
		.name = "drm_mfc_sh",
		.size = SZ_1M,
		.alignment = SZ_1M,
	},
#endif
#ifdef CONFIG_ION_EXYNOS_DRM_MSGBOX_SH
	{
		.name = "drm_msgbox_sh",
		.size = SZ_1M,
		.alignment = SZ_1M,
	},
#endif
#endif
	{
		.size = 0 /* END OF REGION DEFINITIONS */
	}
	};
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	static struct cma_region regions_secure[] = {
#ifdef CONFIG_ION_EXYNOS_DRM_MEMSIZE_FIMD_VIDEO
	{
		.name = "drm_fimd_video",
		.size = CONFIG_ION_EXYNOS_DRM_MEMSIZE_FIMD_VIDEO *
			SZ_1K,
		.alignment = SZ_1M,
	},
#endif
#ifdef CONFIG_ION_EXYNOS_DRM_MEMSIZE_MFC_OUTPUT
	{
		.name = "drm_mfc_output",
		.size = CONFIG_ION_EXYNOS_DRM_MEMSIZE_MFC_OUTPUT *
			SZ_1K,
		.alignment = SZ_1M,
	},
#endif
#ifdef CONFIG_ION_EXYNOS_DRM_MEMSIZE_MFC_INPUT
	{
		.name = "drm_mfc_input",
		.size = CONFIG_ION_EXYNOS_DRM_MEMSIZE_MFC_INPUT *
			SZ_1K,
		.alignment = SZ_1M,
	},
#endif
#ifdef CONFIG_ION_EXYNOS_DRM_MFC_FW
	{
		.name = "drm_mfc_fw",
		.size = SZ_1M,
		.alignment = SZ_1M,
	},
#endif
#ifdef CONFIG_ION_EXYNOS_DRM_SECTBL
	{
		.name = "drm_sectbl",
		.size = SZ_1M,
		.alignment = SZ_1M,
	},
#endif
	{
		.size = 0
	},
	};
#else /* !CONFIG_EXYNOS_CONTENT_PATH_PROTECTION */
	struct cma_region *regions_secure = NULL;
#endif /* CONFIG_EXYNOS_CONTENT_PATH_PROTECTION */

	static const char map[] __initconst =
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
		"ion-exynos/mfc_sh=drm_mfc_sh;"
		"ion-exynos/msgbox_sh=drm_msgbox_sh;"
		"ion-exynos/fimd_video=drm_fimd_video;"
		"ion-exynos/mfc_output=drm_mfc_output;"
		"ion-exynos/mfc_input=drm_mfc_input;"
		"ion-exynos/mfc_fw=drm_mfc_fw;"
		"ion-exynos/sectbl=drm_sectbl;"
		"s5p-smem/mfc_sh=drm_mfc_sh;"
		"s5p-smem/msgbox_sh=drm_msgbox_sh;"
		"s5p-smem/fimd_video=drm_fimd_video;"
		"s5p-smem/mfc_output=drm_mfc_output;"
		"s5p-smem/mfc_input=drm_mfc_input;"
		"s5p-smem/mfc_fw=drm_mfc_fw;"
		"s5p-smem/sectbl=drm_sectbl;"
#endif
		"ion-exynos=ion;"
		"s5p-mfc-v6/f=fw;"
		"s5p-mfc-v6/a=b1;";

	persistent_ram_early_init(&exshields_pr);
	if (exshields_bootloader_fb_start) {
		int err = memblock_reserve(exshields_bootloader_fb_start,
				exshields_bootloader_fb_size);
		if (err)
			pr_warn("failed to reserve old framebuffer location\n");
	} else {
		pr_warn("bootloader framebuffer start address not set\n");
	}

	exynos_cma_region_reserve(regions, regions_secure, 0, map);
	kbase_carveout_mem_reserve(384 * SZ_1M);
	ion_reserve(&exynos_ion_pdata);
}

/* DEVFREQ controlling mif */
static struct exynos5_bus_mif_platform_data exshields_bus_mif_platform_data;

static struct platform_device exynos_bus_mif_devfreq = {
	.name                   = "exynos5-bus-mif",
	.dev = {
		.platform_data	= &exshields_bus_mif_platform_data,
	},
};

/* DEVFREQ controlling int */
static struct platform_device exynos_bus_int_devfreq = {
	.name                   = "exynos5-bus-int",
};

static struct platform_device *exshields_devices[] __initdata = {
	&ramconsole_device,
	&persistent_trace_device,
	&s3c_device_i2c0,
	&s3c_device_i2c4,
	&s3c_device_i2c5,
	&s3c_device_adc,
	&s3c_device_wdt,
	&exynos_device_ion,
	&exynos_device_tmu,
	&exynos_bus_mif_devfreq,
	&exynos_bus_int_devfreq,
	&exynos5_device_g3d,
};

static void __init exshields_map_io(void)
{
	clk_xusbxti.rate = 24000000;
	clk_xxti.rate = 24000000;
	exynos_init_io(NULL, 0);
	s3c24xx_init_clocks(clk_xusbxti.rate);
	s3c24xx_init_uarts(exshields_uartcfgs, ARRAY_SIZE(exshields_uartcfgs));
}

static void __init exshields_sysmmu_init(void)
{
}

static void __init exshields_init_early(void)
{
}

static void __init soc_info_populate(struct soc_device_attribute *soc_dev_attr)
{
	soc_dev_attr->soc_id = kasprintf(GFP_KERNEL, "%08x%08x\n",
			system_serial_high, system_serial_low);
	soc_dev_attr->machine = kasprintf(GFP_KERNEL, "Exynos 5250\n");
	soc_dev_attr->family = kasprintf(GFP_KERNEL, "Exynos 5\n");
	soc_dev_attr->revision = kasprintf(GFP_KERNEL, "%d.%d\n",
			samsung_rev() >> 4,
			samsung_rev() & 0xf);
}

static ssize_t exshields_get_board_revision(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%d\n", exshields_hw_rev);
}

struct device_attribute exshields_soc_attr =
__ATTR(board_rev,  S_IRUGO, exshields_get_board_revision,  NULL);

static void __init exynos5_exshields_sysfs_soc_init(void)
{
	struct device *parent;
	struct soc_device *soc_dev;
	struct soc_device_attribute *soc_dev_attr;

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr) {
		printk(KERN_ERR "Failed to allocate memory for soc_dev_attr\n");
		return;
	}

	soc_info_populate(soc_dev_attr);

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR_OR_NULL(soc_dev)) {
		kfree(soc_dev_attr);
		printk(KERN_ERR "Failed to register a soc device under /sys\n");
		return;
	}

	parent = soc_device_to_device(soc_dev);
	if (!IS_ERR_OR_NULL(parent))
		device_create_file(parent, &exshields_soc_attr);

	return;  /* Or return parent should you need to use one later */
}

void exshields_panic_dump_cpu_pc(int cpu, unsigned long dbgpcsr)
{
	void *pc = NULL;

	pr_err("CPU%d DBGPCSR: %08lx\n", cpu, dbgpcsr);
	if ((dbgpcsr & 3) == 0)
		pc = (void *)(dbgpcsr - 8);
	else if ((dbgpcsr & 1) == 1)
		pc = (void *)((dbgpcsr & ~1) - 4);

	pr_err("CPU%d PC: <%p> %pF\n", cpu, pc, pc);
}

int exshields_panic_notify(struct notifier_block *nb,
		unsigned long event, void *p)
{
	unsigned long dbgpcsr;

	if (exshields_cpu0_debug && cpu_online(0)) {
		dbgpcsr = __raw_readl(exshields_cpu0_debug
				+ EXSHIELDS_CPU_DBGPCSR);
		exshields_panic_dump_cpu_pc(0, dbgpcsr);
	}
	if (exshields_cpu1_debug && cpu_online(1)) {
		dbgpcsr = __raw_readl(exshields_cpu1_debug
				+ EXSHIELDS_CPU_DBGPCSR);
		exshields_panic_dump_cpu_pc(1, dbgpcsr);
	}
	return NOTIFY_OK;
}

struct notifier_block exshields_panic_nb = {
	.notifier_call = exshields_panic_notify,
};

static void __init exshields_panic_init(void)
{
	exshields_cpu0_debug = ioremap(EXSHIELDS_CPU0_DEBUG_PA, SZ_4K);
	exshields_cpu1_debug = ioremap(EXSHIELDS_CPU1_DEBUG_PA, SZ_4K);

	atomic_notifier_chain_register(&panic_notifier_list, &exshields_panic_nb);
}

static void __init exshields_machine_init(void)
{
	exshields_init_hw_rev();

	exynos_serial_debug_init(2, 0);
	exshields_panic_init();

	exshields_sysmmu_init();

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c4_set_platdata(NULL);
	s3c_i2c5_set_platdata(NULL);

	s3c_adc_set_platdata(&exshields_adc_data);

	exynos_tmu_set_platdata(&exshields_tmu_pdata);

	ramconsole_pdata.bootinfo = exynos_get_resetreason();
	platform_add_devices(exshields_devices, ARRAY_SIZE(exshields_devices));

	exynos5_exshields_power_init();
}

MACHINE_START(EXSHIELDS, "Exshields")
	.atag_offset	= 0x100,
	.init_early	= exshields_init_early,
	.init_irq	= exynos5_init_irq,
	.map_io		= exshields_map_io,
	.handle_irq	= gic_handle_irq,
	.init_machine	= exshields_machine_init,
	.timer		= &exynos4_timer,
	.restart	= exynos5_restart,
	.reserve	= exynos_reserve_mem,
MACHINE_END
