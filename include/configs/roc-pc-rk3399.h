/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 */

#ifndef __ROC_PC_RK3399_H
#define __ROC_PC_RK3399_H

// The following bunch of defines is for the splash
#define CONFIG_SPLASHIMAGE_GUARD
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_SPLASH_SOURCE
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE (1920*1080*4)
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_VIDEO_BMP_LOGO
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_VIDEO_LOGO
#define SPLASH_ENV \
		"splashimage=0x08080000\0" \
		"splashpos=m,m\0" \
		"splashsource=mmc_fs\0"

// This sets up additional environment variables for our opinionated setup.
// (See rk3399_common.h, it should be apended to ROCKCHIP_DEVICE_SETTINGS here
//  but it makes patches harder to apply...)
#define OPINIONATED_ENV \
		SPLASH_ENV \
		"bootmenu_delay=-1\0" \
		"bootmenu_0=Default U-Boot boot=run distro_bootcmd; echo \"Boot failed.\"; sleep 5; $menucmd -1\0" \
		"bootmenu_1=Boot from eMMC=run bootcmd_mmc0; echo \"eMMC Boot failed.\"; sleep 5; $menucmd -1\0" \
		"bootmenu_2=Boot from SD=run bootcmd_mmc1; echo \"SD Boot failed.\"; sleep 5; $menucmd -1\0" \
		"bootmenu_3=Boot from USB=run bootcmd_usb0; echo \"USB Boot failed.\"; sleep 5; $menucmd -1\0" \
		"bootmenu_4=Boot PXE=run bootcmd_pxe; echo \"PXE Boot failed.\"; sleep 5; $menucmd -1\0" \
		"bootmenu_5=Boot DHCP=run bootcmd_dhcp; echo \"DHCP Boot failed.\"; sleep 5; $menucmd -1\0" \
		"bootmenu_6=Reboot=reset\0" \
		"menucmd=bootmenu\0"

#define ROCKCHIP_DEVICE_SETTINGS \
		OPINIONATED_ENV \
		"stdin=serial,usbkbd\0" \
		"stdout=serial,vidconsole\0" \
		"stderr=serial,vidconsole\0"

#include <configs/rk3399_common.h>

#define SDRAM_BANK_SIZE			(2UL << 30)

#endif
