#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "hw/misc/at91sam9_pmc.h"

#define PMC_REGION_SIZE 0x200

#define PMC_SCER 0x00
#define PMC_SCDR 0x04
#define PMC_SCSR 0x08
#define PMC_PCER 0x10
#define PMC_PCDR 0x14
#define PMC_PCSR 0x18
#define CKGR_UCKR 0x1c
#define CKGR_MOR 0x20
#define CKGR_MCFR 0x24
#define CKGR_PLLAR 0x28
#define PMC_MCKR 0x30
#define PMC_USB 0x38
#define PMC_SMD 0x3c
#define PMC_PCK0 0x40
#define PMC_PCK1 0x44
#define PMC_IER 0x60
#define PMC_IDR 0x64
#define PMC_SR 0x68
#define PMC_IMR 0x6c
#define PMC_PLLICPR 0x80
#define PMC_WPMR 0xe4
#define PMC_WPSR 0xe8
#define PMC_PCR 0x10c

/*
 * Main oscillator register
 */
#define CKGR_MOR_MOSCXTEN_SHIFT 0
#define CKGR_MOR_MOSCXTBY_SHIFT 1
#define CKGR_MOR_MOSCRCEN_SHIFT 3
#define CKGR_MOR_MUSTZERO_SHIFT 4
#define CKGR_MOR_MOSCXTST_SHIFT 8
#define CKGR_MOR_KEY_SHIFT 16
#define CKGR_MOR_MOSCSEL_SHIFT 24
#define CKGR_MOR_CFDEN_SHIFT 25

#define CKGR_MOR_MOSCXTEN_MASK 1
#define CKGR_MOR_MOSCXTEN_ENABLE 1
#define CKGR_MOR_MOSCXTEN_DISABLE 0
#define CKGR_MOR_MOSCXTBY_MASK 1
#define CKGR_MOR_MOSCXTBY_ENABLE 1
#define CKGR_MOR_MOSCXTBY_DISABLE 0
#define CKGR_MOR_MOSCRCEN_MASK 1
#define CKGR_MOR_MOSCRCEN_ENABLE 1
#define CKGR_MOR_MOSCRCEN_DISABLE 0
#define CKGR_MOR_MUSTZERO_MASK 0x7
#define CKGR_MOR_MUSTZERO_VALID 0x0
#define CKGR_MOR_MOSCXTST_MASK 0xff
#define CKGR_MOR_MOSCXTST_MAX 0xff
#define CKGR_MOR_KEY_MASK 0xff
#define CKGR_MOR_KEY_PASSWD (0x37)
#define CKGR_MOR_MOSCSEL_MASK 1
#define CKGR_MOR_MOSCSEL_XTAL 1
#define CKGR_MOR_MOSCSEL_RC 0
#define CKGR_MOR_CFDEN_MASK 1
#define CKGR_MOR_CFDEN_ENABLE 1
#define CKGR_MOR_CFDEN_DISABLE 0

/*
 * Clock Generator PLLA register
 */
#define CKGR_PLLAR_DIVA_SHIFT 0
#define CKGR_PLLAR_DIVA_MASK 0xff
#define CKGR_PLLAR_DIVA_MAX 0xff
#define CKGR_PLLAR_DIVA_MIN 0x1
#define CKGR_PLLAR_PLLACOUNT_SHIFT 8
#define CKGR_PLLAR_PLLACOUNT_MASK 0x3f
#define CKGR_PLLAR_PLLACOUNT_MAX 0x3f
#define CKGR_PLLAR_OUTA_SHIFT 14
#define CKGR_PLLAR_OUTA_MASK 0x3
#define CKGR_PLLAR_MULA_SHIFT 16
#define CKGR_PLLAR_MULA_MASK 0xff
#define CKGR_PLLAR_MULA_MAX 0xff
#define CKGR_PLLAR_MULA_MIN 0x1
#define CKGR_PLLAR_ONE_SHIFT 29
#define CKGR_PLLAR_ONE_MASK 1
#define CKGR_PLLAR_ONE_VALID 1

/*
 * PLL charge Pump Current register
 */
#define PMC_PLLICPR_ICPLLA_SHIFT 0
#define PMC_PLLICPR_ICPLLA_MASK 1

/*
 * UTMI clock configuration register
 */
#define CKGR_UCKR_UPLLEN_SHIFT 16
#define CKGR_UCKR_UPLLCOUNT_SHIFT 20
#define CKGR_UCKR_BIASEN_SHIFT 24
#define CKGR_UCKR_BIASCOUNT_SHIFT 28

#define CKGR_UCKR_UPLLEN_MASK 1
#define CKGR_UCKR_UPLLEN_ENABLE 1
#define CKGR_UCKR_UPLLEN_DISABLE 0
#define CKGR_UCKR_UPLLCOUNT_MASK 0xf
#define CKGR_UCKR_UPLLCOUNT_MAX 0xf
#define CKGR_UCKR_BIASCOUNT_MASK 0xf
#define CKGR_UCKR_BIASCOUNT_MAX 0xf

