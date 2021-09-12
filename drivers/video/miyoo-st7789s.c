// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <command.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <spi.h>
#include <video.h>
#include <asm/gpio.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/arch/gpio.h>

#define GOTTA_GO_FAST

// 240 x 320 display controller.
#define WIDTH   320
#define HEIGHT  240

#define DATA_BITS_COUNT 16

// Table 1
#define ST7789S_CMD_NOP        (0x00)
#define ST7789S_CMD_SWRESET    (0x01) // Software Reset
#define ST7789S_CMD_RDDID      (0x04) // Read Display ID
#define ST7789S_CMD_RDDST      (0x09) // Read Display Status
#define ST7789S_CMD_RDDPM      (0x0A) // Read Display Power Mode
#define ST7789S_CMD_RDDMADCTL  (0x0B) // Read Display MADCTL
#define ST7789S_CMD_RDDCOLMOD  (0x0C) // Read Display Pixel Format
#define ST7789S_CMD_RDDIM      (0x0D) // Read Display Image Mode
#define ST7789S_CMD_RDDSM      (0x0E) // Read Display Signal Mode
#define ST7789S_CMD_RDDSDR     (0x0F) // Read Display Self-Diagnostic Result
#define ST7789S_CMD_SLPIN      (0x10) // Sleep in
#define ST7789S_CMD_SLPOUT     (0x11) // Sleep Out
#define ST7789S_CMD_PTLON      (0x12) // Partial Display Mode On
#define ST7789S_CMD_NORON      (0x13) // Normal Display Mode On
#define ST7789S_CMD_INVOFF     (0x20) // Display Inversion Off
#define ST7789S_CMD_INVON      (0x21) // Display Inversion On
#define ST7789S_CMD_GAMSET     (0x26) // Gamma Set
#define ST7789S_CMD_DISPOFF    (0x28) // Display Off
#define ST7789S_CMD_DISPON     (0x29) // Display On
#define ST7789S_CMD_CASET      (0x2A) // Column Address Set
#define ST7789S_CMD_RASET      (0x2B) // Row Address Set
#define ST7789S_CMD_RAMWR      (0x2C) // Memory Write
#define ST7789S_CMD_RAMRD      (0x2E) // Memory Read
#define ST7789S_CMD_PTLAR      (0x30) // Partial Area
#define ST7789S_CMD_VSCRDEF    (0x33) // Vertical Scrolling Definition
#define ST7789S_CMD_TEOFF      (0x34) // Tearing Effect Line OFF
#define ST7789S_CMD_TEON       (0x35) // Tearing Effect Line On
#define ST7789S_CMD_MADCTL     (0x36) // Memory Data Access Control
#define ST7789S_CMD_VSCSAD     (0x37) // Vertical Scroll Start Address of RAM
#define ST7789S_CMD_IDMOFF     (0x38) // Idle Mode Off
#define ST7789S_CMD_IDMON      (0x39) // Idle mode on
#define ST7789S_CMD_COLMOD     (0x3A) // Interface Pixel Format
#define ST7789S_CMD_WRMEMC     (0x3C) // Write Memory Continue
#define ST7789S_CMD_RDMEMC     (0x3E) // Read Memory Continue
#define ST7789S_CMD_STE        (0x44) // Set Tear Scanline
#define ST7789S_CMD_GSCAN      (0x45) // Get Scanline
#define ST7789S_CMD_WRDISBV    (0x51) // Write Display Brightness
#define ST7789S_CMD_RDDISBV    (0x52) // Read Display Brightness Value
#define ST7789S_CMD_WRCTRLD    (0x53) // Write CTRL Display
#define ST7789S_CMD_RDCTRLD    (0x54) // Read CTRL Value Display
#define ST7789S_CMD_WRCACE     (0x55) // Write Content Adaptive Brightness Control and Color Enhancement
#define ST7789S_CMD_RDCABC     (0x56) // Read Content Adaptive Brightness Control
#define ST7789S_CMD_WRCABCMB   (0x5E) // Write CABC Minimum Brightness
#define ST7789S_CMD_RDCABCMB   (0x5F) // Read CABC Minimum Brightness
#define ST7789S_CMD_RDID1      (0xDA) // Read ID1
#define ST7789S_CMD_RDID2      (0xDB) // Read ID2
#define ST7789S_CMD_RDID3      (0xDC) // Read ID3

