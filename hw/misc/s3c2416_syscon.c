#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "hw/misc/s3c2416_syscon.h"

#define SYSCON_REGION_SIZE 0x100

#define REG_IND(x) ((x) / 4)

#define LOCKCON0 0x0
#define LOCKCON1 0x4
#define OSCSET 0x8
#define MPLLCON 0x10
#define EPLLCON 0x18
#define EPLLCON_K 0x1c
#define CLKSRC 0x20
#define CLKDIV0 0x24
#define CLKDIV1 0x28
#define CLKDIV2 0x2c
#define HCLKCON 0x30
#define PCLKCON 0x34
#define SCLKCON 0x38
#define PWRMODE 0x40
#define SWRST 0x44
#define BUSPRI0 0x50
#define PWRCFG 0x60
#define RSTCON 0x64
#define RSTSTAT 0x68
#define WKUPSTAT 0x6c
#define INFORM0 0x70
#define INFORM1 0x74
#define INFORM2 0x78
#define INFORM3 0x7c
#define USB_PHYCTRL 0x80
#define USB_PHYPWR 0x84
#define USB_RSTCON 0x88
#define USB_CLKCON 0x8c

#define __MSK(w) ((1 << (w)) - 1)

#define CLKSRC_SELEXTCLK_WIDTH 0x1
#define CLKSRC_SELMPLL_WIDTH 0x1
#define CLKSRC_SELEPLL_WIDTH 0x1
#define CLKSRC_SELESRC_WIDTH 0x2
#define CLKSRC_SELHSMMC0_WIDTH 0x1
#define CLKSRC_SELHSMMC1_WIDTH 0x1
#define CLKSRC_SELHSSPI0_WIDTH 0x1

#define CLKSRC_SELEXTCLK_MSK __MSK(CLKSRC_SELEXTCLK_WIDTH)
#define CLKSRC_SELMPLL_MSK __MSK(CLKSRC_SELMPLL_WIDTH)
#define CLKSRC_SELEPLL_MSK __MSK(CLKSRC_SELEPLL_WIDTH)
#define CLKSRC_SELESRC_MSK __MSK(CLKSRC_SELESRC_WIDTH)
#define CLKSRC_SELHSMMC0_MSK __MSK(CLKSRC_SELHSMMC0_WIDTH)
#define CLKSRC_SELHSMMC1_MSK __MSK(CLKSRC_SELHSMMC1_WIDTH)
#define CLKSRC_SELHSSPI0_MSK __MSK(CLKSRC_SELHSSPI0_WIDTH)

#define CLKSRC_SELEXTCLK_SHIFT 3
#define CLKSRC_SELMPLL_SHIFT 4
#define CLKSRC_SELEPLL_SHIFT 6
#define CLKSRC_SELESRC_SHIFT 7
#define CLKSRC_SELHSMMC0_SHIFT 16
#define CLKSRC_SELHSMMC1_SHIFT 17
#define CLKSRC_SELHSSPI0_SHIFT 18

#define MPLLCON_SDIV_MSK 0x7
#define MPLLCON_PDIV_MSK 0x3f
#define MPLLCON_MDIV_MSK 0x3ff

#define MPLLCON_SDIV_SHIFT 0
#define MPLLCON_PDIV_SHIFT 5
#define MPLLCON_MDIV_SHIFT 14

#define EPLLCON_SDIV_MSK 0x7
#define EPLLCON_PDIV_MSK 0x3f
#define EPLLCON_MDIV_MSK 0xff

#define EPLLCON_SDIV_SHIFT 0
#define EPLLCON_PDIV_SHIFT 8
#define EPLLCON_MDIV_SHIFT 16

#define EPLLCON_KDIV_MSK 0xffff
#define EPLLCON_KDIV_SHIFT 0

