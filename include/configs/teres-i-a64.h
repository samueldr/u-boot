/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration settings for the Olimex TERES-I
 */

#ifndef __TERES_I_A64_CONFIG_H
#define __TERES_I_A64_CONFIG_H

/*
 * TERES-I specific configuration
 */

#define SUNXI_BOARD_EXTRA_ENV "usb_pgood_delay 2000\0"

/*
 * Include common sun50i configuration
 */
#include <configs/sun50i.h>

#endif /* __TERES_I_A64_CONFIG_H */