// Table 2
#define ST7789S_CMD_RAMCTRL    (0xB0) // RAM Control
#define ST7789S_CMD_RGBCTRL    (0xB1) // RGB Interface Control
#define ST7789S_CMD_PORCTRL    (0xB2) // Porch Setting
#define ST7789S_CMD_FRCTRL1    (0xB3) // Frame Rate Control 1 (In partial mode/ idle colors)
#define ST7789S_CMD_GCTRL      (0xB7) // Gate Control
#define ST7789S_CMD_DGMEN      (0xBA) // Digital Gamma Enable
#define ST7789S_CMD_VCOMS      (0xBB) // VCOM Setting
#define ST7789S_CMD_LCMCTRL    (0xC0) // LCM Control
#define ST7789S_CMD_IDSET      (0xC1) // ID Code Setting
#define ST7789S_CMD_VDVVRHEN   (0xC2) // VDV and VRH Command Enable
#define ST7789S_CMD_VRHS       (0xC3) // VRH Set
#define ST7789S_CMD_VDVS       (0xC4) // VDV Set
#define ST7789S_CMD_VCMOFSET   (0xC5) // VCOM Offset Set
#define ST7789S_CMD_FRCTRL2    (0xC6) // Frame Rate Control in Normal Mode
#define ST7789S_CMD_CABCCTRL   (0xC7) // CABC Control
#define ST7789S_CMD_REGSEL1    (0xC8) // Register Value Selection 1
#define ST7789S_CMD_REGSEL2    (0xCA) // Register Value Selection 2
#define ST7789S_CMD_PWCTRL1    (0xD0) // Power Control 1
#define ST7789S_CMD_VAPVANEN   (0xD2) // Enable VAP/VAN signal output
#define ST7789S_CMD_PVGAMCTRL  (0xE0) // Positive Voltage Gamma Control
#define ST7789S_CMD_NVGAMCTRL  (0xE1) // Negative Voltage Gamma Control
#define ST7789S_CMD_DGMLUTR    (0xE2) // Digital Gamma Look-up Table for Red
#define ST7789S_CMD_DGMLUTB    (0xE3) // Digital Gamma Look-up Table for Blue
#define ST7789S_CMD_GATECTRL   (0xE4) // Gate Control
#define ST7789S_CMD_SPI2EN     (0xE7) // SPI2 Enable
#define ST7789S_CMD_PWCTRL2    (0xE8) // Power Control 2
#define ST7789S_CMD_EQCTRL     (0xE9) // Equalize time control
#define ST7789S_CMD_PROMCTRL   (0xEC) // Program Mode Control
#define ST7789S_CMD_PROMEN     (0xFA) // Program Mode Enable
#define ST7789S_CMD_NVMSET     (0xFC) // NVM Setting
#define ST7789S_CMD_PROMACT    (0xFE) // Program action

/**
 * struct miyoo_st7789s_priv - Private structure
 * @backlight_gpio: Pin connected to the backlight (??)
 * @power_gpio: Pin connected to the power pin (??)
 * @dev: Device uclass for video_ops
 */
struct miyoo_st7789s_priv {
	struct gpio_desc backlight_gpio;
	struct gpio_desc power_gpio;

	struct gpio_desc data_gpios[DATA_BITS_COUNT];
	struct gpio_desc wrx_gpio;
	struct gpio_desc dcx_gpio;
	struct gpio_desc csx_gpio;

	struct udevice *dev;
};

#ifdef GOTTA_GO_FAST