#define CLKDIV0_DVS_WIDTH 0x1
#define CLKDIV0_ARMDIV_WIDTH 0x3
#define CLKDIV0_EXTDIV_WIDTH 0x3
#define CLKDIV0_PREDIV_WIDTH 0x2
#define CLKDIV0_HALFHCLK_WIDTH 0x1
#define CLKDIV0_PCLKDIV_WIDTH 0x1
#define CLKDIV0_HCLKDIV_WIDTH 0x2

#define CLKDIV0_DVS_MSK __MSK(CLKDIV0_DVS_WIDTH)
#define CLKDIV0_ARMDIV_MSK __MSK(CLKDIV0_ARMDIV_WIDTH)
#define CLKDIV0_EXTDIV_MSK __MSK(CLKDIV0_EXTDIV_WIDTH)
#define CLKDIV0_PREDIV_MSK __MSK(CLKDIV0_PREDIV_WIDTH)
#define CLKDIV0_HALFHCLK_MSK __MSK(CLKDIV0_HALFHCLK_WIDTH)
#define CLKDIV0_PCLKDIV_MSK __MSK(CLKDIV0_PCLKDIV_WIDTH)
#define CLKDIV0_HCLKDIV_MSK __MSK(CLKDIV0_HCLKDIV_WIDTH)

#define CLKDIV0_DVS_SHIFT 13
#define CLKDIV0_ARMDIV_SHIFT 9
#define CLKDIV0_EXTDIV_SHIFT 6
#define CLKDIV0_PREDIV_SHIFT 4
#define CLKDIV0_HALFHCLK_SHIFT 3
#define CLKDIV0_PCLKDIV_SHIFT 2
#define CLKDIV0_HCLKDIV_SHIFT 0

#define CLKDIV1_SPIDIV0_WIDTH 0x2
#define CLKDIV1_DISPDIV_WIDTH 0x8
#define CLKDIV1_I2SDIV0_WIDTH 0x4
#define CLKDIV1_UARTDIV_WIDTH 0x4
#define CLKDIV1_HSMMCDIV1_WIDTH 0x2
#define CLKDIV1_USBHOSTDIV_WIDTH 0x2

#define CLKDIV1_SPIDIV0_MSK __MSK(CLKDIV1_SPIDIV0_WIDTH)
#define CLKDIV1_DISPDIV_MSK __MSK(CLKDIV1_DISPDIV_WIDTH)
#define CLKDIV1_I2SDIV0_MSK __MSK(CLKDIV1_I2SDIV0_WIDTH)
#define CLKDIV1_UARTDIV_MSK __MSK(CLKDIV1_UARTDIV_WIDTH)
#define CLKDIV1_HSMMCDIV1_MSK __MSK(CLKDIV1_HSMMCDIV1_WIDTH)
#define CLKDIV1_USBHOSTDIV_MSK __MSK(CLKDIV1_USBHOSTDIV_WIDTH)

#define CLKDIV1_SPIDIV0_SHIFT 24
#define CLKDIV1_DISPDIV_SHIFT 16
#define CLKDIV1_I2SDIV0_SHIFT 12
#define CLKDIV1_UARTDIV_SHIFT 8
#define CLKDIV1_HSMMCDIV1_SHIFT 6
#define CLKDIV1_USBHOSTDIV_SHIFT 4

#define PCLKCON_PCM_SHIFT 19
#define PCLKCON_GPIO_SHIFT 13
#define PCLKCON_RTC_SHIFT 12
#define PCLKCON_WDT_SHIFT 11
#define PCLKCON_PWM_SHIFT 10
#define PCLKCON_I2S0_SHIFT 9
#define PCLKCON_AC97_SHIFT 8
#define PCLKCON_TSADC _SHIFT 7
#define PCLKCON_SPI_HS0_SHIFT 6
#define PCLKCON_I2C_SHIFT 4
#define PCLKCON_UART3_SHIFT 3
#define PCLKCON_UART2_SHIFT 2
#define PCLKCON_UART1_SHIFT 1
#define PCLKCON_UART0_SHIFT 0

#define SCLKCON_UART_SHIFT 8