/*
 * Clock Generator Main Clock Frequency Register
 */
#define CKGR_MCFR_MAINFRDY_SHIFT 16
#define CKGR_MCFR_MAINFRDY_MASK 1
#define CKGR_MCFR_MAINFRDY_READY 1
#define CKGR_MCFR_MAINFRDY_NOTRDY 0
#define CKGR_MCFR_MAINF_SHIFT 0
#define CKGR_MCFR_MAINF_MASK 0xffff

/*
 * PMC Master Clock register
 */
#define PMC_MCKR_CSS_SHIFT 0
#define PMC_MCKR_PRES_SHIFT 4
#define PMC_MCKR_MDIV_SHIFT 8
#define PMC_MCKR_PLLADIV2_SHIFT 12

#define PMC_MCKR_CSS_MASK 0x3
#define PMC_MCKR_PRES_MASK 0x7
#define PMC_MCKR_MDIV_MASK 0x3
#define PMC_MCKR_PLLADIV2_MAKS 0x1
#define PMC_MCKR_PLLADIV2_WIDTH 0x1
/*
 * PMC Status register
 */
#define PMC_SR_MOSCXTS (1 << 0)
#define PMC_SR_LOCKA (1 << 1)
#define PMC_SR_MCKRDY (1 << 3)
#define PMC_SR_LOCKU (1 << 6)
#define PMC_SR_OSCSELS (1 << 7)
#define PMC_SR_PCKRDY0 (1 << 8)
#define PMC_SR_PCKRDY1 (1 << 9)
#define PMC_SR_MOSCSELS (1 << 16)
#define PMC_SR_MOSCRCS (1 << 17)
#define PMC_SR_CFDEV (1 << 18)
#define PMC_SR_CFDS (1 << 19)
#define PMC_SR_FOS (1 << 20)

