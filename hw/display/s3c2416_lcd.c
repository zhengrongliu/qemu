/*
 * s3c2416_lcd.c
 *
 *  Created on: Dec 22, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu/typedefs.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/display/framebuffer.h"
#include "hw/display/s3c2416_lcd.h"
#include "ui/pixel_ops.h"
#include "exec/address-spaces.h"
#include "qemu-common.h"

#define LCD_REGION_SIZE 0x1000

#define VIDCON0 0x0
#define VICCON1 0x4
#define VIDTCON0 0x8
#define VIDTCON1 0xc
#define VIDTCON2 0x10
#define WINCON0 0x14
#define WINCON1 0x18
#define VIDOSD0A 0x28
#define VIDOSD0B 0x2c
#define VIDOSD1A 0x34
#define VIDOSD1B 0x38
#define VIDOSD1C 0x3c
#define VIDW00ADD0B0 0x64
#define VIDW00ADD0B1 0x68
#define VIDW01ADD0 0x6c
#define VIDW00ADD1B0 0x7c
#define VIDW00ADD1B1 0x80
#define VIDW01ADD1 0x84
#define VIDW00ADD2B0 0x94
#define VIDW00ADD2B1 0x98
#define VIDW01ADD2 0x9c
#define VIDINTCON 0xac
#define W1KEYCON0 0xB0
#define W1KEYCON1 0xB4
#define W2KEYCON0 0xB8
#define W2KEYCON1 0xBC
#define W3KEYCON0 0xC0
#define W3KEYCON1 0xC4
#define W4KEYCON0 0xC8
#define W4KEYCON1 0xCC
#define WIN0MAP 0xD0
#define WIN1MAP 0xD4
#define WPALCON 0xE4
#define SYSIFCON0 0x130
#define SYSIFCON1 0x134
#define DITHMODE 0x138
#define SIFCCON0 0x13c
#define SIFCCON1 0x144
#define CPUTRIGCON2 0x160
#define WIN0_PALETTE_BASE 0x400
#define WIN0_PALETTE_END 0x800
#define WIN1_PALETTE_BASE 0x800
#define WIN1_PALETTE_END 0xC00

/*
 * VIDCON0
 */
#define VIDCON0_FORMAT_SHIFT 22
#define VIDCON0_FORMAT_RGB 0x0
#define VIDCON0_FORMAT_I80_MAIN 0x2
#define VIDCON0_FORMAT_I80_SUB 0x3
#define VIDCON0_FORMAT_MASK 0x03
#define VIDCON0_PNRMODE_SHIFT 13
#define VIDCON0_PNRMODE_PARALLET_RGB 0x0
#define VIDCON0_PNRMODE_PARALLET_BGR 0x1
#define VIDCON0_VCLK_ENABLE (0x1 << 5)
#define VIDCON0_VCLK_DISABLE (0x0 << 5)
#define VIDCON0_ENVID_SHIFT 0x0
#define VIDCON0_ENVID_DISABLE_IMMD 0x0
#define VIDCON0_ENVID_DISABLE_ATEF 0x2
#define VIDCON0_ENVID_ENABLE 0x3

#define VIDWXXADD_PAGE_WIDTH_SHIFT 0
#define VIDWXXADD_PAGE_WIDTH_MASK 0x1fff;
#define VIDWXXADD_OFFSIZE_SHIFT 13
#define VIDWXXADD_OFFSIZE_MASK 0x1fff;

/*
 * window x control register
 */
