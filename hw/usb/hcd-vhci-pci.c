/*
 * hcd-vhci-pci.c
 *
 *  Created on: Dec 14, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "exec/address-spaces.h"
#include "hw/pci/pci.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "qapi/qmp/qerror.h"
#include "qapi/error.h"

#include "hcd-vhci.h"

#define PCI_VENDOR_ID_VIRT	0x0623
#define PCI_DEVICE_ID_VIRT_VHCI	0x0528

static uint64_t vhci_pci_read(void *opaque, hwaddr offset, unsigned size)
{
	VhciPCIState * vs = VHCI_PCI(opaque);
	VhciState *vhci = &vs->vhci;

	return vhci_io_read(vhci,offset,size);
}

static void vhci_pci_write(void *opaque, hwaddr offset, uint64_t value,
	unsigned size)
{
	VhciPCIState * vs = VHCI_PCI(opaque);
	VhciState *vhci = &vs->vhci;

	vhci_io_write(vhci,offset,value,size);

}


static const MemoryRegionOps vhci_pci_ops = {
	.read = vhci_pci_read,
	.write = vhci_pci_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static Property vhci_pci_properties[] = {
	DEFINE_PROP_STRING("mode",VhciPCIState,vhci.mode),
	DEFINE_PROP_END_OF_LIST(),
};

static void vhci_pci_reset(DeviceState *opaque)
{

}


static void vhci_pci_realize(PCIDevice *dev, Error **errp)
{
	VhciPCIState * vs = VHCI_PCI(dev);
	VhciState *vhci = &vs->vhci;
	uint8_t *pci_conf = dev->config;
#if 0
	if(vhci->mode != NULL &&
			strcmp(vhci->mode,"qemu") != 0 &&
			strcmp(vhci->mode,"usbip") != 0){
		error_setg(errp, QERR_INVALID_PARAMETER_VALUE, "mode",
                       "vhci mode");
		return ;
	}

	if(vhci->mode != NULL && strcmp(vhci->mode,"usbip") == 0 &&
			!qemu_chr_fe_backend_connected(&vhci->cs)){
		error_setg(errp, QERR_MISSING_PARAMETER, "chardev");
		return ;
	}
#endif
	pci_set_byte(&pci_conf[PCI_CLASS_PROG], 0x70);
	pci_set_byte(&pci_conf[PCI_CAPABILITY_LIST], 0x00);
	pci_set_byte(&pci_conf[PCI_INTERRUPT_PIN], 1); /* interrupt pin A */

	memory_region_init_io(&vhci->iomem, OBJECT(dev), &vhci_pci_ops, vs,
                          TYPE_VHCI_PCI, VHCI_IO_REGION_SIZE);

	vhci->irq = pci_allocate_irq(dev);
	vhci->as = pci_get_address_space(dev);

	/*
	 * the default mode is "qemu"
	 */
	vhci_qemu_init(vhci, DEVICE(dev), errp);

	pci_register_bar(dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &vhci->iomem);

}

static void vhci_pci_class_init(ObjectClass *c ,void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(c);

	k->vendor_id = PCI_VENDOR_ID_VIRT;
	k->device_id = PCI_DEVICE_ID_VIRT_VHCI;
	k->class_id = PCI_CLASS_SERIAL_USB;
	k->realize = vhci_pci_realize;
	k->revision = 0;

	dc->reset = vhci_pci_reset;
	dc->props = vhci_pci_properties;
}

static const TypeInfo vhci_pci_type_info = {
	.name = TYPE_VHCI_PCI,
	.parent = TYPE_PCI_DEVICE,
	.instance_size = sizeof(VhciPCIState),
	.class_init = vhci_pci_class_init,
	.interfaces = (InterfaceInfo[]) {
		{ INTERFACE_CONVENTIONAL_PCI_DEVICE },
		{ },
	},
};


static void vhci_pci_register_types(void)
{
	type_register_static(&vhci_pci_type_info);
}

type_init(vhci_pci_register_types)




