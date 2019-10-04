/*
 * s3c2416_lcd.h
 *
 *  Created on: Feb 15, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_DISPLAY_S3C2416_LCD_H_
#define INCLUDE_HW_DISPLAY_S3C2416_LCD_H_
#include "hw/sysbus.h"
#include "qemu-common.h"
#include "qemu/typedefs.h"
#include "ui/console.h"

#define TYPE_S3C2416_LCD "s3c2416-lcd"

#define S3C2416_LCD(obj) OBJECT_CHECK(S3C2416LCDState, (obj), TYPE_S3C2416_LCD)

enum s3c2416_lcd_bpp_type {
	pallet,
	rgb1232,
	rgb565,
	rgb1555,
	rgb666,
	rgb1665,
	rgb1666,
	rgb888,
	rgb1887,
	rgb1888,
	rgb4888,
};
struct s3c2416_lcd_pixel {
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	unsigned int alpha;
};
struct s3c2416_lcd_winbuf {
	unsigned int pixel_bits;
	unsigned int page_width;
	unsigned int offsize;
	hwaddr start;
	hwaddr end;
	MemoryRegionSection sec;
};

struct s3c2416_lcd_win {
	struct s3c2416_lcd_winbuf *buf;
	unsigned int right;
	unsigned int top;
	unsigned int left;
	unsigned int bottom;
	unsigned int enabled;
	enum s3c2416_lcd_bpp_type bpp_type;
	enum s3c2416_lcd_bpp_type palette_bpp_type;
	uint32_t palette[256];
};

typedef struct S3C2416LCDState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	MemoryRegion iomem;
	const GraphicHwOps *graph_ops;
	QemuConsole *console;
	bool invalidate;

	struct s3c2416_lcd_win win0;
	struct s3c2416_lcd_win win1;

	struct s3c2416_lcd_winbuf win0_buf0;
	struct s3c2416_lcd_winbuf win0_buf1;
	struct s3c2416_lcd_winbuf win1_buf0;

	bool enabled;

	unsigned int win1_alpha0_red;
	unsigned int win1_alpha0_green;
	unsigned int win1_alpha0_blue;
	unsigned int win1_alpha1_red;
	unsigned int win1_alpha1_green;
	unsigned int win1_alpha1_blue;

	int xres;
	int yres;

	int panel_xres;
	int panel_yres;
} S3C2416LCDState;

#endif /* INCLUDE_HW_DISPLAY_S3C2416_LCD_H_ */
