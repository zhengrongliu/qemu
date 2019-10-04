/*
 * at91sam9_pio.c
 *
 *  Created on: Jun 7, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "hw/gpio/at91sam9_pio.h"

#define PIO_PER 0x0
#define PIO_PDR 0x4
#define PIO_PSR 0x8
#define PIO_OER 0x10
#define PIO_ODR 0x14
#define PIO_OSR 0x18
#define PIO_IFER 0x20
#define PIO_IFDR 0x24
#define PIO_IFSR 0x28
#define PIO_SODR 0x30
#define PIO_CODR 0x34
#define PIO_ODSR 0x38
#define PIO_PDSR 0x3c
#define PIO_IER 0x40
#define PIO_IDR 0x44
#define PIO_IMR 0x48
#define PIO_ISR 0x4c
#define PIO_MDER 0x50
#define PIO_MDDR 0x54
#define PIO_MDSR 0x58
#define PIO_PUDR 0x60
#define PIO_PUER 0x64
#define PIO_PUSR 0x68
#define PIO_ABCDSR1 0x70
#define PIO_ABCDSR2 0x74
#define PIO_IFSCDR 0x80
#define PIO_IFSCER 0x84
#define PIO_IFSCSR 0x88
#define PIO_SCDR 0x8c
#define PIO_PPDDR 0x90
#define PIO_PPDER 0x94
#define PIO_PPDSR 0x98
#define PIO_OWER 0xa0
#define PIO_OWDR 0xa4
#define PIO_OWSR 0xa8
#define PIO_AIMER 0xb0
#define PIO_AIMDR 0xb4
#define PIO_AIMMR 0xb8
#define PIO_ESR 0xc0
#define PIO_LSR 0xc4
#define PIO_ELSR 0xc8
#define PIO_FELLSR 0xd0
#define PIO_REHLSR 0xd4
#define PIO_FRLHSR 0xd8
#define PIO_WPMR 0xe4
#define PIO_WPSR 0xe8
#define PIO_SCHMITT 0x100
#define PIO_DELAYR 0x110
#define PIO_DRIVER1 0x114
#define PIO_DIRVER2 0x118

#define PIO_REGION_SIZE 0x200

static uint64_t at91sam9_pio_read(void *opaque, hwaddr addr, unsigned size)
{
	return 0;
}

static void at91sam9_pio_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
}

static const MemoryRegionOps at91sam9_pio_ops = {
	.read = at91sam9_pio_read,
	.write = at91sam9_pio_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static void at91sam9_pio_init(Object *obj)
{

	At91sam9PioState *ps = AT91SAM9_PIO(obj);

	memory_region_init_io(&ps->iomem, OBJECT(ps), &at91sam9_pio_ops, ps,
			      TYPE_AT91SAM9_PIO, PIO_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ps), &ps->iomem);

}

static void at91sam9_pio_realize(DeviceState *dev, Error **errp)
{
	At91sam9PioState *ps = AT91SAM9_PIO(dev);

	sysbus_mmio_map(SYS_BUS_DEVICE(ps), 0, ps->addr);
}

static void at91sam9_pio_reset(DeviceState *dev) {}

static Property at91sam9_pio_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9PioState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void at91sam9_pio_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = at91sam9_pio_realize;
	dc->props = at91sam9_pio_properties;
	dc->reset = at91sam9_pio_reset;
}

static const TypeInfo at91sam9_pio_info = {
    .name = TYPE_AT91SAM9_PIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = at91sam9_pio_init,
    .instance_size = sizeof(At91sam9PioState),
    .class_init = at91sam9_pio_class_init,
};

static void at91sam9_pio_register_types(void)
{
	type_register_static(&at91sam9_pio_info);
}

type_init(at91sam9_pio_register_types)
