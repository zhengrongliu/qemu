/*
 * vgpu.c
 *
 *  Created on: Feb 23, 2020
 *      Author: jason
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

#define VIRTIO_ID_VGPU 24

#define TYPE_VIRT_GPU "virt-gpu"

#define VGPU_MAX_SCANOUTS 16

#define VIRT_GPU(obj)                                                     \
	OBJECT_CHECK(VirtGpuState, (obj), TYPE_VIRT_GPU)

enum vgpu_cmd_type {
	VGPU_CMD_MODE_SET,
	VGPU_CMD_FB_SETUP,
};

enum vgpu_color_formats {
	VGPU_FORMAT_B8G8R8A8_UNORM  = 1,
	VGPU_FORMAT_B8G8R8X8_UNORM  = 2,
	VGPU_FORMAT_A8R8G8B8_UNORM  = 3,
	VGPU_FORMAT_X8R8G8B8_UNORM  = 4,
	VGPU_FORMAT_R8G8B8A8_UNORM  = 5,
	VGPU_FORMAT_X8B8G8R8_UNORM  = 6,
	VGPU_FORMAT_A8B8G8R8_UNORM  = 7,
	VGPU_FORMAT_R8G8B8X8_UNORM  = 8,
};

struct vgpu_virtio_config{
	uint32_t num_scanouts;
};

struct vgpu_cmd_hdr{
	uint32_t type;
	uint32_t unused;
};

struct vgpu_cmd_set_mode{
	struct vgpu_cmd_hdr hdr;
	uint32_t scanout;
	uint32_t width;
	uint32_t height;
};

struct vgpu_cmd_setup_fb{
	struct vgpu_cmd_hdr hdr;
	uint32_t scanout;
	uint32_t format;
	uint32_t pitch;
	uint64_t paddr;
};



struct vgpu_scanout {
	QemuConsole *con;
	DisplaySurface *ds;
	uint32_t width;
	uint32_t height;
};


typedef struct VirtGpuState {
	/* <private> */
	VirtIODevice parent_obj;

	/* <public> */
	struct vgpu_virtio_config config;
	struct vgpu_scanout scanout[VGPU_MAX_SCANOUTS];
	VirtQueue *vq_ctrl;
	SimpleSpiceDisplay ssd;
} VirtGpuState;



#define TYPE_VIRT_GPU_PCI "virt-gpu-pci"

#define VIRT_GPU_PCI(obj)                                                     \
	OBJECT_CHECK(VirtGpuPciState, (obj), TYPE_VIRT_GPU_PCI)

typedef struct VirtGpuPciState {
	/* <private> */
	VirtIOPCIProxy parent_obj;
	/* <public> */
	VirtGpuState vdev;
} VirtGpuPciState;

static void vgpu_invalidate_display(void *opaque)
{
}

static void vgpu_update_display(void *opaque)
{
}

/* spice display interface callbacks */

static void interface_attach_worker(QXLInstance *sin, QXLWorker *qxl_worker)
{
}

static void interface_set_compression_level(QXLInstance *sin, int level)
{
}


static void interface_get_init_info(QXLInstance *sin, QXLDevInitInfo *info)
{
	VirtGpuState *vgpu = container_of(sin, VirtGpuState, ssd.qxl);

	info->memslot_gen_bits = MEMSLOT_GENERATION_BITS;
	info->memslot_id_bits = MEMSLOT_SLOT_BITS;
	info->num_memslots = NUM_MEMSLOTS;
	info->num_memslots_groups = NUM_MEMSLOTS_GROUPS;
	info->internal_groupslot_id = 0;
	info->qxl_ram_size =0;
	info->n_surfaces = vgpu->ssd.num_surfaces;

}


/* called from spice server thread context only */
static int interface_get_command(QXLInstance *sin, struct QXLCommandExt *ext)
{
	SimpleSpiceDisplay *ssd = container_of(sin, SimpleSpiceDisplay, qxl);
	SimpleSpiceUpdate *update;
	int ret = false;

	qemu_mutex_lock(&ssd->lock);
	update = QTAILQ_FIRST(&ssd->updates);
	if (update != NULL) {
		QTAILQ_REMOVE(&ssd->updates, update, next);
		*ext = update->ext;
		ret = true;
	}
	qemu_mutex_unlock(&ssd->lock);

	return ret;
}

