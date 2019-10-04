/*
 * usbip-stub.c
 *
 *  Created on: Feb 27, 2019
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "hw/hw.h"
#include "ui/console.h"
#include "hw/usb.h"
#include "hw/usb/desc.h"
#include "qapi/qmp/qerror.h"
#include "qemu/error-report.h"
#include "sysemu/sysemu.h"
#include "chardev/char-fe.h"
#include "qemu/error-report.h"
#include "usbip_common.h"


#define TYPE_USBIP_DEVICE	"usbip"
#define USBIP_DEVICE(obj) OBJECT_CHECK(USBIPDevice, (obj), TYPE_USBIP_DEVICE)


#define URB_SHORT_NOT_OK	0x0001	/* report short reads as errors */
#define URB_ISO_ASAP		0x0002	/* iso-only; use the first unexpired
					 * slot in the schedule */
#define URB_ZERO_PACKET		0x0040	/* Finish bulk OUT with short packet */

#define usbip_err(dev,...) \
    do { \
        if (dev->debug >= usbip_dbglvl_error) { \
            error_report(TYPE_USBIP_DEVICE ":" __VA_ARGS__); \
        } \
    } while (0)
#define usbip_info(dev,...) \
    do { \
        if (dev->debug >= usbip_dbglvl_info) { \
            info_report(TYPE_USBIP_DEVICE ":" __VA_ARGS__); \
        } \
    } while (0)
#define usbip_warn(dev,...) \
    do { \
        if (dev->debug >= usbip_dbglvl_warn) { \
            warn_report(TYPE_USBIP_DEVICE ":" __VA_ARGS__); \
        } \
    } while (0)
#define usbip_dbg(dev,...) \
    do { \
        if (dev->debug >= usbip_dbglvl_debug) { \
            info_report(TYPE_USBIP_DEVICE ":" __VA_ARGS__); \
        } \
    } while (0)

enum usbip_debug_level{
	usbip_dbglvl_error,
	usbip_dbglvl_info,
	usbip_dbglvl_warn,
	usbip_dbglvl_debug,
};

struct usbip_parser{

};

typedef struct USBIPDevice{
	/* <private> */
	USBDevice dev;

	/* <public> */
	CharBackend cs;
	int debug;
	QEMUBH *chardev_close_bh;

	uint32_t remote_id;
	uint32_t seqnum;
}USBIPDevice;



static void usbip_header_correct_endian(struct usbip_header *pdu, int send)
{

}

static void usbip_get_config(USBIPDevice *dev, USBPacket *p)
{

}

static void usbip_get_descriptor(USBIPDevice *dev, USBPacket *p,int value,int index)
{

}


static void usbip_get_interface(USBIPDevice *dev, USBPacket *p,int index)
{

}



static void usbip_cancel_packet(USBDevice *udev, USBPacket *p)
{

}

static void usbip_handle_reset(USBDevice *udev)
{

}

static void usbip_handle_data(USBDevice *udev, USBPacket *p)
{
	USBIPDevice * updev = USBIP_DEVICE(udev);
	struct usbip_header pdu;
	struct usbip_header_cmd_submit *spdu = &pdu.u.cmd_submit;
	int dir = (p->pid == USB_TOKEN_IN)?
			USBIP_DIR_IN:USBIP_DIR_OUT;
	size_t length = usb_packet_size(p);

	memset(&pdu,0,sizeof(pdu));

	pdu.base.command   = USBIP_CMD_SUBMIT;
	pdu.base.seqnum    = updev->seqnum++;
	pdu.base.devid     = updev->remote_id;
	pdu.base.direction = dir;
	pdu.base.ep	   = p->ep->nr;

	spdu->transfer_buffer_length = length;
	if(p->short_not_ok){
		spdu->transfer_flags |= URB_SHORT_NOT_OK;
	}

	usbip_header_correct_endian(&pdu, 1);

	qemu_chr_fe_write_all(&updev->cs,(uint8_t *)&pdu,sizeof(pdu));
	if(dir == USBIP_DIR_OUT && length > 0){
		QEMUIOVector *iov = &p->iov;
		int i;

		for(i = 0;i < iov->niov;i++){
			qemu_chr_fe_write_all(&updev->cs,
					iov->iov[i].iov_base,iov->iov[i].iov_len);
		}
	}
#if 0
	if(p->ep->type == USB_ENDPOINT_XFER_ISOC){
		struct usbip_iso_packet_descriptor *iso_desc;
	}
#endif
	p->status = USB_RET_ASYNC;


}