#define WINCON_BUFSTATUS_0 (0 << 24)
#define WINCON_BUFSTATUS_1 (1 << 24)
#define WINCON_BUFSEL_MASK (1 << 23)
#define WINCON_BUFSEL_0 (0 << 23)
#define WINCON_BUFSEL_1 (1 << 23)
#define WINCON_BUFAUTO_DISABLE (0 << 22)
#define WINCON_BUFAUTO_ENABLE (1 << 22)
#define WINCON_BITSWAP_DISABLE (0 << 18)
#define WINCON_BITSWAP_ENABLE (1 << 18)
#define WINCON_BYTESWAP_DISABLE (0 << 17)
#define WINCON_BYTESWAP_ENABLE (1 << 17)
#define WINCON_HWSWAP_DISABLE (0 << 16)
#define WINCON_HWSWAP_ENABLE (1 << 16)
#define WINCON_BURSTLEN_SHIFT (9)
#define WINCON_BURSTLEN_MASK (0x3)
#define WINCON_BURSTLEN_16 (0)
#define WINCON_BURSTLEN_8 (1)
#define WINCON_BURSTLEN_4 (2)
#define WINCON_BLEND_PER_PLANE (0 << 6)
#define WINCON_BLEND_PER_PIXEL (1 << 6)
#define WINCON_BPPMODE_SHIFT (2)
#define WINCON_BPPMODE_PALLETIZE_1BPP (0)
#define WINCON_BPPMODE_PALLETIZE_2BPP (1)
#define WINCON_BPPMODE_PALLETIZE_4BPP (2)
#define WINCON_BPPMODE_PALLETIZE_8BPP (3)
#define WINCON_BPPMODE_RGB565 (5)
#define WINCON_BPPMODE_RGB1555 (7)
#define WINCON_BPPMODE_RGB666 (8)
#define WINCON_BPPMODE_RGB888 (11)
#define WINCON_BPPMODE_MASK (0xf)
#define WINCON_ALPHA_SEL_0 (0 << 1)
#define WINCON_ALPHA_SEL_1 (1 << 1)
#define WINCON_DISABLE (0 << 0)
#define WINCON_ENABLE (1 << 0)

/*
 * Video time control 2 regsiter
 */
#define VIDTCON2_LINEVAL_SHIFT (11)
#define VIDTCON2_LINEVAL_MASK (0x7ff)
#define VIDTCON2_HOZVAL_SHIFT (0)
#define VIDTCON2_HOZVAL_MASK (0x7ff)

/*
 * Window x position control register
 */
#define VIDOSD_LEFT_SHIFT (11)
#define VIDOSD_LEFT_MASK (0x7ff)
#define VIDOSD_TOP_SHIFT (0)
#define VIDOSD_TOP_MASK (0x7ff)
#define VIDOSD_RIGHT_SHIFT (11)
#define VIDOSD_RIGHT_MASK (0x7ff)
#define VIDOSD_BOTTOM_SHIFT (0)
#define VIDOSD_BOTTOM_MASK (0x7ff)

/*
 * window alpha value regsiter
 */
#define VIDOSD_ALPHA0_RED_SHIFT (20)
#define VIDOSD_ALPHA0_GREEN_SHIFT (16)
#define VIDOSD_ALPHA0_BLUE_SHIFT (12)
#define VIDOSD_ALPHA1_RED_SHIFT (8)
#define VIDOSD_ALPHA1_GREEN_SHIFT (4)
#define VIDOSD_ALPHA1_BLUE_SHIFT (0)
#define VIDOSD_ALPHA_VALUE_MASK (0xf)

static struct s3c2416_lcd_pixel
s3c2416_lcd_pixel_blend(S3C2416LCDState *ls, struct s3c2416_lcd_pixel *p0,
			struct s3c2416_lcd_pixel *p1)
{
	struct s3c2416_lcd_pixel pixel = {};
	double ar = 0, ag = 0, ab = 0;

	pixel.red = ((double)p0->red) * (1 - ar) + ((double)p1->red) * ar;
	pixel.green = ((double)p0->green) * (1 - ag) + ((double)p1->green) * ag;
	pixel.blue = ((double)p0->blue) * (1 - ab) + ((double)p1->blue) * ab;

	return pixel;
}

