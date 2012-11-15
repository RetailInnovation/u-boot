/*
 * Copyright 2012 Retail Innovation
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <i2c.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/clock.h>
#include <errno.h>

#define SAIF_CTRL	(MXS_SAIF0_BASE + 0x00)
#define SAIF_CTRL_SET	(MXS_SAIF0_BASE + 0x04)
#define SAIF_CTRL_CLR	(MXS_SAIF0_BASE + 0x08)
#define SAIF_CTRL_TOG	(MXS_SAIF0_BASE + 0x0C)

#define SAIF_STAT	(MXS_SAIF0_BASE + 0x10)
#define SAIF_STAT_SET	(MXS_SAIF0_BASE + 0x14)
#define SAIF_STAT_CLR	(MXS_SAIF0_BASE + 0x18)
#define SAIF_STAT_TOG	(MXS_SAIF0_BASE + 0x1C)

#define SAIF_DATA	(MXS_SAIF0_BASE + 0x20)

#include "pcm_samples.c"

int i2c_write_byte(uchar chip, uint addr, uchar byte)
{
	mdelay(10);
	return i2c_write(chip, addr, 1, &byte, 1);
}

int do_speakertest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i, j;
	int rv = 0;

	struct mx28_clkctrl_regs *clkctrl =
		(struct mx28_clkctrl_regs*)MXS_CLKCTRL_BASE;

	i2c_init(100000, 0x18);

	mdelay(1);

	/* Soft reset */
	rv += i2c_write_byte(0x18, 0x00, 0x00);
	rv += i2c_write_byte(0x18, 0x01, 0x01);

	mdelay(1);

	/* Configure MCLK as clock input */
	rv += i2c_write_byte(0x18, 0x04, 0x00);

	/* Configure clock dividers */
	rv += i2c_write_byte(0x18, 0x0B, 0x82); // NDAC
	rv += i2c_write_byte(0x18, 0x0C, 0x82); // MDAC
	rv += i2c_write_byte(0x18, 0x0D, 0x00); // DOSR
	rv += i2c_write_byte(0x18, 0x0E, 0x40); // DOSR

	rv += i2c_write_byte(0x18, 0x1B, 0x00); // i2s word length

	/* DAC processing block */
	rv += i2c_write_byte(0x18, 0x3C, 0x10);
	rv += i2c_write_byte(0x18, 0x00, 0x08);
	rv += i2c_write_byte(0x18, 0x01, 0x04);

	rv += i2c_write_byte(0x18, 0x00, 0x01); // Set page 1
	rv += i2c_write_byte(0x18, 0x23, 0x40); // DAC output to mixer
	rv += i2c_write_byte(0x18, 0x2A, 0x1C); // Unmute Class-D and set gain
	rv += i2c_write_byte(0x18, 0x20, 0x86); // Powerup Class-D
	rv += i2c_write_byte(0x18, 0x26, 0x92); // Set gain and route analog volume to Class-D

	rv += i2c_write_byte(0x18, 0x00, 0x00); // Set page 0
	rv += i2c_write_byte(0x18, 0x3F, 0xB4); // Powerup DAC
	rv += i2c_write_byte(0x18, 0x41, 0xEA); // Set DAC gain
	rv += i2c_write_byte(0x18, 0x40, 0x04); // Unmute DAC

	if (rv != 0) {
		puts("SPEAKERTEST: i2c_write_failed\n");
		return 1;
	}

	/* Reset and gate off saif clock */
	writel((0x3 << 30), SAIF_CTRL_SET);

	/* Disable bypass */
	clkctrl->hw_clkctrl_clkseq_clr = CLKCTRL_CLKSEQ_BYPASS_SAIF0;

	/* saif_clk: 22.5792 Mhz */
	clkctrl->hw_clkctrl_saif0_clr = CLKCTRL_SAIF0_CLKGATE;
	clkctrl->hw_clkctrl_saif0     = CLKCTRL_SAIF0_DIV_FRAC_EN | 0x0C0B;

	while(clkctrl->hw_clkctrl_saif0 & CLKCTRL_SAIF0_BUSY);

	/* Clear reset and enable clock */
	writel((0x3 << 30), SAIF_CTRL_CLR);

	/* MCLK: 11.2896 Mhz, I2S Mode */
	writel((1 << 27) | (1 << 11), SAIF_CTRL);

	/* RUN! */
	writel(0x1, SAIF_CTRL_SET);

	for (i = 0; i < 5; i++) {
		for (j = 0; j < 44100; j++) {

			/* Wait until FIFO needs service */
			while(!(readl(SAIF_STAT) & (1 << 4)));

			u16 s0 = pcm_samples[j];
			u16 s1 = pcm_samples[j];

			/* Write sample */
			writel((s0 << 16) | s1, SAIF_DATA);
		}
	}

	writel(0x1, SAIF_CTRL_CLR);

	return 0;
}

U_BOOT_CMD(
	speakertest, 3, 1, do_speakertest,
	"Test speaker",
	"\n"
	"    - Test speaker"
);