/* called from spice server thread context only */
static int interface_req_cmd_notification(QXLInstance *sin)
{
	return 1;
}

/* called from spice server thread context only */
static void interface_release_resource(QXLInstance *sin,
                                       QXLReleaseInfoExt ext)
{
	return ;
}

/* called from spice server thread context only */
static int interface_get_cursor_command(QXLInstance *sin, struct QXLCommandExt *ext)
{
	return 0;
}

/* called from spice server thread context only */
static int interface_req_cursor_notification(QXLInstance *sin)
{
	return 1;
}

/* called from spice server thread context */
static void interface_notify_update(QXLInstance *sin, uint32_t update_id)
{
    /*
     * Called by spice-server as a result of a QXL_CMD_UPDATE which is not in
     * use by xf86-video-qxl and is defined out in the qxl windows driver.
     * Probably was at some earlier version that is prior to git start (2009),
     * and is still guest trigerrable.
     */
    fprintf(stderr, "%s: deprecated\n", __func__);
}

/* called from spice server thread context only */
static int interface_flush_resources(QXLInstance *sin)
{
    return 0;
}



/* called from spice server thread context only */
static void interface_update_area_complete(QXLInstance *sin,
        uint32_t surface_id,
        QXLRect *dirty, uint32_t num_updated_rects)
{
}

/* called from spice server thread context only */
static void interface_async_complete(QXLInstance *sin, uint64_t cookie_token)
{
}

/* called from spice server thread context only */
static void interface_set_client_capabilities(QXLInstance *sin,
                                              uint8_t client_present,
                                              uint8_t caps[58])
{
}

/* called from main context only */
static int interface_client_monitors_config(QXLInstance *sin,
                                        VDAgentMonitorsConfig *monitors_config)
{
	return 0;
}




static const QXLInterface qxl_interface = {
	.base.type               = SPICE_INTERFACE_QXL,
	.base.description        = "vgpu",
	.base.major_version      = SPICE_INTERFACE_QXL_MAJOR,
	.base.minor_version      = SPICE_INTERFACE_QXL_MINOR,

	.attache_worker          = interface_attach_worker,
	.set_compression_level   = interface_set_compression_level,
	.get_init_info           = interface_get_init_info,

	/* the callbacks below are called from spice server thread context */
	.get_command             = interface_get_command,
	.req_cmd_notification    = interface_req_cmd_notification,
	.release_resource        = interface_release_resource,
	.get_cursor_command      = interface_get_cursor_command,
	.req_cursor_notification = interface_req_cursor_notification,
	.notify_update           = interface_notify_update,
	.flush_resources         = interface_flush_resources,
	.async_complete          = interface_async_complete,
	.update_area_complete    = interface_update_area_complete,
	.set_client_capabilities = interface_set_client_capabilities,
	.client_monitors_config  = interface_client_monitors_config,
};




const GraphicHwOps vgpu_graph_ops = {
	.invalidate = vgpu_invalidate_display,
	.gfx_update = vgpu_update_display,
};


static Property vgpu_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};

static pixman_format_code_t get_pixman_format(uint32_t vgpu_format)
{
    switch (vgpu_format) {
    case VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM:
        return PIXMAN_BE_b8g8r8x8;
    case VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM:
        return PIXMAN_BE_b8g8r8a8;
    case VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM:
        return PIXMAN_BE_x8r8g8b8;
    case VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM:
        return PIXMAN_BE_a8r8g8b8;
    case VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM:
        return PIXMAN_BE_r8g8b8x8;
    case VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM:
        return PIXMAN_BE_r8g8b8a8;
    case VIRTIO_GPU_FORMAT_X8B8G8R8_UNORM:
        return PIXMAN_BE_x8b8g8r8;
    case VIRTIO_GPU_FORMAT_A8B8G8R8_UNORM:
        return PIXMAN_BE_a8b8g8r8;
    default:
        return 0;
    }
}
#if 0
static uint32_t get_spice_format(uint32_t vgpu_format)
{
	switch (vgpu_format) {
	case VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM:
		return SPICE_SURFACE_FMT_32_xRGB;
	case VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM:
		return SPICE_SURFACE_FMT_32_ARGB;
	default:
		return 0;
	}
}