static struct s3c2416_lcd_pixel
s3c2416_lcd_win_pixel(struct s3c2416_lcd_win *win, int row, int col)
{
	struct s3c2416_lcd_winbuf *buf = win->buf;
	enum s3c2416_lcd_bpp_type bpp = win->bpp_type;
	enum s3c2416_lcd_bpp_type palette_bpp = win->palette_bpp_type;
	uint32_t value;
	uint8_t *addr;
	int len;
	int bits;
	struct s3c2416_lcd_pixel pixel = {};

	len = buf->page_width + buf->offsize;

	addr = memory_region_get_ram_ptr(buf->sec.mr) +
	       buf->sec.offset_within_region;
	bits = col * buf->pixel_bits;
	addr += row * len + bits / 8;
	bits = bits % 8;

	if (bpp == pallet) {
		value =
		    ((*(uint8_t *)addr) >> bits) & ((1 << buf->pixel_bits) - 1);
		addr = (uint8_t *)(win->palette + value);
		bpp = palette_bpp;
	}

	switch (bpp) {
	case rgb1232:
		pixel.alpha = ((*addr) >> 7) & 0x1;
		pixel.red = ((*addr) >> 5) & 0x3;
		pixel.green = ((*addr) >> 2) & 0x7;
		pixel.blue = (*addr) & 0x3;
		break;
	case rgb565:
		pixel.red = ((*(uint16_t *)addr) >> 11) & 0x1f;
		pixel.green = ((*(uint16_t *)addr) >> 5) & 0x3f;
		pixel.blue = (*(uint16_t *)addr) & 0x1f;
		break;
	case rgb1555:
		pixel.alpha = ((*(uint16_t *)addr) >> 15) & 0x1;
		pixel.red = ((*(uint16_t *)addr) >> 10) & 0x1f;
		pixel.green = ((*(uint16_t *)addr) >> 5) & 0x1f;
		pixel.blue = (*(uint16_t *)addr) & 0x1f;
		break;
	case rgb666:
		pixel.red = ((*(uint32_t *)addr) >> 12) & 0x3f;
		pixel.green = ((*(uint32_t *)addr) >> 6) & 0x3f;
		pixel.blue = (*(uint32_t *)addr) & 0x3f;
		break;
	case rgb1665:
		pixel.alpha = ((*(uint32_t *)addr) >> 17) & 0x1;
		pixel.red = ((*(uint32_t *)addr) >> 11) & 0x3f;
		pixel.green = ((*(uint32_t *)addr) >> 5) & 0x3f;
		pixel.blue = (*(uint32_t *)addr) & 0x1f;
		break;
	case rgb1666:
		pixel.alpha = ((*(uint32_t *)addr) >> 18) & 0x1;
		pixel.red = ((*(uint32_t *)addr) >> 12) & 0x3f;
		pixel.green = ((*(uint32_t *)addr) >> 6) & 0x3f;
		pixel.blue = (*(uint32_t *)addr) & 0x3f;
		break;
	case rgb888:
		pixel.red = ((*(uint32_t *)addr) >> 16) & 0xff;
		pixel.green = ((*(uint32_t *)addr) >> 8) & 0xff;
		pixel.blue = (*(uint32_t *)addr) & 0xff;
		break;
	case rgb1887:
		pixel.alpha = ((*(uint32_t *)addr) >> 23) & 0x1;
		pixel.red = ((*(uint32_t *)addr) >> 15) & 0xff;
		pixel.green = ((*(uint32_t *)addr) >> 7) & 0xff;
		pixel.blue = (*(uint32_t *)addr) & 0x7f;
		break;
	case rgb1888:
		pixel.alpha = ((*(uint32_t *)addr) >> 24) & 0x1;
		pixel.red = ((*(uint32_t *)addr) >> 16) & 0xff;
		pixel.green = ((*(uint32_t *)addr) >> 8) & 0xff;
		pixel.blue = (*(uint32_t *)addr) & 0xff;
		break;
	case rgb4888:
		pixel.alpha = ((*(uint32_t *)addr) >> 24) & 0xf;
		pixel.red = ((*(uint32_t *)addr) >> 16) & 0xff;
		pixel.green = ((*(uint32_t *)addr) >> 8) & 0xff;
		pixel.blue = (*(uint32_t *)addr) & 0xff;
		break;
	default:
		break;
	}

	return pixel;
}

