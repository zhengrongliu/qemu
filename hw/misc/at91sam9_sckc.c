#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "hw/misc/at91sam9_sckc.h"

#define SCKC_REGION_SIZE 0x4

#define SCKC_CR 0x0

#define SCKC_CR_OSCSEL_SHIFT 3
#define SCKC_CR_OSC32BYP_SHIFT 2
#define SCKC_CR_OSC32EN_SHIFT 1
#define SCKC_CR_RCEN_SHIFT 0

#define SCKC_CR_OSCSEL_MASK 1
#define SCKC_CR_OSCSEL_XTAL 1
#define SCKC_CR_OSCSEL_RC 0

#define SCKC_CR_OSC32EN_MASK 1
#define SCKC_CR_OSC32EN_ENABLE 1
#define SCKC_CR_OSC32EN_DISABLE 0

#define SCKC_CR_RCEN_MASK 1
#define SCKC_CR_RCEN_ENABLE 1
#define SCKC_CR_RCEN_DISABLE 0

#define SCKC_CR_OSC32BYP_MASK 1
#define SCKC_CR_OSC32BYP_ENABLE 1
#define SCKC_CR_OSC32BYP_DISABLE 0

static uint64_t at91sam9_sckc_read(void *opaque, hwaddr addr, unsigned size)
{
	At91sam9SckcState *ss = AT91SAM9_SCKC(opaque);

	switch (addr) {
	case SCKC_CR:
		return ss->reg_sckc_cr;
	}
	return 0;
}

static void at91sam9_sckc_write(void *opaque, hwaddr addr, uint64_t data,
				unsigned size)
{
	At91sam9SckcState *ss = AT91SAM9_SCKC(opaque);

	switch (addr) {
	case SCKC_CR:
		ss->reg_sckc_cr = data;
		break;
	}
}

static const MemoryRegionOps at91sam9_sckc_ops = {
    .read = at91sam9_sckc_read,
    .write = at91sam9_sckc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void at91sam9_sckc_init(Object *obj)
{

	At91sam9SckcState *ps = AT91SAM9_SCKC(obj);

	memory_region_init_io(&ps->iomem, OBJECT(ps), &at91sam9_sckc_ops, ps,
			      TYPE_AT91SAM9_SCKC, SCKC_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ps), &ps->iomem);
}

static void at91sam9_sckc_realize(DeviceState *dev, Error **errp)
{
	At91sam9SckcState *ps = AT91SAM9_SCKC(dev);

	sysbus_mmio_map(SYS_BUS_DEVICE(ps), 0, ps->addr);
}

static void at91sam9_sckc_reset(DeviceState *dev)
{
	At91sam9SckcState *ps = AT91SAM9_SCKC(dev);

	ps->reg_sckc_cr = 0x00000001;
}

static Property at91sam9_sckc_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9SckcState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void at91sam9_sckc_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = at91sam9_sckc_realize;
	dc->props = at91sam9_sckc_properties;
	dc->reset = at91sam9_sckc_reset;
}

static const TypeInfo at91sam9_sckc_info = {
    .name = TYPE_AT91SAM9_SCKC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = at91sam9_sckc_init,
    .instance_size = sizeof(At91sam9SckcState),
    .class_init = at91sam9_sckc_class_init,
};

static void at91sam9_sckc_register_types(void)
{
	type_register_static(&at91sam9_sckc_info);
}

type_init(at91sam9_sckc_register_types)