#endif




static void vgpu_cmd_fb_setup_handle(VirtGpuState *vgpu,VirtQueueElement *elem)
{
	struct vgpu_cmd_setup_fb cmd;
	struct vgpu_scanout *scanout;
        pixman_image_t *rect;
	pixman_format_code_t format;
	void *addr;
	hwaddr size;
	size_t len;

	len = iov_to_buf(elem->out_sg,elem->out_num,0,&cmd,sizeof(cmd));
        if(len != sizeof(cmd)){
        	return ;
        }

        le32_to_cpus(&cmd.hdr.type);
        le32_to_cpus(&cmd.scanout);
        le32_to_cpus(&cmd.format);
        le32_to_cpus(&cmd.pitch);
        le64_to_cpus(&cmd.paddr);

        scanout = &vgpu->scanout[cmd.scanout];
        addr = cpu_physical_memory_map(cmd.paddr,&size,0);

        size = scanout->height * cmd.pitch;
        format = get_pixman_format(cmd.format);

        if(addr == 0){
        	dpy_gfx_replace_surface(scanout->con, NULL);
        	return ;
        }

        if(scanout->ds && surface_data(scanout->ds) == addr &&
        		surface_format(scanout->ds) == format){
        	return;
        }


        rect = pixman_image_create_bits(format,scanout->width,scanout->height,
        		addr,cmd.pitch);

        scanout->ds = qemu_create_displaysurface_pixman(rect);
        pixman_image_unref(rect);

        dpy_gfx_replace_surface(scanout->con, scanout->ds);
#if 0

	QXLInstance *inst = &vgpu->ssd.qxl;
	QXLDevSurfaceCreate surface;

        surface.format     = get_spice_format(cmd.format);
        surface.height     = le32_to_cpu(scanout->height);
        surface.width      = le32_to_cpu(scanout->width);
        surface.mem        = addr;
        surface.position   = 0;
        surface.stride     = le32_to_cpu(cmd->pitch);
        surface.type       = 0;
        surface.flags      = 0;
        surface.mouse_mode = true;
        surface.group_id   = MEMSLOT_GROUP_GUEST;

        spice_qxl_create_primary_surface(inst, 0, &surface);
#endif
}

static void vgpu_cmd_mode_set_handle(VirtGpuState *vgpu,VirtQueueElement *elem)
{
	struct vgpu_cmd_set_mode set_mode;
	struct vgpu_scanout *scanout;
	size_t len;

	len = iov_to_buf(elem->out_sg,elem->out_num,0,&set_mode,sizeof(set_mode));
        if(len != sizeof(set_mode)){
        	return ;
        }

        le32_to_cpus(&set_mode.hdr.type);
        le32_to_cpus(&set_mode.scanout);
        le32_to_cpus(&set_mode.width);
        le32_to_cpus(&set_mode.height);


        scanout = &vgpu->scanout[set_mode.scanout];

        scanout->width = set_mode.width;
        scanout->height = set_mode.height;





}

static void vgpu_command_dispatch(VirtGpuState *vgpu,VirtQueueElement *elem)
{
	struct vgpu_cmd_hdr cmd;
	size_t len;

	len = iov_to_buf(elem->out_sg,elem->out_num,0,&cmd,sizeof(cmd));
        if(len != sizeof(cmd)){
        	return ;
        }

        le32_to_cpus(&cmd.type);

        switch(cmd.type){
        case VGPU_CMD_MODE_SET:
        	vgpu_cmd_mode_set_handle(vgpu,elem);
        	break;
        case VGPU_CMD_FB_SETUP:
        	vgpu_cmd_fb_setup_handle(vgpu,elem);
        	break;
        }

}

static void vgpu_virtqueue_ctrl_handle(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtGpuState *vgpu = VIRT_GPU(vdev);
	VirtQueueElement *elem;

	for (;;) {
		elem = virtqueue_pop(vgpu->vq_ctrl, sizeof(VirtQueueElement));
		if (!elem) {
			break;
		}

		vgpu_command_dispatch(vgpu,elem);

		virtqueue_push(vgpu->vq_ctrl, elem, 0);
		g_free(elem);
	}
	virtio_notify(vdev, vgpu->vq_ctrl);
}