// This byte-bangs data on the parallel interface.
// It is **much** faster than the bit-banged lcdc_output
// But it is extremely hardcoded to a specific device :/
static void lcdc_output(struct udevice *dev, bool is_data, uint16_t val)
{
	uint32_t in_value;
	struct sunxi_gpio_reg * const gpio = (struct sunxi_gpio_reg *)SUNXI_PIO_BASE;

	// Note: PD0 and PD9 are used by gamepad buttons, and skipped.
	// Decompose 16bit byte pair in bits PD1:8 and PD10:17
	in_value = (val & 0x00ff) << 1;
	in_value|= (val & 0xff00) << 2;
	// Assuming 0xffff, in_value is now `111111110111111110`
	// Two bundles of bits spaced by one.
	// This matches, from LSB, from PD0 to PD17,
	// where the gaps are "conflicting" features.

	// PD19 is D/CX; high (1) when data is transferred.
	in_value|= is_data ? 0x80000 : 0;

	// Sets PD20 (CSX) high (always)
	in_value|= (1 << 20);

	// Write once with PD18 (WRX) set to 0
	writel(in_value, &gpio->gpio_bank[SUNXI_GPIO_D].dat);
	in_value|= (1 << 18);
	// Write again with PD18 (WRX) set to 1
	writel(in_value, &gpio->gpio_bank[SUNXI_GPIO_D].dat);
	// This toggling of PD18 (WRX) causes a read on rising edge.
}

#else

#error Slow path for st7789s driver does not work.

#define DEBUG

// This slows down U-Boot **considerably**.
// Not sure if there's a way forward to keep this clean as far as DM goes.
static uint32_t lcdc_output(struct udevice *dev, bool is_data, uint16_t val)
{
	int i;
	uint32_t ret;

	struct miyoo_st7789s_priv *priv = dev_get_priv(dev);

	for (i = 0; i < DATA_BITS_COUNT; i++) {
		ret = dm_gpio_set_value(&priv->data_gpios[i], val & BIT(i));
#ifdef DEBUG
		if (ret) {
			dev_err(dev, "Error %d when writing data bit %d", ret, i);
			return ret;
		}
#endif
	}

	ret = dm_gpio_set_value(&priv->dcx_gpio, is_data);
#ifdef DEBUG
	if (ret) {
		dev_err(dev, "Error %d when setting dcx to %d", ret, is_data);
		return ret;
	}
#endif

	ret = dm_gpio_set_value(&priv->csx_gpio, 1);
#ifdef DEBUG
	if (ret) {
		dev_err(dev, "Error %d when setting csx high", ret);
		return ret;
	}
#endif

	ret = dm_gpio_set_value(&priv->wrx_gpio, 0);
#ifdef DEBUG
	if (ret) {
		dev_err(dev, "Error %d when toggling WRX off", ret);
		return ret;
	}
#endif

	ret = dm_gpio_set_value(&priv->wrx_gpio, 1);
#ifdef DEBUG
	if (ret) {
		dev_err(dev, "Error %d when toggling WRX on", ret);
		return ret;
	}
#endif
}

#endif // GOTTA_GO_FAST

static void lcd_wr_cmd(struct udevice *dev, uint16_t reg)
{
	lcdc_output(dev, 0, reg);
}
static void lcd_wr_dat(struct udevice *dev, uint16_t reg)
{
	lcdc_output(dev, 1, reg);
}