static bool s3c2416_lcd_need_update(S3C2416LCDState *ls, int row)
{
	int len = 0;
	bool dirty;
	ram_addr_t addr;
	assert(row >= 0 && row <= ls->yres);
	assert(ls->win0.enabled);

	len = ls->win0.buf->offsize + ls->win0.buf->page_width;
	addr = ls->win0.buf->sec.offset_within_region + len * row;
	dirty = memory_region_get_dirty(ls->win0.buf->sec.mr, addr, len,
					DIRTY_MEMORY_VGA);

	if (ls->win1.enabled) {
		len = ls->win1.buf->offsize + ls->win1.buf->page_width;
		row -= ls->win1.top - ls->win0.top;
		addr = ls->win1.buf->sec.offset_within_region + len * row;
		dirty |= memory_region_get_dirty(ls->win1.buf->sec.mr, addr,
						 len, DIRTY_MEMORY_VGA);
	}

	return dirty;
}

static void s3c2416_lcd_draw_row(S3C2416LCDState *ls, int row)
{
	DisplaySurface *surface = qemu_console_surface(ls->console);
	uint8_t *dest = surface_data(surface);
	struct s3c2416_lcd_pixel p0, p1;
	int bpp = surface_bits_per_pixel(surface);
	int width, height;
	uint32_t value;
	int col;

	height = surface_height(surface);
	if (row > height) {
		return;
	}

	width = surface_width(surface);
	dest += (row * width * bpp) / 8;

	width = width < ls->xres ? width : ls->xres;
	for (col = 0; col < width; col++) {
		p0 = s3c2416_lcd_win_pixel(&ls->win0, row, col);
		if (ls->win1.enabled &&
		    (row + ls->win0.top > ls->win1.top &&
		     row + ls->win0.top < ls->win1.bottom) &&
		    (col + ls->win0.left > ls->win1.left &&
		     col + ls->win0.left < ls->win0.right)) {
			p1 = s3c2416_lcd_win_pixel(
			    &ls->win1, row - (ls->win1.top - ls->win0.top),
			    col - (ls->win1.left - ls->win0.left));

			p0 = s3c2416_lcd_pixel_blend(ls, &p0, &p1);
		}

		switch (bpp) {
		case 8:
			*dest++ = rgb_to_pixel8(p0.red, p0.green, p0.blue);
			break;
		case 15:
			*(uint16_t *)dest =
			    rgb_to_pixel15(p0.red, p0.green, p0.blue);
			dest += 2;
			break;
		case 16:
			*(uint16_t *)dest =
			    rgb_to_pixel16(p0.red, p0.green, p0.blue);
			dest += 2;
			break;
		case 24:
			value = rgb_to_pixel24(p0.red, p0.green, p0.blue);
			*dest++ = value & 0xff;
			*dest++ = (value >> 8) & 0xff;
			*dest++ = (value >> 16) & 0xff;
			break;
		case 32:
			*(uint32_t *)dest =
			    rgb_to_pixel32(p0.red, p0.green, p0.blue);
			dest += 4;
			break;
		default:
			break;
		}
	}
}

static void s3c2416_lcd_update_dirty_region(S3C2416LCDState *ls, int *pfirst,
					    int *plast)
{
	int row = *pfirst;
	bool need_update;
	bool first = true;

	for (row = 0; row < ls->yres; row++) {
		need_update = s3c2416_lcd_need_update(ls, row);

		if (need_update || ls->invalidate) {
			s3c2416_lcd_draw_row(ls, row);
			*plast = row;
			if (first) {
				*pfirst = row;
				first = false;
			}
		}
	}
}