static void usbip_handle_control(USBDevice *udev, USBPacket *p,
        int request, int value, int index, int length, uint8_t *data)
{
	USBIPDevice * updev = USBIP_DEVICE(udev);
	struct usbip_header pdu;
	struct usbip_header_cmd_submit *spdu = &pdu.u.cmd_submit;
	uint8_t *setup;
	int dir = (request & USB_DIR_IN<<8)?
			USBIP_DIR_IN:USBIP_DIR_OUT;

	switch (request) {
	case DeviceOutRequest | USB_REQ_SET_ADDRESS:
		udev->addr = value;
		return;
	case DeviceRequest | USB_REQ_GET_CONFIGURATION:
		usbip_get_config(updev, p);
		return;
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		usbip_get_descriptor(updev, p,value,index);
		return;
	case InterfaceRequest | USB_REQ_GET_INTERFACE:
		usbip_get_interface(updev, p, index);
		return;
	}

	memset(&pdu,0,sizeof(pdu));

	pdu.base.command   = USBIP_CMD_SUBMIT;
	pdu.base.seqnum    = updev->seqnum++;
	pdu.base.devid     = updev->remote_id;
	pdu.base.direction = dir;
	pdu.base.ep	   = p->ep->nr;

	setup = spdu->setup;

	setup[0] = request>>8;
	setup[1] = request;
	setup[2] = value;
	setup[3] = value>>8;
	setup[4] = index;
	setup[5] = index>>8;
	setup[6] = length;
	setup[7] = length>>8;

	spdu->transfer_buffer_length = length;

	usbip_header_correct_endian(&pdu, 1);

	qemu_chr_fe_write_all(&updev->cs,(uint8_t *)&pdu,sizeof(pdu));
	if(dir == USBIP_DIR_OUT && length > 0){
		qemu_chr_fe_write_all(&updev->cs,data,length);
	}

	p->status = USB_RET_ASYNC;
}

static void usbip_chardev_close_bh(void *opaque)
{

}

static int usbip_chardev_can_read(void *opaque)
{
	if (!runstate_check(RUN_STATE_RUNNING)) {
		return 0;
	}

	return 1024*1024;
}

static void usbip_chardev_read(void *opaque, const uint8_t *buf, int size)
{
	//USBIPDevice *dev = opaque;

}

static void usbip_chardev_event(void *opaque, int event)
{

	USBIPDevice *dev = opaque;

	switch (event) {
	case CHR_EVENT_OPENED:
		usbip_info(dev,"chardev open\n");
		/* Make sure any pending closes are handled (no-op if none pending) */
		usbip_chardev_close_bh(dev);
		qemu_bh_cancel(dev->chardev_close_bh);
		break;
	case CHR_EVENT_CLOSED:
		usbip_info(dev,"chardev close\n");
		qemu_bh_schedule(dev->chardev_close_bh);
		break;
	}


}


static void usbip_realize(USBDevice *udev, Error **errp)
{
	USBIPDevice *dev = USBIP_DEVICE(udev);

	if (!qemu_chr_fe_backend_connected(&dev->cs)) {
		error_setg(errp, QERR_MISSING_PARAMETER, "chardev");
		return;
	}


	usb_ep_init(&dev->dev);

	dev->chardev_close_bh = qemu_bh_new(usbip_chardev_close_bh, dev);

	qemu_chr_fe_set_handlers(&dev->cs, usbip_chardev_can_read,
                             usbip_chardev_read, usbip_chardev_event,
                             NULL, dev, NULL, true);

}

static void usbip_unrealize(USBDevice *udev, Error **errp)
{
	USBIPDevice *dev = USBIP_DEVICE(udev);

	qemu_chr_fe_deinit(&dev->cs, true);
	qemu_bh_delete(dev->chardev_close_bh);
}

static void usbip_flush_ep_queue(USBDevice *dev, USBEndpoint *ep)
{
	if (ep->pid == USB_TOKEN_IN && ep->pipeline) {
		usb_ep_combine_input_packets(ep);
	}

}

static void usbip_ep_stopped(USBDevice *udev, USBEndpoint *uep)
{

}

static Property usbip_properties[] = {
	DEFINE_PROP_CHR("chardev", USBIPDevice, cs),
	DEFINE_PROP_END_OF_LIST(),
};



static void usbip_class_init(ObjectClass *klass, void *data)
{
	USBDeviceClass *uc = USB_DEVICE_CLASS(klass);
	DeviceClass *dc = DEVICE_CLASS(klass);


	uc->product_desc   = "USBIP Device";
	uc->realize        = usbip_realize;
	uc->unrealize      = usbip_unrealize;
	uc->cancel_packet  = usbip_cancel_packet;
	uc->handle_reset   = usbip_handle_reset;
	uc->handle_data    = usbip_handle_data;
	uc->handle_control = usbip_handle_control;
	uc->flush_ep_queue = usbip_flush_ep_queue;
	uc->ep_stopped     = usbip_ep_stopped;


	//uc->alloc_streams  = usbip_alloc_streams;
	//uc->free_streams   = usbip_free_streams;


	dc->props = usbip_properties;
}

static void usbip_instance_init(Object *obj)
{

}

static const TypeInfo usbip_type_info = {
	.name = TYPE_USBIP_DEVICE,
	.parent = TYPE_USB_DEVICE,
	.instance_size = sizeof(USBIPDevice),
	.class_init = usbip_class_init,
	.instance_init = usbip_instance_init,
};


static void usbip_register_types(void)
{
	type_register_static(&usbip_type_info);
}

type_init(usbip_register_types)



