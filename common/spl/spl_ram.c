// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2016
 * Xilinx, Inc.
 *
 * (C) Copyright 2016
 * Toradex AG
 *
 * Michal Simek <michal.simek@amd.com>
 * Stefan Agner <stefan.agner@toradex.com>
 */
#include <binman_sym.h>
#include <image.h>
#include <log.h>
#include <mapmem.h>
#include <spl.h>
#include <linux/libfdt.h>

/*
 * SPL loader for spl_ram.
 */
static ulong spl_ram_load_read(struct spl_load_info *load, ulong sector, ulong count, void *buf)
{
	ulong address = (ulong)load->priv;
	debug("\t%s: address 0x%lx, sector 0x%lx, count 0x%lx, buf 0x%lx\n", __func__, address, sector, count, (ulong)buf);
	address += sector;
	debug("\t%s: actual address = 0x%lx\n", __func__, address);
	memcpy(buf, (void *)address, count);

	return count;
}

/*
 * Tries to load the next stage from the given address.
 *
 * First, handles IMAGE_PRE_LOAD, if relevant.
 *
 * Then, tries to use the location as either a FIT image, or a Legacy image.
 */
static int spl_ram_try_location(struct spl_image_info *spl_image, struct spl_boot_device *bootdev, ulong address)
{
	int ret = 0;

	debug("\t%s: address = 0x%lx\n", __func__, address);

	if (CONFIG_IS_ENABLED(IMAGE_PRE_LOAD)) {
		debug("\t%s: Running IMAGE_PRE_LOAD.\n", __func__);
		ret = image_pre_load(address);

		if (ret) {
			debug("\t%s: IMAGE_PRE_LOAD ret = %d.\n", __func__, ret);
			return ret;
		}

	 	address += image_load_offset;
	}

	struct legacy_img_hdr *header;
	header = map_sysmem(address, 0);

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) && image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		debug("\t%s: Found a FIT image.\n", __func__);
		// NOTE: passing the address itself, and not a pointer.
		spl_load_init(&load, spl_ram_load_read, (void*)address, 1);
		ret = spl_load_simple_fit(spl_image, &load, 0, header);
	} else {
		debug("\t%s: Not a FIT image. Trying Legacy image\n", __func__);
		header = map_sysmem(address, 0);
		ret = spl_parse_image_header(spl_image, bootdev, header);
	}

	return ret;
}

/*
 * Glue logic that allowing different methods of locating the next stage in RAM.
 *
 * First, handles DFU upload to RAM, to support DFU from SPL.
 * It is expected that SPL DFU support uploads to one of the locations tried next.
 *
 * Then, tries the different plausible locations the next stage may be;
 * from the SPL image itself, from a reference from binman, or from where the next stage is expected to be loaded into.
 *
 * All of these methods try to go to the next stage using the same function (`spl_ram_try_location`).
 * This means that any of the supported methods to load U-Boot will be tried at every plausible locations.
 */
static int spl_ram_load_image(struct spl_image_info *spl_image, struct spl_boot_device *bootdev)
{
	ulong address = -1;
	int ret;

	if (CONFIG_IS_ENABLED(DFU) && bootdev->boot_device == BOOT_DEVICE_DFU) {
		debug("\t%s: Loading data from DFU.\n", __func__);
		spl_dfu_cmd(0, "dfu_alt_info_ram", "ram", "0");
		// NOTE: the spl_dfu implementation assumes the following attempts will try the dfu_alt_info_ram locations.
	}

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT)) {
		debug("\t%s: Trying from CONFIG_SPL_LOAD_FIT_ADDRESS.\n", __func__);
		address = IF_ENABLED_INT(CONFIG_SPL_LOAD_FIT, CONFIG_SPL_LOAD_FIT_ADDRESS);
		ret = spl_ram_try_location(spl_image, bootdev, address);
		debug("\t >> ret = %d\n", ret);
	}

	if (ret != 0) {
		debug("\t%s: Trying from a u-boot-any ref from current SPL image.\n", __func__);
		address = spl_get_image_pos();
		if (address != BINMAN_SYM_MISSING) {
			ret = spl_ram_try_location(spl_image, bootdev, address);
			debug("\t >> ret = %d\n", ret);
		}
		else {
			debug("\t%s: Warning: No binman information for a 'u-boot-any' in current SPL image!!\n", __func__);
		}
	}

	/* No binman support or no information. */
	if (ret != 0) {
		struct legacy_img_hdr *header;
		header = map_sysmem(address, 0);
		/* For now, fix it to the address pointed to by U-Boot. */
		debug("\t%s: Falling back to SPL load buffer...\n", __func__);
		/*
		 * Get the header.  It will point to an address defined by
		 * handoff which will tell where the image located inside
		 * the flash.
		 */
		address = (ulong)spl_get_load_buffer(-sizeof(*header), sizeof(*header));
		ret = spl_ram_try_location(spl_image, bootdev, address);
		debug("\t >> ret = %d\n", ret);
	}

	return ret;
}

#if CONFIG_IS_ENABLED(RAM_DEVICE)
SPL_LOAD_IMAGE_METHOD("RAM", 0, BOOT_DEVICE_RAM, spl_ram_load_image);
#endif
#if CONFIG_IS_ENABLED(DFU)
SPL_LOAD_IMAGE_METHOD("DFU", 0, BOOT_DEVICE_DFU, spl_ram_load_image);
#endif