static void s3c2416_syscon_update_clk(S3C2416SysconState *ss)
{
	uint64_t mrefclk = ss->clock_freq;
	uint64_t mrefclk_div;
	uint64_t mpll;
	uint64_t mnopll;
	uint64_t tmp;
	uint64_t msysclk;
	uint64_t hclk;
	uint64_t pclk;

	/*
	 * mpll = mrefclk*mdiv/(pdiv * 1<<sdiv)
	 */
	tmp = mrefclk * ((ss->regs[REG_IND(MPLLCON)] >> MPLLCON_MDIV_SHIFT) &
			 MPLLCON_MDIV_MSK);
	mpll =
	    tmp / (((ss->regs[REG_IND(MPLLCON)] >> MPLLCON_PDIV_SHIFT) &
		    MPLLCON_PDIV_MSK) *
		   (1 << ((ss->regs[REG_IND(MPLLCON)] >> MPLLCON_SDIV_SHIFT) &
			  MPLLCON_SDIV_MSK)));

	/*
	 * mrefclk_div = mrefclk/(2*div+1)
	 */
	tmp = 2 * ((ss->regs[REG_IND(CLKDIV0)] >> CLKDIV0_EXTDIV_SHIFT) &
		   CLKDIV0_EXTDIV_MSK) +
	      1;
	mrefclk_div = mrefclk / tmp;

	/*
	 * mnopll
	 */
	if (ss->regs[REG_IND(CLKSRC)] & (1 << CLKSRC_SELEXTCLK_SHIFT)) {
		mnopll = mrefclk_div;
	} else {
		mnopll = mrefclk;
	}

	/*
	 * msysclk
	 */
	if (ss->regs[REG_IND(CLKSRC)] & (1 << CLKSRC_SELMPLL_SHIFT)) {
		msysclk = mpll;
	} else {
		msysclk = mnopll;
	}

	/*
	 * hclk
	 */
	tmp = ((ss->regs[REG_IND(CLKDIV0)] >> CLKDIV0_PREDIV_SHIFT) &
	       CLKDIV0_PREDIV_MSK) +
	      1;
	tmp *= ((ss->regs[REG_IND(CLKDIV0)] >> CLKDIV0_HCLKDIV_SHIFT) &
		CLKDIV0_HCLKDIV_MSK) +
	       1;
	hclk = msysclk / tmp;

	/*
	 * pclk
	 */
	tmp = ((ss->regs[REG_IND(CLKDIV0)] >> CLKDIV0_PCLKDIV_SHIFT) &
	       CLKDIV0_PCLKDIV_MSK) +
	      1;
	pclk = hclk / tmp;

	object_property_set_int(OBJECT(ss->timer_device), pclk, "clock-freq",
				&error_abort);
}

static uint64_t s3c2416_syscon_read(void *opaque, hwaddr addr, unsigned size)
{
	S3C2416SysconState *ss = S3C2416_SYSCON(opaque);

	return ss->regs[REG_IND(addr)];
}

static void s3c2416_syscon_write(void *opaque, hwaddr addr, uint64_t data,
				 unsigned size)
{
	S3C2416SysconState *ss = S3C2416_SYSCON(opaque);
	bool update_clk;

	switch (addr) {
	case MPLLCON:
	case EPLLCON:
	case EPLLCON_K:
	case CLKSRC:
	case CLKDIV0:
	case CLKDIV1:
	case CLKDIV2:
		if (data != ss->regs[REG_IND(addr)]) {
			update_clk = true;
		}
		break;
	case HCLKCON:
		break;
	case PCLKCON:
		if (data & (1 << PCLKCON_PWM_SHIFT)) {
			object_property_set_bool(OBJECT(ss->timer_device), true,
						 "enabled", &error_abort);
		} else {
			object_property_set_bool(OBJECT(ss->timer_device),
						 false, "enabled",
						 &error_abort);
		}
		break;
	case SCLKCON:
		break;
	}

	ss->regs[REG_IND(addr)] = data;

	if (update_clk) {
		s3c2416_syscon_update_clk(ss);
	}
}

