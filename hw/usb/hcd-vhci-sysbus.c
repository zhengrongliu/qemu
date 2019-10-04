/*
 * hcd-vhci-sysbus.c
 *
 *  Created on: Nov 24, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "exec/address-spaces.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "qapi/qmp/qerror.h"
#include "qapi/error.h"

#include "hcd-vhci.h"



static uint64_t vhci_sysbus_read(void *opaque, hwaddr offset, unsigned size)
{
	VhciSysbusState * vs = VHCI_SYSBUS(opaque);
	VhciState *vhci = &vs->vhci;

	return vhci_io_read(vhci,offset,size);
}

static void vhci_sysbus_write(void *opaque, hwaddr offset, uint64_t value,
	unsigned size)
{
	VhciSysbusState * vs = VHCI_SYSBUS(opaque);
	VhciState *vhci = &vs->vhci;

	vhci_io_write(vhci,offset,value,size);

}


static const MemoryRegionOps vhci_sysbus_ops = {
	.read = vhci_sysbus_read,
	.write = vhci_sysbus_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static Property vhci_sysbus_properties[] = {
	DEFINE_PROP_STRING("mode",VhciPCIState,vhci.mode),
	DEFINE_PROP_END_OF_LIST(),
};

static void vhci_sysbus_reset(DeviceState *opaque)
{

}


static void vhci_sysbus_realize(DeviceState *dev, Error **errp)
{
	VhciSysbusState * vs = VHCI_SYSBUS(dev);
	VhciState *vhci = &vs->vhci;




	sysbus_init_irq(SYS_BUS_DEVICE(vs),&(vhci->irq));

	memory_region_init_io(&vhci->iomem,OBJECT(vs),&vhci_sysbus_ops,vs,
		TYPE_VHCI_SYSBUS,VHCI_IO_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(vs),&vhci->iomem);

	vhci->as = &address_space_memory;

	/*
	 * the default mode is "qemu"
	 */
	vhci_qemu_init(vhci, DEVICE(dev), errp);

}

static void vhci_sysbus_class_init(ObjectClass *c ,void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = vhci_sysbus_realize;
	dc->reset = vhci_sysbus_reset;
	dc->props = vhci_sysbus_properties;
}

static const TypeInfo vhci_sysbus_type_info = {
	.name = TYPE_VHCI_SYSBUS,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(VhciSysbusState),
	.class_init = vhci_sysbus_class_init,
};


static void vhci_sysbus_register_types(void)
{
	type_register_static(&vhci_sysbus_type_info);
}

type_init(vhci_sysbus_register_types)