static void s3c2416_lcd_invalidate(void *opaque)
{
	S3C2416LCDState *ls = S3C2416_LCD(opaque);

	ls->invalidate = true;
}

static void s3c2416_lcd_gfx_update(void *opaque)
{
	S3C2416LCDState *ls = S3C2416_LCD(opaque);
	int first = 0, last = 0;

	if (!ls->enabled || !ls->win0.enabled) {
		return;
	}

	if (ls->invalidate) {
		framebuffer_update_memory_section(
		    &ls->win0.buf->sec, get_system_memory(),
		    ls->win0.buf->start, ls->yres,
		    ls->win0.buf->page_width + ls->win0.buf->offsize);
		if (ls->win1.enabled) {
			framebuffer_update_memory_section(
			    &ls->win1.buf->sec, get_system_memory(),
			    ls->win1.buf->start, 0, 0);
		}
	}

	memory_region_sync_dirty_bitmap(ls->win0.buf->sec.mr);
	if (ls->win1.enabled) {
		memory_region_sync_dirty_bitmap(ls->win1.buf->sec.mr);
	}

	s3c2416_lcd_update_dirty_region(ls, &first, &last);

	memory_region_reset_dirty(
	    ls->win0.buf->sec.mr, ls->win0.buf->sec.offset_within_region,
	    (ls->win0.buf->page_width + ls->win0.buf->offsize) * ls->yres,
	    DIRTY_MEMORY_VGA);
	if (ls->win1.enabled) {
		memory_region_reset_dirty(
		    ls->win1.buf->sec.mr,
		    ls->win1.buf->sec.offset_within_region,
		    (ls->win1.buf->page_width + ls->win1.buf->offsize) *
			ls->yres,
		    DIRTY_MEMORY_VGA);
	}

	if (first >= 0) {
		dpy_gfx_update(ls->console, 0, first, ls->xres,
			       last - first + 1);
	}

	ls->invalidate = false;
}

static uint64_t s3c2416_lcd_read(void *opaque, hwaddr offset, unsigned size)
{
	S3C2416LCDState *ls = S3C2416_LCD(opaque);
	uint64_t value = 0;
	switch (offset) {
	default:
		if (offset > WIN1_PALETTE_BASE && offset < WIN1_PALETTE_END) {
			value = ls->win1.palette[offset - WIN1_PALETTE_BASE];
		} else if (offset > WIN0_PALETTE_BASE &&
			   offset < WIN0_PALETTE_END) {
			value = ls->win0.palette[offset - WIN0_PALETTE_BASE];
		}
		break;
	}

	return value;
}

static void s3c2416_lcd_win_config(struct s3c2416_lcd_win *win, uint32_t value)
{
	int bpp;

	if (value & WINCON_ENABLE) {
		win->enabled = true;
	} else {
		win->enabled = false;
	}

	bpp = (value >> WINCON_BPPMODE_SHIFT) & WINCON_BPPMODE_MASK;
	switch (bpp) {
	case WINCON_BPPMODE_RGB565:
		win->bpp_type = rgb565;
		break;
	case WINCON_BPPMODE_RGB1555:
		win->bpp_type = rgb1555;
		break;
	case WINCON_BPPMODE_RGB666:
		win->bpp_type = rgb666;
		break;
	case WINCON_BPPMODE_RGB888:
		win->bpp_type = rgb888;
		break;
	}
}

