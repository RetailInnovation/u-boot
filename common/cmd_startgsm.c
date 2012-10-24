/*
 * cmd_startgsm.c - start the GSM module
 *
 */

#include <config.h>
#include <common.h>
#include <command.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/clock.h>
#include <errno.h>

#define GSM_MODULE_RESET_PIN	MX28_PAD_ENET0_TXD2__GPIO_4_11
#define GSM_MODULE_ON_OFF_PIN	MX28_PAD_ENET0_TXD3__GPIO_4_12
#define GSM_MODULE_PWR_EN_PIN	MX28_PAD_ENET0_COL__GPIO_4_14
#define GSM_MODULE_STATUS_PIN	MX28_PAD_SSP1_CMD__GPIO_2_13

int do_startgsm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;

	gpio_direction_output(GSM_MODULE_PWR_EN_PIN, 1);
	gpio_direction_output(GSM_MODULE_RESET_PIN, 1);
	gpio_direction_output(GSM_MODULE_ON_OFF_PIN, 1);
	gpio_direction_input(GSM_MODULE_STATUS_PIN);

	mdelay(10);

	/* Turn on power */
	gpio_set_value(GSM_MODULE_PWR_EN_PIN, 0);

	mdelay(10);

	if (gpio_get_value(GSM_MODULE_STATUS_PIN) == 1) {
		puts("GSM: Module already on?\n");
		return;
	}

	gpio_set_value(GSM_MODULE_ON_OFF_PIN, 0);
	for (i = 0; i < 50; i++) {
		mdelay(100);
	}
	gpio_set_value(GSM_MODULE_ON_OFF_PIN, 1);

	if (gpio_get_value(GSM_MODULE_STATUS_PIN) != 1) {
		puts("GSM: Failed to turn on module\n");
		return;
        }

	for (i = 0; i < 10; i++) {
		mdelay(100);
	}

	return 0;
}

U_BOOT_CMD(
	startgsm, 3, 1, do_startgsm,
	"start gsm module",
	"\n"
	"    - Send start sequence to GSM module. Takes about 5 seconds to complete."
);
