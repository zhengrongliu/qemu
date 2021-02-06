/*
 * virtio-snd.c
 *
 *  Created on: Nov 8, 2020
 *      Author: Zhengrong.liu
 */
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "ui/console.h"
#include "ui/qemu-spice.h"
#include "ui/spice-display.h"
#include "qemu/iov.h"
#include "hw/pci/pci.h"
#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-bus.h"
#include "hw/virtio/virtio-pci.h"

#define VIRTIO_ID_SND 25

#define TYPE_VIRTIO_SND "virtio-snd"

#define VIRTIO_SND(obj)                                                     \
	OBJECT_CHECK(VirtioSndState, (obj), TYPE_VIRTIO_SND)


#define TYPE_VIRTIO_SND_PCI "virtio-snd-pci"

#define VIRTIO_SND_PCI(obj)                                                     \
	OBJECT_CHECK(VirtioSndPciState, (obj), TYPE_VIRTIO_SND_PCI)


typedef struct VirtioSndState {
	/* <private> */
	VirtIODevice parent_obj;
	/* <public> */
	VirtQueue *vq_ctrl;
} VirtioSndState;


typedef struct VirtioSndPciState {
	/* <private> */
	VirtIOPCIProxy parent_obj;
	/* <public> */
	VirtioSndState vsnd;
} VirtioSndPciState;

static void virtio_command_dispatch(VirtioSndState *vgpu,VirtQueueElement *elem)
{

}

static void virtio_snd_ctrl_handle(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtioSndState *vgpu = VIRTIO_SND(vdev);
	VirtQueueElement *elem;

	for (;;) {
		elem = virtqueue_pop(vgpu->vq_ctrl, sizeof(VirtQueueElement));
		if (!elem) {
			break;
		}

		virtio_command_dispatch(vgpu,elem);

		virtqueue_push(vgpu->vq_ctrl, elem, 0);
		g_free(elem);
	}
	virtio_notify(vdev, vgpu->vq_ctrl);
}

static uint64_t virtio_snd_get_features(VirtIODevice *vdev, uint64_t features,
                                        Error **errp)
{
	return features;
}

static void virtio_snd_set_features(VirtIODevice *vdev, uint64_t val)
{
}

static void virtio_snd_get_config(VirtIODevice *vdev, uint8_t *config)
{
}

static void virtio_snd_set_config(VirtIODevice *vdev, const uint8_t *config)
{
}


static void virtio_snd_realize(DeviceState *dev, Error **errp)
{
	VirtioSndState *vsnd = VIRTIO_SND(dev);

	virtio_init(VIRTIO_DEVICE(vsnd), "virtio-vgpu", VIRTIO_ID_SND,
			0);

	vsnd->vq_ctrl = virtio_add_queue(VIRTIO_DEVICE(vsnd), 64, virtio_snd_ctrl_handle);

}


static Property virtio_snd_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};


static void virtio_snd_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);
	VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

	dc->props = virtio_snd_properties;
	dc->hotpluggable = false;

	vdc->realize = virtio_snd_realize;
	vdc->get_features = virtio_snd_get_features;
	vdc->set_features = virtio_snd_set_features;
	vdc->get_config = virtio_snd_get_config;
	vdc->set_config = virtio_snd_set_config;
}

static void virtio_snd_init(Object *obj)
{

}



static Property virtio_snd_pci_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};

static void virtio_snd_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
	VirtioSndPciState *dev = VIRTIO_SND_PCI(vpci_dev);
	Error *err = NULL;

	qdev_set_parent_bus(DEVICE(&dev->vsnd), BUS(&vpci_dev->bus));
	virtio_pci_force_virtio_1(vpci_dev);
	object_property_set_bool(OBJECT(&dev->vsnd), true, "realized", &err);

	if (err) {
		error_propagate(errp, err);
		return;
	}
}

static void virtio_snd_pci_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dev_klass = DEVICE_CLASS(klass);
	PCIDeviceClass *pci_klass = PCI_DEVICE_CLASS(klass);
	VirtioPCIClass *vio_klass = VIRTIO_PCI_CLASS(klass);

	set_bit(DEVICE_CATEGORY_SOUND, dev_klass->categories);

	dev_klass->props = virtio_snd_pci_properties;
	dev_klass->hotpluggable = false;

	vio_klass->realize = virtio_snd_pci_realize;

	pci_klass->class_id = PCI_CLASS_MULTIMEDIA_AUDIO;

}

static void virtio_snd_pci_init(Object *obj)
{
	VirtioSndPciState *dev = VIRTIO_SND_PCI(obj);

	virtio_instance_init_common(obj, &dev->vsnd, sizeof(dev->vsnd),
                                TYPE_VIRTIO_SND);
}

static const TypeInfo virtio_snd_info = {
	.name = TYPE_VIRTIO_SND,
	.parent = TYPE_VIRTIO_DEVICE,
	.instance_size = sizeof(VirtioSndState),
	.instance_init = virtio_snd_init,
	.class_init = virtio_snd_class_init,
};


static const TypeInfo virtio_snd_pci_info = {
	.name = TYPE_VIRTIO_SND_PCI,
	.parent = TYPE_VIRTIO_PCI,
	.instance_size = sizeof(VirtioSndPciState),
	.instance_init = virtio_snd_pci_init,
	.class_init = virtio_snd_pci_class_init,
};

static void virtio_snd_pci_register_types(void)
{
	type_register_static(&virtio_snd_info);
	type_register_static(&virtio_snd_pci_info);
}
type_init(virtio_snd_pci_register_types)