static uint64_t vgpu_get_features(VirtIODevice *vdev, uint64_t features,
                                        Error **errp)
{
	return features;
}

static void vgpu_set_features(VirtIODevice *vdev, uint64_t val)
{

}



static void vgpu_get_config(VirtIODevice *vdev, uint8_t *config)
{
	VirtGpuState *vgpu = VIRT_GPU(vdev);
	memcpy(config, &vgpu->config, sizeof(vgpu->config));
}

static void vgpu_set_config(VirtIODevice *vdev, const uint8_t *config)
{
}


static void vgpu_realize(DeviceState *dev, Error **errp)
{
	VirtGpuState *vgpu = VIRT_GPU(dev);
	int i;

	vgpu->config.num_scanouts = 1;
	virtio_init(VIRTIO_DEVICE(vgpu), "virtio-vgpu", VIRTIO_ID_VGPU,
			sizeof(vgpu->config));

	vgpu->vq_ctrl = virtio_add_queue(VIRTIO_DEVICE(vgpu), 64, vgpu_virtqueue_ctrl_handle);

	for(i = 0;i < vgpu->config.num_scanouts;i++){
		vgpu->scanout[i].con =
				graphic_console_init(DEVICE(dev),i,&vgpu_graph_ops,vgpu);
		if(i > 0){
			dpy_gfx_replace_surface(vgpu->scanout[i].con, NULL);
		}
	}
	vgpu->ssd.qxl.base.sif = &qxl_interface.base;
#if 0
	if (qemu_spice_add_display_interface(&vgpu->ssd.qxl, vgpu->scanout[0].con) != 0) {
		error_setg(errp, "qxl interface %d.%d not supported by spice-server",
                   SPICE_INTERFACE_QXL_MAJOR, SPICE_INTERFACE_QXL_MINOR);
		return;
	}
#endif
}


static void vgpu_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);
	VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

	dc->props = vgpu_properties;
	dc->hotpluggable = false;

	vdc->realize = vgpu_realize;
	vdc->get_features = vgpu_get_features;
	vdc->set_features = vgpu_set_features;
	vdc->get_config = vgpu_get_config;
	vdc->set_config = vgpu_set_config;
}

static void vgpu_init(Object *obj)
{

}



static Property vgpu_pci_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};

static void vgpu_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
	VirtGpuPciState *dev = VIRT_GPU_PCI(vpci_dev);
	Error *err = NULL;

	qdev_set_parent_bus(DEVICE(&dev->vdev), BUS(&vpci_dev->bus));
	virtio_pci_force_virtio_1(vpci_dev);
	object_property_set_bool(OBJECT(&dev->vdev), true, "realized", &err);

	if (err) {
		error_propagate(errp, err);
		return;
	}
}

static void vgpu_pci_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dev_klass = DEVICE_CLASS(klass);
	PCIDeviceClass *pci_klass = PCI_DEVICE_CLASS(klass);
	VirtioPCIClass *vio_klass = VIRTIO_PCI_CLASS(klass);

	set_bit(DEVICE_CATEGORY_DISPLAY, dev_klass->categories);

	dev_klass->props = vgpu_pci_properties;
	dev_klass->hotpluggable = false;
	vio_klass->realize = vgpu_pci_realize;

	pci_klass->class_id = PCI_CLASS_DISPLAY_OTHER;

}

static void vgpu_pci_init(Object *obj)
{
	VirtGpuPciState *dev = VIRT_GPU_PCI(obj);

	virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VIRT_GPU);
}

static const TypeInfo vgpu_info = {
	.name = TYPE_VIRT_GPU,
	.parent = TYPE_VIRTIO_DEVICE,
	.instance_size = sizeof(VirtGpuState),
	.instance_init = vgpu_init,
	.class_init = vgpu_class_init,
};


static const TypeInfo vgpu_pci_info = {
	.name = TYPE_VIRT_GPU_PCI,
	.parent = TYPE_VIRTIO_PCI,
	.instance_size = sizeof(VirtGpuPciState),
	.instance_init = vgpu_pci_init,
	.class_init = vgpu_pci_class_init,
};

static void vgpu_pci_register_types(void)
{
	type_register_static(&vgpu_info);
	type_register_static(&vgpu_pci_info);
}
type_init(vgpu_pci_register_types)