static uint32_t miyoo_st7789s_lcd_init(struct udevice *dev)
{
	uint32_t ret;
	uint32_t i;

	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct miyoo_st7789s_priv *priv = dev_get_priv(dev);

	// --- power ---

	// Off
	ret = dm_gpio_set_value(&priv->power_gpio, 0);
	if (ret)
		return ret;
	mdelay(250);

	// On
	ret = dm_gpio_set_value(&priv->power_gpio, 1);
	if (ret)
		return ret;

	mdelay(250);

	// --- st7789s ---

	lcd_wr_cmd(dev, ST7789S_CMD_SLPOUT);
	mdelay(150);

	lcd_wr_cmd(dev, ST7789S_CMD_MADCTL);
	// TODO: vertical flip defined in dts
	// This flips the display upside down on v90
	if (0) {
		lcd_wr_dat(dev, 0x70); // 1110000 → MX MV ML
	}
	else {
		// Correct orientation on v90
		lcd_wr_dat(dev, 0xB0); //
	}

	// Start the "dance" required to setup the display...

	lcd_wr_cmd(dev, ST7789S_CMD_COLMOD);
	lcd_wr_dat(dev, 0x05); // 101 → 16bpp

	lcd_wr_cmd(dev, ST7789S_CMD_CASET);
	lcd_wr_dat(dev, 0x00);
	lcd_wr_dat(dev, 0x00);
	lcd_wr_dat(dev, 0x01);
	lcd_wr_dat(dev, 0x3f);

	lcd_wr_cmd(dev, ST7789S_CMD_RASET);
	lcd_wr_dat(dev, 0x00);
	lcd_wr_dat(dev, 0x00);
	lcd_wr_dat(dev, 0x00);
	lcd_wr_dat(dev, 0xef);

	lcd_wr_cmd(dev, ST7789S_CMD_PORCTRL);
	lcd_wr_dat(dev, 0x08);
	lcd_wr_dat(dev, 0x08);
	lcd_wr_dat(dev, 0x00);
	lcd_wr_dat(dev, 0x33);
	lcd_wr_dat(dev, 0x33);

	lcd_wr_cmd(dev, ST7789S_CMD_GCTRL);
	lcd_wr_dat(dev, 0x35);

	lcd_wr_cmd(dev, ST7789S_CMD_VCOMS);
	lcd_wr_dat(dev, 0x15);

	lcd_wr_cmd(dev, ST7789S_CMD_LCMCTRL);
	lcd_wr_dat(dev, 0x3C);

	lcd_wr_cmd(dev, ST7789S_CMD_VDVVRHEN);
	lcd_wr_dat(dev, 0x01);

	lcd_wr_cmd(dev, ST7789S_CMD_VRHS);
	lcd_wr_dat(dev, 0x13);

	lcd_wr_cmd(dev, ST7789S_CMD_VDVS);
	lcd_wr_dat(dev, 0x20);

	lcd_wr_cmd(dev, ST7789S_CMD_FRCTRL2);
	lcd_wr_dat(dev, 0x04);

	lcd_wr_cmd(dev, ST7789S_CMD_PWCTRL1);
	lcd_wr_dat(dev, 0xa4);
	lcd_wr_dat(dev, 0xa1);

	lcd_wr_cmd(dev, ST7789S_CMD_PWCTRL2);
	lcd_wr_dat(dev, 0x03);

	lcd_wr_cmd(dev, ST7789S_CMD_EQCTRL);
	lcd_wr_dat(dev, 0x0d);
	lcd_wr_dat(dev, 0x12);
	lcd_wr_dat(dev, 0x00);

	lcd_wr_cmd(dev, ST7789S_CMD_PVGAMCTRL);
	lcd_wr_dat(dev, 0x70);
	lcd_wr_dat(dev, 0x00);
	lcd_wr_dat(dev, 0x06);
	lcd_wr_dat(dev, 0x09);
	lcd_wr_dat(dev, 0x0b);
	lcd_wr_dat(dev, 0x2a);
	lcd_wr_dat(dev, 0x3c);
	lcd_wr_dat(dev, 0x33);
	lcd_wr_dat(dev, 0x4b);
	lcd_wr_dat(dev, 0x08);
	lcd_wr_dat(dev, 0x16);
	lcd_wr_dat(dev, 0x14);
	lcd_wr_dat(dev, 0x2a);
	lcd_wr_dat(dev, 0x23);

	lcd_wr_cmd(dev, ST7789S_CMD_NVGAMCTRL);
	lcd_wr_dat(dev, 0xd0);
	lcd_wr_dat(dev, 0x00);
	lcd_wr_dat(dev, 0x06);
	lcd_wr_dat(dev, 0x09);
	lcd_wr_dat(dev, 0x0b);
	lcd_wr_dat(dev, 0x29);
	lcd_wr_dat(dev, 0x36);
	lcd_wr_dat(dev, 0x54);
	lcd_wr_dat(dev, 0x4b);
	lcd_wr_dat(dev, 0x0d);
	lcd_wr_dat(dev, 0x16);
	lcd_wr_dat(dev, 0x14);
	lcd_wr_dat(dev, 0x28);
	lcd_wr_dat(dev, 0x22);

	// Start a transfer to the MCU
	lcd_wr_cmd(dev, ST7789S_CMD_RAMWR);

	// Clear the framebuffer, prevents garbage from being seen.
	for (i = 0; i < (uc_priv->xsize * uc_priv->ysize); i++) {
		lcd_wr_dat(dev, 0x0000);
	}

	// Turn the display on
	lcd_wr_cmd(dev, ST7789S_CMD_DISPON); // <DISPON> Display On


	// Turn the backlight on
	ret = dm_gpio_set_value(&priv->backlight_gpio, 1);
	if (ret)
		return ret;

	return 0;
}

