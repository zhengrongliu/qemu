/*
 * am335x_cm.c
 *
 *  Created on: Mar 16, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"
#include "qapi/error.h"


#define TYPE_AM335X_CM "am335x-cm"
#define AM335X_CM(obj)                                                      \
	OBJECT_CHECK(Am335xCmState, (obj), TYPE_AM335X_CM)

#define CM_REGION_SIZE 0x2000

#define _REG(x)	((x)/4)

typedef struct Am335xCmState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;
	uint32_t addr;
	uint32_t regs[CM_REGION_SIZE/4];
} Am335xCmState;






static uint64_t am335x_cm_read(void *opaque, hwaddr addr, unsigned size)
{
	Am335xCmState *ps = AM335X_CM(opaque);

	return ps->regs[_REG(addr)];
}

static void am335x_cm_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	Am335xCmState *ps = AM335X_CM(opaque);

	ps->regs[_REG(addr)] = data;

}

static const MemoryRegionOps am335x_cm_ops = {
	.read = am335x_cm_read,
	.write = am335x_cm_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static void am335x_cm_init(Object *obj)
{
	Am335xCmState *ps = AM335X_CM(obj);

	memory_region_init_io(&ps->iomem, OBJECT(ps), &am335x_cm_ops, ps,
			      TYPE_AM335X_CM, CM_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ps), &ps->iomem);
}

static void am335x_cm_realize(DeviceState *dev, Error **errp)
{
	Am335xCmState *ps = AM335X_CM(dev);

	sysbus_mmio_map(SYS_BUS_DEVICE(ps), 0, ps->addr);
}

static void am335x_cm_reset(DeviceState *dev)
{
}

static Property am335x_cm_properties[] = {
	DEFINE_PROP_UINT32("addr",Am335xCmState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void am335x_cm_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = am335x_cm_realize;
	dc->props = am335x_cm_properties;
	dc->reset = am335x_cm_reset;
}

static const TypeInfo am335x_cm_info = {
    .name = TYPE_AM335X_CM,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = am335x_cm_init,
    .instance_size = sizeof(Am335xCmState),
    .class_init = am335x_cm_class_init,
};

static void am335x_cm_register_types(void)
{
	type_register_static(&am335x_cm_info);
}

type_init(am335x_cm_register_types)


