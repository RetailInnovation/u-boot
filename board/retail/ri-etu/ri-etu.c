/*
 * Retail Innovation ri-etu board
 *
 * (C) Copyright 2012 Retail Innovation
 *
 * Based on mx28evk.c:
 * (C) Copyright 2011 Freescale Semiconductor, Inc.
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * ...and m28evk.c:
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/mii.h>
#include <miiphy.h>
#include <netdev.h>
#include <errno.h>
#include <fdt_support.h>
#include <mtd_node.h>
#include <jffs2/load_kernel.h>

DECLARE_GLOBAL_DATA_PTR;

static void board_usb_hub_init(void);
static void board_gsm_modem_init(void);

/*
 * Functions
 */
int board_early_init_f(void)
{
	/* IO0 clock at 480MHz */
	mx28_set_ioclk(MXC_IOCLK0, 480000);
	/* IO1 clock at 480MHz */
	mx28_set_ioclk(MXC_IOCLK1, 480000);

	/* SSP0 clock at 96MHz */
	mx28_set_sspclk(MXC_SSPCLK0, 96000, 0);
	/* SSP2 clock at 96MHz */
	mx28_set_sspclk(MXC_SSPCLK2, 96000, 0);

	return 0;
}

int dram_init(void)
{
	return mx28_dram_init();
}

int board_init(void)
{
	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

#ifdef CONFIG_CMD_USB
	board_usb_hub_init();
	board_gsm_modem_init();
#endif
	return 0;
}

#ifdef	CONFIG_CMD_MMC
static int ri_etu_mmc_wp(int id)
{
	if (id != 0) {
		printf("MXS MMC: Invalid card selected (card id = %d)\n", id);
		return 1;
	}

	return gpio_get_value(MX28_PAD_SSP1_SCK__GPIO_2_12);
}

int board_mmc_init(bd_t *bis)
{
	/* Configure WP as input */
	gpio_direction_input(MX28_PAD_SSP1_SCK__GPIO_2_12);

	/* Configure MMC0 Power Enable */
	gpio_direction_output(MX28_PAD_PWM3__GPIO_3_28, 0);

	return mxsmmc_initialize(bis, 0, ri_etu_mmc_wp);
}
#endif

#ifdef	CONFIG_CMD_NET

int board_eth_init(bd_t *bis)
{
	struct mx28_clkctrl_regs *clkctrl_regs =
		(struct mx28_clkctrl_regs *)MXS_CLKCTRL_BASE;
	struct eth_device *dev;
	int ret;

	ret = cpu_eth_init(bis);

	/* RI-ETU uses ENET_CLK PAD to drive FEC clock */
	writel(CLKCTRL_ENET_TIME_SEL_RMII_CLK | CLKCTRL_ENET_CLK_OUT_EN,
					&clkctrl_regs->hw_clkctrl_enet);

	/* Power-on FEC */
	gpio_direction_output(MX28_PAD_SSP1_DATA3__GPIO_2_15, 0);

	/* Reset FEC PHY */
	gpio_direction_output(MX28_PAD_ENET0_RX_CLK__GPIO_4_13, 0);
	udelay(200);
	gpio_set_value(MX28_PAD_ENET0_RX_CLK__GPIO_4_13, 1);

	ret = fecmxc_initialize_multi(bis, 0, 0, MXS_ENET0_BASE);
	if (ret) {
		puts("FEC MXS: Unable to init FEC0\n");
		return ret;
	}

	dev = eth_get_dev_by_name("FEC0");
	if (!dev) {
		puts("FEC MXS: Unable to get FEC0 device entry\n");
		return -EINVAL;
	}

	return ret;
}

#endif

#ifdef CONFIG_CMD_USB

#define USB_HUB_I2C_ADDR 0x2C

uint8_t hub_conf[255 + 11] = {
	24,
	0x24, 0x04, 0x14, 0x25, 0xB3, 0x0B, 0x9B, 0x20,
	0x02, 0x00, 0x00, 0x00, 0x01, 0x32, 0x01, 0x32,
	0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	15,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x43,
};

static void board_usb_hub_init(void)
{
	int rv;
	int i, j;
	uint8_t hub_attach_cmd[2] = { 0x01, 0x01 };

        /* USB hub reset */
	gpio_direction_output(MX28_PAD_ENET0_RXD2__GPIO_4_9, 0);
	mxs_iomux_setup_pad(MX28_PAD_ENET0_RXD2__GPIO_4_9 |
				(MXS_PAD_12MA | MXS_PAD_3V3));
	udelay(10);
	gpio_set_value(MX28_PAD_ENET0_RXD2__GPIO_4_9, 1);

	rv = i2c_init(CONFIG_SYS_I2C_SPEED, USB_HUB_I2C_ADDR);
	if (rv) {
		puts("USB HUB: Failed to init i2c bus\n");
	}

	mdelay(100);

	for (i = 0; i < 11; i++) {
		j = (i * 24) + i;
		rv = i2c_write(0x2C, i * 24, 1, &hub_conf[j], (hub_conf[j] + 1) );
		if (rv) {
			puts("USB HUB: Failed to write configuration\n");
		}
		mdelay(10);
	}

	mdelay(100);

	/* Attach to upstream port */
	rv = i2c_write(0x2C, 0xFF, 1, hub_attach_cmd, sizeof(hub_attach_cmd));
	if (rv) {
		puts("USB HUB: Failed to write attach command\n");
	}
}

static void board_gsm_modem_init(void)
{
        /* Configure GSM pins */
        gpio_direction_input(MX28_PAD_SSP1_CMD__GPIO_2_13);
        gpio_direction_input(MX28_PAD_SSP3_SCK__GPIO_2_24);
        gpio_direction_input(MX28_PAD_SSP3_MOSI__GPIO_2_25);
        gpio_direction_output(MX28_PAD_SSP3_MISO__GPIO_2_26, 0);
        gpio_direction_input(MX28_PAD_SSP3_SS0__GPIO_2_27);
        gpio_direction_input(MX28_PAD_ENET0_TXD2__GPIO_4_11);
        gpio_direction_input(MX28_PAD_ENET0_TXD3__GPIO_4_12);
        gpio_direction_input(MX28_PAD_AUART0_RX__GPIO_3_0);
        gpio_direction_output(MX28_PAD_AUART0_TX__GPIO_3_1, 0);
        gpio_direction_input(MX28_PAD_AUART0_CTS__GPIO_3_2);
        gpio_direction_output(MX28_PAD_AUART0_RTS__GPIO_3_3, 0);
        gpio_direction_output(MX28_PAD_ENET0_RXD3__GPIO_4_10, 0);
        udelay(100);
        gpio_set_value(MX28_PAD_ENET0_RXD3__GPIO_4_10, 1);
}
#endif

#if defined(CONFIG_OF_LIBFDT) && defined(CONFIG_OF_BOARD_SETUP)
void ft_board_setup(void *blob, bd_t *bd)
{
#ifdef CONFIG_FDT_FIXUP_PARTITIONS
	struct node_info nodes[] = {
		{ "fsl,imx28-gpmi-nand", MTD_DEV_TYPE_NAND, },
	};
	fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));
#endif
}
#endif