#define __FIELD_MASK(regname, fieldname)                                       \
	(regname##_##fieldname##_MASK << regname##_##fieldname##_SHIFT)

#define __FIELD_VALUE_FIX(regname, fieldname, valname)                         \
	(regname##_##fieldname##_##valname << regname##_##fieldname##_SHIFT)

#define __FIELD_IS_SET(regname, fieldname, valname, value)                     \
	(((value) &                                                            \
	  (regname##_##fieldname##_MASK << regname##_##fieldname##_SHIFT)) ==  \
	 (regname##_##fieldname##_##valname << regname##_##fieldname##_SHIFT))

#define __FIELD_VALUE_SET(regname, fieldname, value)                           \
	(((value)&regname##_##fieldname##_MASK)                                \
	 << regname##_##fieldname##_SHIFT)

#define __FIELD_VALUE_GET(regname, fieldname, value)                           \
	(((value) >> regname##_##fieldname##_SHIFT) &                          \
	 regname##_##fieldname##_MASK)

static void at91sam9_pmc_CKGR_PLLAR_write(At91sam9PmcState *ps, uint32_t val)
{
	if (!__FIELD_IS_SET(CKGR_PLLAR, ONE, VALID, val)) {
		fprintf(stderr, "reg CKGR_PLLAR field %d %x must be one\n",
			CKGR_PLLAR_ONE_SHIFT, CKGR_PLLAR_ONE_MASK);
		return;
	}

	if (__FIELD_VALUE_GET(CKGR_PLLAR, MULA, val) != 0) {
		ps->reg_pmc_sr |= PMC_SR_LOCKA;
	} else {
		ps->reg_pmc_sr &= ~PMC_SR_LOCKA;
	}
}

static void at91sam9_pmc_CKGR_MOR_write(At91sam9PmcState *ps, uint32_t val)
{
	if (!__FIELD_IS_SET(CKGR_MOR, KEY, PASSWD, val)) {
		fprintf(stderr, "reg CKGR_MOR write passwd not valid\n");
		return;
	}
	if (!__FIELD_IS_SET(CKGR_MOR, MUSTZERO, VALID, val)) {
		fprintf(stderr, "reg CKGR_MOR field %d %x must be zero\n",
			CKGR_MOR_MUSTZERO_SHIFT, CKGR_MOR_MUSTZERO_MASK);
		return;
	}

	val &=
	    ~(__FIELD_MASK(CKGR_MOR, KEY) | __FIELD_MASK(CKGR_MOR, MOSCXTST));
	val = (val ^ ps->reg_ckgr_mor) & val;

	if (__FIELD_IS_SET(CKGR_MOR, MOSCXTEN, ENABLE, val)) {
		ps->reg_pmc_sr |= PMC_SR_MOSCXTS;
	} else if (__FIELD_IS_SET(CKGR_MOR, MOSCXTEN, DISABLE, val)) {
		ps->reg_pmc_sr &= ~PMC_SR_MOSCXTS;
	}

	if (__FIELD_IS_SET(CKGR_MOR, MOSCRCEN, ENABLE, val)) {
		ps->reg_pmc_sr |= PMC_SR_MOSCRCS;
	} else if (__FIELD_IS_SET(CKGR_MOR, MOSCRCEN, DISABLE, val)) {
		ps->reg_pmc_sr &= ~PMC_SR_MOSCRCS;
	}

	if (__FIELD_IS_SET(CKGR_MOR, MOSCSEL, RC, val) ||
	    __FIELD_IS_SET(CKGR_MOR, MOSCSEL, XTAL, val)) {
		ps->reg_pmc_sr |= PMC_SR_MOSCSELS;
	}
}

static uint64_t at91sam9_pmc_read(void *opaque, hwaddr addr, unsigned size)
{
	At91sam9PmcState *ps = AT91SAM9_PMC(opaque);
	// fprintf(stderr,"pmc_read %lx\n",addr);
	switch (addr) {
	case PMC_SR:
		return ps->reg_pmc_sr;
	case PMC_SCSR:
		break;
	case PMC_PCER:
		break;
	case CKGR_UCKR:
		return ps->reg_ckgr_uckr;
	case CKGR_MOR:
		return ps->reg_ckgr_mor;
	case CKGR_MCFR:
		return ps->reg_ckgr_mcfr;
	case CKGR_PLLAR:
		return ps->reg_ckgr_pllar;
	case PMC_MCKR:
		return ps->reg_pmc_mckr;
	case PMC_USB:
		break;
	case PMC_SMD:
		break;
	case PMC_PLLICPR:
		return ps->reg_pmc_pllicpr;
	}
	return 0;
}

static void at91sam9_pmc_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	At91sam9PmcState *ps = AT91SAM9_PMC(opaque);
	uint32_t val = data;
	// fprintf(stderr,"pmc_write %lx %lx\n",addr,data);
	switch (addr) {
	case CKGR_MOR:
		at91sam9_pmc_CKGR_MOR_write(ps, val);
		ps->reg_ckgr_mor = val;
		break;
	case CKGR_PLLAR:
		at91sam9_pmc_CKGR_PLLAR_write(ps, val);
		ps->reg_ckgr_pllar = val;
		break;
	case CKGR_UCKR:
		ps->reg_ckgr_uckr = val;
		break;
	case PMC_MCKR:
		ps->reg_pmc_mckr = val;
		ps->reg_pmc_sr |= PMC_SR_MCKRDY;
		break;
	case PMC_SCER:
		break;
	case PMC_SCDR:
		break;
	case PMC_PCER:
		break;
	case PMC_PCDR:
		break;
	case PMC_PCSR:
		break;
	case PMC_PLLICPR:
		ps->reg_pmc_pllicpr = val;
		break;
	}
}

static const MemoryRegionOps at91sam9_pmc_ops = {
    .read = at91sam9_pmc_read,
    .write = at91sam9_pmc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void at91sam9_pmc_init(Object *obj)
{
	At91sam9PmcState *ps = AT91SAM9_PMC(obj);

	memory_region_init_io(&ps->iomem, OBJECT(ps), &at91sam9_pmc_ops, ps,
			      TYPE_AT91SAM9_PMC, PMC_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ps), &ps->iomem);
}

static void at91sam9_pmc_realize(DeviceState *dev, Error **errp)
{
	At91sam9PmcState *ps = AT91SAM9_PMC(dev);

	sysbus_mmio_map(SYS_BUS_DEVICE(ps), 0, ps->addr);
}

static void at91sam9_pmc_reset(DeviceState *dev)
{
	At91sam9PmcState *ps = AT91SAM9_PMC(dev);

	ps->reg_ckgr_pllar = __FIELD_VALUE_SET(CKGR_PLLAR, MULA, 99) |
			     __FIELD_VALUE_SET(CKGR_PLLAR, DIVA, 3);
	ps->reg_pmc_mckr = __FIELD_VALUE_SET(PMC_MCKR, CSS, 2);
}

static Property at91sam9_pmc_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9PmcState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void at91sam9_pmc_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = at91sam9_pmc_realize;
	dc->props = at91sam9_pmc_properties;
	dc->reset = at91sam9_pmc_reset;
}

static const TypeInfo at91sam9_pmc_info = {
    .name = TYPE_AT91SAM9_PMC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = at91sam9_pmc_init,
    .instance_size = sizeof(At91sam9PmcState),
    .class_init = at91sam9_pmc_class_init,
};

static void at91sam9_pmc_register_types(void)
{
	type_register_static(&at91sam9_pmc_info);
}

type_init(at91sam9_pmc_register_types)