static void s3c2416_lcd_write(void *opaque, hwaddr offset, uint64_t value,
			      unsigned size)
{
	S3C2416LCDState *ls = S3C2416_LCD(opaque);
	switch (offset) {
	case VIDCON0:
		if (value & VIDCON0_ENVID_ENABLE) {
			ls->enabled = true;
		} else {
			ls->enabled = false;
		}
		break;
	case VIDOSD0A:
		ls->win0.left = (value >> VIDOSD_LEFT_SHIFT) & VIDOSD_LEFT_MASK;
		ls->win0.top = (value >> VIDOSD_TOP_SHIFT) & VIDOSD_TOP_MASK;
		break;
	case VIDOSD0B:
		ls->win0.right =
		    (value >> VIDOSD_RIGHT_SHIFT) & VIDOSD_RIGHT_MASK;
		ls->win0.bottom =
		    (value >> VIDOSD_BOTTOM_SHIFT) & VIDOSD_BOTTOM_MASK;
		break;
	case VIDOSD1A:
		ls->win1.left = (value >> VIDOSD_LEFT_SHIFT) & VIDOSD_LEFT_MASK;
		ls->win1.top = (value >> VIDOSD_TOP_SHIFT) & VIDOSD_TOP_MASK;
		break;
	case VIDOSD1B:
		ls->win1.right =
		    (value >> VIDOSD_RIGHT_SHIFT) & VIDOSD_RIGHT_MASK;
		ls->win1.bottom =
		    (value >> VIDOSD_BOTTOM_SHIFT) & VIDOSD_BOTTOM_MASK;
		break;
	case VIDTCON2:
		ls->xres = ((value >> VIDTCON2_LINEVAL_SHIFT) &
			    VIDTCON2_LINEVAL_MASK) +
			   1;
		ls->yres =
		    ((value >> VIDTCON2_HOZVAL_SHIFT) & VIDTCON2_HOZVAL_MASK) +
		    1;
		break;
	case VIDOSD1C:
		ls->win1_alpha0_red = (value >> VIDOSD_ALPHA0_RED_SHIFT) &
				      VIDOSD_ALPHA_VALUE_MASK;
		ls->win1_alpha0_green = (value >> VIDOSD_ALPHA0_GREEN_SHIFT) &
					VIDOSD_ALPHA_VALUE_MASK;
		ls->win1_alpha0_blue = (value >> VIDOSD_ALPHA0_BLUE_SHIFT) &
				       VIDOSD_ALPHA_VALUE_MASK;
		ls->win1_alpha1_red = (value >> VIDOSD_ALPHA1_RED_SHIFT) &
				      VIDOSD_ALPHA_VALUE_MASK;
		ls->win1_alpha1_green = (value >> VIDOSD_ALPHA1_GREEN_SHIFT) &
					VIDOSD_ALPHA_VALUE_MASK;
		ls->win1_alpha1_blue = (value >> VIDOSD_ALPHA1_BLUE_SHIFT) &
				       VIDOSD_ALPHA_VALUE_MASK;
		break;
	case WINCON0:
		if ((value & WINCON_BUFSEL_MASK) == WINCON_BUFSEL_0) {
			ls->win0.buf = &ls->win0_buf0;
		} else {
			ls->win0.buf = &ls->win0_buf1;
		}
		s3c2416_lcd_win_config(&ls->win0, value);
		break;
	case WINCON1:
		s3c2416_lcd_win_config(&ls->win1, value);
		break;
	case VIDW00ADD0B0:
		ls->win0_buf0.start = value;
		break;
	case VIDW00ADD0B1:
		ls->win0_buf1.start = value;
		break;
	case VIDW01ADD0:
		ls->win1_buf0.start = value;
		break;
	case VIDW00ADD1B0:
		ls->win0_buf0.end = value;
		break;
	case VIDW00ADD1B1:
		ls->win0_buf1.end = value;
		break;
	case VIDW01ADD1:
		ls->win1_buf0.end = value;
		break;
	case VIDW00ADD2B0:
		ls->win0_buf0.page_width =
		    (value >> VIDWXXADD_PAGE_WIDTH_SHIFT) &
		    VIDWXXADD_PAGE_WIDTH_MASK;
		ls->win0_buf0.offsize =
		    (value >> VIDWXXADD_OFFSIZE_SHIFT) & VIDWXXADD_OFFSIZE_MASK;
		break;
	case VIDW00ADD2B1:
		ls->win0_buf1.page_width =
		    (value >> VIDWXXADD_PAGE_WIDTH_SHIFT) &
		    VIDWXXADD_PAGE_WIDTH_MASK;
		ls->win0_buf1.offsize =
		    (value >> VIDWXXADD_OFFSIZE_SHIFT) & VIDWXXADD_OFFSIZE_MASK;
		break;
	case VIDW01ADD2:
		ls->win1_buf0.page_width =
		    (value >> VIDWXXADD_PAGE_WIDTH_SHIFT) &
		    VIDWXXADD_PAGE_WIDTH_MASK;
		ls->win1_buf0.offsize =
		    (value >> VIDWXXADD_OFFSIZE_SHIFT) & VIDWXXADD_OFFSIZE_MASK;
		break;
	default:
		if (offset > WIN1_PALETTE_BASE && offset < WIN1_PALETTE_END) {
			ls->win1.palette[(offset - WIN1_PALETTE_BASE) /
					 sizeof(ls->win1.palette[0])] = value;
		} else if (offset > WIN0_PALETTE_BASE &&
			   offset < WIN0_PALETTE_END) {
			ls->win0.palette[(offset - WIN0_PALETTE_BASE) /
					 sizeof(ls->win0.palette[0])] = value;
		}
		break;
	}
}