static int miyoo_st7789s_sync(struct udevice *vid)
{
	struct video_priv *uc_priv = dev_get_uclass_priv(vid);
	int i;
	u8 data1, data2;
	u8 *buf = uc_priv->fb;

	// Start a Transfer to the MCU
	lcd_wr_cmd(vid, ST7789S_CMD_RAMWR);

	// Shove bytes to the display
	for (i = 0; i < (uc_priv->xsize * uc_priv->ysize); i++) {
		data2 = *buf++;
		data1 = *buf++;
		lcd_wr_dat(vid, (data1 << 8) + data2);
	}

	return 0;
}

static int miyoo_st7789s_probe(struct udevice *dev)
{
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct miyoo_st7789s_priv *priv = dev_get_priv(dev);
	int ret;

	ret = gpio_request_by_name(dev, "backlight-gpio", 0,
				   &priv->backlight_gpio, GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "error %d requesting backlight GPIO\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "power-gpio", 0,
				   &priv->power_gpio, GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "error %d requesting power GPIO\n", ret);
		return ret;
	}

	uc_priv->xsize = WIDTH;
	uc_priv->ysize = HEIGHT;

	// Hardcoded and assumed to be RGB565
	uc_priv->bpix = VIDEO_BPP16;	/* Uses RGB565 format */

	// TODO: get from DT
	uc_priv->rot = 0;

	// Parallel interface configuration

	ret = gpio_request_list_by_name(
			dev,
			"lcd-data-gpios",
			priv->data_gpios,
			DATA_BITS_COUNT,
			GPIOD_IS_OUT
			);
	if (ret != DATA_BITS_COUNT) {
		dev_err(dev, "error %d requesting data GPIO\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "lcd-wrx", 0,
				   &priv->wrx_gpio, GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "error %d requesting wrx GPIO\n", ret);
		return ret;
	}
	ret = gpio_request_by_name(dev, "lcd-dcx", 0,
				   &priv->dcx_gpio, GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "error %d requesting dcx GPIO\n", ret);
		return ret;
	}
	ret = gpio_request_by_name(dev, "lcd-csx", 0,
				   &priv->csx_gpio, GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "error %d requesting csx GPIO\n", ret);
		return ret;
	}

	miyoo_st7789s_lcd_init(dev);

	priv->dev = dev;

	return 0;
}

static int miyoo_st7789s_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	// 2 is hardcoded for VIDEO_BPP16
	plat->size = WIDTH * HEIGHT * 2;

	return 0;
}

static const struct video_ops miyoo_st7789s_ops = {
	.video_sync = miyoo_st7789s_sync,
};

static const struct udevice_id miyoo_st7789s_ids[] = {
	{ .compatible = "sitronix,miyoo-st7789s" },
	{ }
};

U_BOOT_DRIVER(miyoo_st7789s_video) = {
	.name = "miyoo_st7789s_video",
	.id = UCLASS_VIDEO,
	.of_match = miyoo_st7789s_ids,
	.ops = &miyoo_st7789s_ops,
	.plat_auto = sizeof(struct video_uc_plat),
	.bind = miyoo_st7789s_bind,
	.probe = miyoo_st7789s_probe,
	.priv_auto = sizeof(struct miyoo_st7789s_priv),
};