static const MemoryRegionOps s3c2416_syscon_ops = {
    .read = s3c2416_syscon_read,
    .write = s3c2416_syscon_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void s3c2416_syscon_init(Object *obj) {}

static void s3c2416_syscon_realize(DeviceState *dev, Error **errp)
{
	S3C2416SysconState *ss = S3C2416_SYSCON(dev);

	memory_region_init_io(&ss->iomem, OBJECT(ss), &s3c2416_syscon_ops, ss,
			      TYPE_S3C2416_SYSCON, SYSCON_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ss), &ss->iomem);
}

static void s3c2416_syscon_reset(DeviceState *dev)
{
	S3C2416SysconState *ss = S3C2416_SYSCON(dev);

	/*
	 * initial value
	 */
	ss->regs[REG_IND(LOCKCON0)] = 0x0000ffff;
	ss->regs[REG_IND(LOCKCON1)] = 0x0000ffff;
	ss->regs[REG_IND(OSCSET)] = 0x00008000;
	ss->regs[REG_IND(MPLLCON)] = 0x018540c0;
	ss->regs[REG_IND(EPLLCON)] = 0x01200102;
	ss->regs[REG_IND(EPLLCON_K)] = 0x00000000;
	ss->regs[REG_IND(CLKSRC)] = 0x00000000;
	ss->regs[REG_IND(CLKDIV0)] = 0x0000000C;
	ss->regs[REG_IND(CLKDIV1)] = 0x00000000;
	ss->regs[REG_IND(CLKDIV2)] = 0x00000000;
	ss->regs[REG_IND(HCLKCON)] = 0xffffffff;
	ss->regs[REG_IND(PCLKCON)] = 0xffffffff;
	ss->regs[REG_IND(SCLKCON)] = 0xffffdfff;

	/*
	 * for debug
	 */
	ss->regs[REG_IND(MPLLCON)] = (400 << MPLLCON_MDIV_SHIFT) |
				     (3 << MPLLCON_PDIV_SHIFT) |
				     (2 << MPLLCON_SDIV_SHIFT);
	ss->regs[REG_IND(EPLLCON)] = (32 << EPLLCON_MDIV_SHIFT) |
				     (1 << EPLLCON_PDIV_SHIFT) |
				     (3 << EPLLCON_SDIV_SHIFT);
	ss->regs[REG_IND(EPLLCON_K)] = 0;
	ss->regs[REG_IND(CLKSRC)] |= (1 << CLKSRC_SELMPLL_SHIFT);
	ss->regs[REG_IND(CLKDIV0)] |= (2 << CLKDIV0_PREDIV_SHIFT) |
				      (0 << CLKDIV0_HCLKDIV_SHIFT) |
				      (1 << CLKDIV0_PCLKDIV_SHIFT);
	ss->regs[REG_IND(PCLKCON)] = 0x0;

	s3c2416_syscon_update_clk(ss);
}

static Property s3c2416_syscon_properties[] = {
    DEFINE_PROP_UINT64("clock-freq", S3C2416SysconState, clock_freq, 12000000),
    DEFINE_PROP_UINT64("crystal-freq", S3C2416SysconState, crystal_freq,
		       12000000),
    DEFINE_PROP_UINT64("osc-freq", S3C2416SysconState, osc_freq, 12000000),
    DEFINE_PROP_PTR("timer-device", S3C2416SysconState, timer_device),
    DEFINE_PROP_END_OF_LIST(),
};

static void s3c2416_syscon_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = s3c2416_syscon_realize;
	dc->props = s3c2416_syscon_properties;
	dc->reset = s3c2416_syscon_reset;
}

static const TypeInfo s3c2416_syscon_info = {
    .name = TYPE_S3C2416_SYSCON,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = s3c2416_syscon_init,
    .instance_size = sizeof(S3C2416SysconState),
    .class_init = s3c2416_syscon_class_init,
};

static void s3c2416_syscon_register_types(void)
{
	type_register_static(&s3c2416_syscon_info);
}

type_init(s3c2416_syscon_register_types)