static void s3c2416_lcd_reset(DeviceState *opaque)
{
	S3C2416LCDState *ls = S3C2416_LCD(opaque);

	ls->enabled = false;
	ls->invalidate = true;

	ls->win0.buf = &ls->win0_buf0;

	ls->win0.enabled = false;

	ls->win0_buf0.start = 0;
	ls->win0_buf0.end = 0;
	ls->win0_buf0.offsize = 0;
	ls->win0_buf0.page_width = 0;
	ls->win0_buf0.pixel_bits = 0;

	ls->win0_buf1.start = 0;
	ls->win0_buf1.end = 0;
	ls->win0_buf1.offsize = 0;
	ls->win0_buf1.page_width = 0;
	ls->win0_buf1.pixel_bits = 0;

	ls->win1.enabled = false;

	ls->xres = 0;
	ls->yres = 0;
}

static const GraphicHwOps s3c2416_lcd_graph_ops = {
    .invalidate = s3c2416_lcd_invalidate, .gfx_update = s3c2416_lcd_gfx_update,
};

static const MemoryRegionOps s3c2416_lcd_ops = {
    .read = s3c2416_lcd_read,
    .write = s3c2416_lcd_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property s3c2416_lcd_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void s3c2416_lcd_init(Object *obj)
{
	S3C2416LCDState *ls = S3C2416_LCD(obj);

	ls->panel_xres = 640;
	ls->panel_yres = 480;
}

static void s3c2416_lcd_realize(DeviceState *dev, Error **errp)
{
	S3C2416LCDState *ls = S3C2416_LCD(dev);

	memory_region_init_io(&ls->iomem, OBJECT(ls), &s3c2416_lcd_ops, ls,
			      TYPE_S3C2416_LCD, LCD_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ls), &ls->iomem);

	ls->console =
	    graphic_console_init(DEVICE(ls), 0, &s3c2416_lcd_graph_ops, ls);
	qemu_console_resize(ls->console, ls->panel_xres, ls->panel_yres);
}

static void s3c2416_lcd_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = s3c2416_lcd_realize;
	dc->reset = s3c2416_lcd_reset;
	dc->props = s3c2416_lcd_properties;
}

static const TypeInfo s3c2416_lcd_type_info = {
    .name = TYPE_S3C2416_LCD,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S3C2416LCDState),
    .instance_init = s3c2416_lcd_init,
    .class_init = s3c2416_lcd_class_init,
};

static void s3c2416_lcd_register_types(void)
{
	type_register_static(&s3c2416_lcd_type_info);
}

type_init(s3c2416_lcd_register_types)
