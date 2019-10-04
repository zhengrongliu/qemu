/*
 * hcd-vhci.c
 *
 *  Created on: Nov 24, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "exec/address-spaces.h"
#include "sysemu/dma.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hcd-vhci.h"


/*
 * vhci_raise_irq - set status and update irq
 *
 * @vhci - vhci device object
 * @st	- status
 */
static void vhci_raise_irq(VhciState *vhci,uint32_t st)
{

	vhci->status |= st;

	vhci_update_irq(vhci);
}




/*
 * vhci_alloc_packet - alloc local packet and link it into transfer structure
 *
 * @vhci - vhci device object
 * @xfer - vhci transfer object
 */
static struct vhci_packet *vhci_alloc_packet(VhciState *vhci,struct vhci_transfer *xfer)
{
	struct vhci_packet *p;

	p = g_new0(struct vhci_packet,1);
	if(!p)
		return NULL;

	usb_packet_init(&p->packet);

	QTAILQ_INSERT_TAIL(&xfer->packets,p,queue);

	p->transfer = xfer;
	p->async_status = VHCI_ASYNC_NONE;

	return p;
}

/*
 * vhci_free_packet - free vhci packet allocated by vhci_alloc_packet
 *
 * it should free sgl and clean usb packet
 *
 * @vhci - vhci device object
 * @p - the vhci_packet will be free
 */
static void vhci_free_packet(VhciState *vhci,struct vhci_packet *p)
{

	usb_packet_unmap(&p->packet,&p->sgl);
	if(p->sgl.sg != NULL)
		qemu_sglist_destroy(&p->sgl);
        usb_packet_cleanup(&p->packet);
	g_free(p);
}

/*
 * vhci_alloc_transfer - alloc transfer
 *
 * the packets link list must be init
 *
 * @vhci - vhci device object
 */
static struct vhci_transfer *vhci_alloc_transfer(VhciState *vhci)
{
	struct vhci_transfer *p;

	p = g_new0(struct vhci_transfer,1);
	if(!p)
		return NULL;

	p->async_status = VHCI_ASYNC_NONE;

	QTAILQ_INIT(&p->packets);
	return p;
}

/*
 * vhci_free_transfer - free transfer
 *
 * @vhci - vhci device object
 */
static void vhci_free_transfer(VhciState *vhci,struct vhci_transfer *xfer)
{
	struct vhci_packet *packet,*next;

	QTAILQ_FOREACH_SAFE(packet,&xfer->packets,queue,next){
		QTAILQ_REMOVE(&xfer->packets,packet,queue);
		vhci_free_packet(vhci,packet);
	}
	g_free(xfer);
}

/*
 * vhci_find_device - find USBDevice according usb addr
 *
 * every port is a root of usb device tree,so let's
 * search all port unit find device.
 *
 * @vhci - vhci device object
 * @addr - the usb addr of usb device
 */
static USBDevice *vhci_find_device(VhciState *vhci,uint8_t addr)
{
	USBDevice *dev;
	USBPort *port;
	int i;
	for(i = 0;i < ARRAY_SIZE(vhci->ports);i++){
		port = &vhci->ports[i];

		dev = usb_find_device(port,addr);
		if(dev != NULL){
			return dev;
		}
	}

	return NULL;
}

/*
 * vhci_transfer_status - translate packet status to transfer status
 *
 * @packet_status - the status of packet
 */

static int vhci_transfer_status(int packet_status)
{

	switch(packet_status){
	case USB_RET_SUCCESS:
		return VHCI_HEAD_STATUS_XFER_OK;
	case USB_RET_NODEV:
		return VHCI_HEAD_STATUS_ERROR_NODEV;
	case USB_RET_STALL:
		return VHCI_HEAD_STATUS_XFER_STALL;
	case USB_RET_BABBLE:
		return VHCI_HEAD_STATUS_ERROR_BABBLE;
	case USB_RET_IOERROR:
		return VHCI_HEAD_STATUS_ERROR_IO;
	}

	return VHCI_HEAD_STATUS_ERROR_IOERR;
}

/*
 * vhci_complete_transfer - complete transfer
 *
 * update status and notify guest a transfer was completed
 *
 * @vhci - vhci device object
 * @transfer - vhci transfer
 */
static int vhci_complete_transfer(VhciState *vhci,struct vhci_transfer *transfer)
{
	struct vhci_ringnode_head *head;
	struct vhci_packet *vpacket;
	union vhci_ringnode *node;
	unsigned int index;


	index = transfer->head;
	node = (union vhci_ringnode*)vhci->ring_qemu_base;

	head = &node[index].head;

	head->length = 0;

	QTAILQ_FOREACH(vpacket,&transfer->packets,queue){
		if(vpacket->packet.pid != USB_TOKEN_SETUP){
			head->length += vpacket->packet.actual_length;
		}
		if(head->status == VHCI_HEAD_STATUS_XFER_OK){
			head->status = vhci_transfer_status(vpacket->packet.status);
		}
	}


	QTAILQ_INSERT_TAIL(&vhci->finished,transfer,queue);

	vhci_raise_irq(vhci,VHCI_ST_XFER_DONE);

	return 0;
}

/*
 * vhci_packet_setup - setup packet data
 *
 * @vhci - vhci device object
 * @vpacket - the packet of vhci
 * @ep - the endpoint
 * @head - the head ring node of transfer
 * @pid - the pid of transfer
 * @segment - the first segment of packet
 * @count - the count of segments of packet
 * @irq - whether the packet will toggle interrupt
 */

static void vhci_packet_setup(VhciState *vhci,struct vhci_packet *vpacket,
		USBEndpoint *ep,struct vhci_ringnode_head *head,int pid,
		struct vhci_ringnode_segment *segment,int count,bool irq)
{

	struct vhci_ringnode_base *base = (struct vhci_ringnode_base *)vhci->ring_qemu_base;
	struct vhci_ringnode_list *list;
	bool short_not_ok;
	uint64_t packet_id;

	short_not_ok = head->request&VHCI_REQ_SHORT_NOT_OK;

	packet_id = ((uint64_t)head) <<32;


	/*
	 * setup packet
	 */
	usb_packet_setup(&vpacket->packet, pid, ep, 0,
        		packet_id++,short_not_ok ,irq);
	qemu_sglist_init(&vpacket->sgl, vhci->device, count, vhci->as);


	list = &segment->list;
	/*
	 * fetch the remain segments and add to sgl
	 */
	for(;count >0;count--){
		qemu_sglist_add(&vpacket->sgl,segment->addr,segment->length);
		list = &segment->list;
		segment = vhci_node_list_next_segment(base,list);
	}


}

/*
 * vhci_handle_data - handle bulk/int/iso transfer
 *
 * @vhci - vhci device object
 * @head - the head info of transfer
 * @dev - the usb device
 * @ep - the endpoint
 */
static int vhci_handle_data(VhciState *vhci,struct vhci_ringnode_head *head,struct vhci_transfer *xfer)
{
	struct vhci_packet *vpacket;
	struct vhci_ringnode_base *base = (struct vhci_ringnode_base *)vhci->ring_qemu_base;
	struct vhci_ringnode_segment *segment;
	USBDevice *dev;
	USBEndpoint *ep;
	int count;
	int ret = 0;
	int pid;


	/*
	 * fetch enough infomation of head
	 */
	count = head->segments;
	pid = (head->request & USB_DIR_IN)?USB_TOKEN_IN:USB_TOKEN_OUT;


	dev = vhci_find_device(vhci,head->addr);
	if(!dev){
		vhci_err(vhci,"vhci get device failed\n");
		return -ENODEV;
	}

	ep = usb_ep_get(dev,(head->request&USB_DIR_IN)?USB_TOKEN_IN:USB_TOKEN_OUT,
			head->epnum);
	if(!ep){
		vhci_err(vhci,"vhci get device endpoint failed\n");
		return -ENODEV;
	}


	/*
	 * alloc and setup packet
	 */
	vpacket = vhci_alloc_packet(vhci,xfer);
	if(!vpacket){
		ret = -ENOMEM;
		goto failed;
	}

	segment = vhci_node_list_next_segment(base,&head->list);
	vhci_packet_setup(vhci,vpacket,ep,head,pid,segment,count,true);


	usb_packet_map(&vpacket->packet,&vpacket->sgl);
	usb_handle_packet(dev,&vpacket->packet);


	/*
	 * check packet status
	 */
	if(vpacket->packet.status == USB_RET_ASYNC){
		xfer->async_status = VHCI_ASYNC_INFLIGHT;
		vpacket->async_status = VHCI_ASYNC_INFLIGHT;
	}

	/*
	 * check transfer status
	 */
	if(xfer->async_status == VHCI_ASYNC_INFLIGHT){
		QTAILQ_INSERT_TAIL(&vhci->inflight,xfer,queue);
		usb_device_flush_ep_queue(dev, ep);
		return 0;
	}


	vhci_complete_transfer(vhci,xfer);

	return 0;
failed:
	vhci_free_transfer(vhci,xfer);
	return ret;
}

/*
 * vhci_handle_control - handle control transfer
 *
 * there will be two or three packet for control transfer
 *
 * @vhci - vhci device object
 * @head - head info of transfer
 * @dev - usb device
 * @ep - usb endpoint
 */
static int vhci_handle_control(VhciState *vhci,struct vhci_ringnode_head *head,struct vhci_transfer *xfer)
{
	struct vhci_packet *vpackets[3] = {},*vpacket;
	struct vhci_packet **p;
	struct vhci_ringnode_segment *segment;
	struct vhci_ringnode_base *base;
	USBDevice *dev;
	USBEndpoint *ep;
	int count;
	int pid;
	int ret = 0;

	base = (struct vhci_ringnode_base*)vhci->ring_qemu_base;

	dev = vhci_find_device(vhci,head->addr);
	if(!dev){
		vhci_err(vhci,"vhci get device failed\n");
		return -ENODEV;
	}

	ep = usb_ep_get(dev,(head->request&USB_DIR_IN)?USB_TOKEN_IN:USB_TOKEN_OUT,
			head->epnum);
	if(!ep){
		vhci_err(vhci,"vhci get device endpoint failed\n");
		return -ENODEV;
	}


	/*
	 * fetch info from head node
	 */
	count = head->segments;
	pid = (head->request & USB_DIR_IN)?USB_TOKEN_IN:USB_TOKEN_OUT;





	p = vpackets;

	/*
	 * usb control setup packet
	 */
	*p = vhci_alloc_packet(vhci,xfer);
	if(!*p){
		vhci_free_transfer(vhci,xfer);
		return -ENOMEM;
	}

	segment = vhci_node_list_next_segment(base,&head->list);
	vhci_packet_setup(vhci,*p,ep,head,USB_TOKEN_SETUP,segment,1,false);
	p++;

	/*
	 * usb control data packet
	 */
	if(head->length > 0){
		*p = vhci_alloc_packet(vhci,xfer);
		if(!*p){
			ret = -ENOMEM;
			goto failed_alloc;
		}

		segment = vhci_node_list_next_segment(base,&segment->list);
		vhci_packet_setup(vhci,*p,ep,head,pid,segment,count-2,false);
		p++;

	}


	/*
	 * usb control ack packet
	 */
	*p = vhci_alloc_packet(vhci,xfer);
	if(!*p){
		vhci_free_transfer(vhci,xfer);
		return -ENOMEM;
	}

	segment = vhci_node_list_prev_segment(base,&head->list);
	vhci_packet_setup(vhci,*p,ep,head,
			(pid == USB_TOKEN_IN)?USB_TOKEN_OUT:USB_TOKEN_IN,segment,1,true);

	/*
	 * handle all packet of transfer
	 */
	QTAILQ_FOREACH(vpacket,&xfer->packets,queue){
		usb_packet_map(&vpacket->packet,&vpacket->sgl);
		usb_handle_packet(dev,&vpacket->packet);

		if(vpacket->packet.status  == USB_RET_ASYNC){
			vpacket->async_status = VHCI_ASYNC_INFLIGHT;
			xfer->async_status = VHCI_ASYNC_INFLIGHT;
		} else {
			vpacket->async_status = VHCI_ASYNC_FINISHED;
		}
	}

	/*
	 * check async status
	 */
	if(xfer->async_status == VHCI_ASYNC_INFLIGHT){
		QTAILQ_INSERT_TAIL(&vhci->inflight,xfer,queue);
		usb_device_flush_ep_queue(dev, ep);
		return 0;
	}


	vhci_complete_transfer(vhci,xfer);


	return 0;

failed_alloc:
	vhci_free_transfer(vhci,xfer);
	return ret;
}


/*
 * vhci_handle_error - handle error
 *
 * return error direct to driver
 *
 * @vhci - vhci device object
 * @head - head info of transfer
 * @error_code - error code
 */

static int vhci_handle_error(VhciState *vhci,struct vhci_ringnode_head *head,struct vhci_transfer *xfer,
		int error_code)
{

	head->length = 0;

	switch(error_code){
	case -ENODEV:
		head->status = VHCI_HEAD_STATUS_ERROR_NODEV;
		break;
	}

	vhci_complete_transfer(vhci,xfer);
	return 0;
}
/*
 * vhci_submit_transfer - guest submit transfer to hc
 *
 * @vhci - vhci device object
 * @index - the index of head packet
 */
static int vhci_submit_transfer(VhciState *vhci,int index)
{
	struct vhci_ringnode_head *head;
	union vhci_ringnode *node;
	struct vhci_transfer *xfer;
	int ret;

	node = (union vhci_ringnode*)vhci->ring_qemu_base;
	head = &node[index].head;

	xfer = vhci_alloc_transfer(vhci);
	if(!xfer){
		return -ENOMEM;
	}

	/*
	 * record the head index in order to locate transfer node
	 */
	xfer->head = vhci_node_list_index(&node->base,&head->list);

	if((head->request & VHCI_REQ_TYPE_MASK) == VHCI_REQ_TYPE_CONTROL){
		ret = vhci_handle_control(vhci,head,xfer);
	} else {
		ret = vhci_handle_data(vhci,head,xfer);
	}

	/*
	 * handle error
	 */
	if(ret){
		vhci_handle_error(vhci,head,xfer,ret);
	}


	return 0;
}



/*
 * vhci_cancel_transfer - cancel transfer
 *
 * update status and notify guest a transfer was canceld
 *
 * @vhci - vhci device object
 * @index - index of head packet
 */
static int vhci_cancel_transfer(VhciState *vhci,int index)
{
	struct vhci_transfer *xfer = NULL;
	struct vhci_packet *vpacket = NULL;
	struct vhci_ringnode_head *head;
	union vhci_ringnode *node;

	node = (union vhci_ringnode*)vhci->ring_qemu_base;
	head = &node[index].head;

	QTAILQ_FOREACH(xfer,&vhci->inflight,queue){
		if(xfer->head == index){
			break;
		}
	}

	if(!xfer){
		return -ENODEV;
	}

	/*
	 * cancel all inflight USBPacket
	 */
	QTAILQ_FOREACH(vpacket,&xfer->packets,queue){
		if(vpacket->async_status == VHCI_ASYNC_INFLIGHT){
			usb_cancel_packet(&vpacket->packet);
		}
	}

	/*
	 * update head information
	 */
	head->status = VHCI_HEAD_STATUS_XFER_CANCEL;
	head->length = 0;

	QTAILQ_REMOVE(&vhci->inflight,xfer,queue);
	xfer->async_status = VHCI_ASYNC_FINISHED;

	vhci_complete_transfer(vhci,xfer);

	return 0;
}

/*
 * vhci_done_transfer - done transfer
 *
 * was called when transfer was done
 *
 * @vhci - vhci device object
 */
static int vhci_done_transfer(VhciState *vhci)
{
	struct vhci_transfer *xfer;
	int ret;

	if(QTAILQ_EMPTY(&vhci->finished)){
		return ~0;
	}

	xfer = QTAILQ_FIRST(&vhci->finished);
	ret = xfer->head;

	QTAILQ_REMOVE(&vhci->finished,xfer,queue);

	vhci_free_transfer(vhci,xfer);

	if(!QTAILQ_EMPTY(&vhci->finished))
		vhci_raise_irq(vhci,VHCI_ST_XFER_DONE);

	return ret;

}

/*
 * vhci_port_command - done transfer
 *
 * was called when transfer was done
 *
 * @vhci - vhci device object
 * @port - port index
 * @cmd - port command
 */
static int vhci_port_command(VhciState *vhci,unsigned int port,unsigned int cmd)
{
	USBPort *usbport = &vhci->ports[port];
	uint32_t *status = &vhci->port_status[port];
	switch(cmd){
	case VHCI_PORT_CMD_RESET:
		if(usbport->dev && usbport->dev->attached){
			usb_port_reset(&vhci->ports[port]);
			*status &= ~VHCI_PORT_ST_C_CONNECTION;
		}
		if(usbport->dev && usbport->dev->attached){
			*status |= VHCI_PORT_ST_ENABLE;
			*status |= VHCI_PORT_ST_C_RESET;

		}
		break;
	}
	return 0;
}

/*
 * vhci_reset - reset vhci
 */
static void vhci_reset(VhciState *vhci)
{
	int i;

	for(i = 0;i < ARRAY_SIZE(vhci->ports);i++){
		if(vhci->ports[i].dev && vhci->ports[i].dev->attached){
			usb_detach(&vhci->ports[i]);
		}
	}

	vhci->status = 0;
	vhci->intmask = 0;
	vhci->ring_base = 0;
	vhci->ring_size_power2 = 0;
	vhci->ring_qemu_size = 0;

	memset(&vhci->port_status,0,sizeof(vhci->port_status));

	for(i = 0;i < ARRAY_SIZE(vhci->ports);i++){
		if(vhci->ports[i].dev && vhci->ports[i].dev->attached){
			usb_attach(&vhci->ports[i]);
			usb_device_reset(vhci->ports[i].dev);
		}
	}

	/*
	 * TODO:cancel packet
	 */
	qemu_bh_cancel(vhci->async_bh);
}



/*
 * vhci_attach
 *
 * was called when a usb device attach to vhci
 *
 * @port - which port the device attached
 */
static void vhci_attach(USBPort *port)
{
	struct VhciState *vhci = (struct VhciState*)port->opaque;
	unsigned int speed = VHCI_PORT_ST_SPEED_HIGH;

	vhci->port_status[port->index] |= VHCI_PORT_ST_C_CONNECTION;
	vhci->port_status[port->index] |= VHCI_PORT_ST_CONNECTION;
	switch(port->dev->speed){
	case USB_SPEED_LOW:
		speed = VHCI_PORT_ST_SPEED_LOW;
		break;
	case USB_SPEED_FULL:
		speed = VHCI_PORT_ST_SPEED_FULL;
		break;
	case USB_SPEED_HIGH:
		speed = VHCI_PORT_ST_SPEED_HIGH;
		break;
	}

	vhci->port_status[port->index] |= speed;

	vhci_raise_irq(vhci,VHCI_ST_PORT_CHANGED);
}

/*
 * vhci_detach
 *
 * was called when a usb device detach from vhci
 *
 * @port - which port the device detach
 */
static void vhci_detach(USBPort *port)
{
	struct VhciState *vhci = (struct VhciState*)port->opaque;

	vhci->port_status[port->index] |= VHCI_PORT_ST_C_CONNECTION;
	vhci->port_status[port->index] &= ~(VHCI_PORT_ST_CONNECTION|VHCI_PORT_ST_ENABLE);

	vhci_raise_irq(vhci,VHCI_ST_PORT_CHANGED);

}

/*
 * vhci_async_complete_packet
 *
 * was called when a usb device complete a inflight packet
 *
 * @port - which port the device detach
 * @packet - the packet was completed
 */
static void vhci_async_complete_packet(USBPort *port, USBPacket *packet)
{
	struct VhciState *vhci = (struct VhciState *)port->opaque;
	struct vhci_packet *vpacket = container_of(packet,struct vhci_packet,packet);
	struct vhci_transfer *xfer = vpacket->transfer;


	if(packet->status == USB_RET_REMOVE_FROM_QUEUE){
		usb_cancel_packet(packet);
	}

	vpacket->async_status = VHCI_ASYNC_FINISHED;

	if(packet->int_req){
		xfer->async_status = VHCI_ASYNC_FINISHED;
		qemu_bh_schedule(vhci->async_bh);
	}

}

/*
 * vhci_async_bh
 *
 * @opaque - VhciState of vhci
 */
static void vhci_async_bh(void *opaque)
{
	VhciState *vhci = opaque;
	struct vhci_transfer *xfer,*next;

	QTAILQ_FOREACH_SAFE(xfer,&vhci->inflight,queue,next){
		if(xfer->async_status == VHCI_ASYNC_FINISHED){
			QTAILQ_REMOVE(&vhci->inflight,xfer,queue);
			vhci_complete_transfer(vhci,xfer);
		}
	}
}

static USBBusOps vhci_bus_ops = {
};

static USBPortOps vhci_port_ops = {
    .attach = vhci_attach,
    .detach = vhci_detach,
#if 0
    .child_detach = ohci_child_detach,
    .wakeup = vhci_wakeup,
#endif
    .complete = vhci_async_complete_packet,
};




void vhci_qemu_init(VhciState *s, DeviceState *dev, Error **errp)
{
	int i;

	assert(sizeof(struct vhci_ringnode_segment) == sizeof(struct vhci_ringnode_head));

	s->debug = vhci_dbglvl_debug;


	s->feature  = VHCI_FT_USB2PORTNR(NR_PORTS);
	QTAILQ_INIT(&s->finished);
	QTAILQ_INIT(&s->inflight);

	s->hc_reset = vhci_reset;
	s->xfer_submit = vhci_submit_transfer;
	s->xfer_cancel = vhci_cancel_transfer;
	s->xfer_done = vhci_done_transfer;
	s->port_command = vhci_port_command;

	/*
	 * create usb bus
	 */
	usb_bus_new(&s->bus, sizeof(s->bus), &vhci_bus_ops, dev);

	s->async_bh = qemu_bh_new(vhci_async_bh, s);
	/*
	 * register usb port
	 */
	for (i = 0; i < ARRAY_SIZE(s->ports); i++) {
		usb_register_port(&s->bus, &s->ports[i], s, i, &vhci_port_ops,
                          USB_SPEED_MASK_HIGH|USB_SPEED_MASK_FULL|USB_SPEED_MASK_LOW);
		s->ports[i].dev = 0;
	}


	s->device = dev;

}





/*
 * vhci_update_irq - update irq level according to status
 */
void vhci_update_irq(VhciState *vhci)
{
	int level = 0;

	if(vhci->status & vhci->intmask){
		level = 1;
	}

	qemu_set_irq(vhci->irq, level);


}


/*
 * vhci_command - handle vhci command
 */
static void vhci_command(VhciState *vhci,uint32_t cmd,uint32_t param)
{
	switch(cmd){
	case VHCI_CMD_INIT:
		if(vhci->ring_qemu_base){
			cpu_physical_memory_unmap(vhci->ring_qemu_base,vhci->ring_qemu_size,
					1,vhci->ring_qemu_size);
		}
		vhci->ring_qemu_base = cpu_physical_memory_map(vhci->ring_base,
						&vhci->ring_qemu_size,1);
		break;
	case VHCI_CMD_SUBMIT:
		vhci->xfer_submit(vhci,param);
		break;
	case VHCI_CMD_CANCEL:
		vhci->xfer_cancel(vhci,param);
		break;
	case VHCI_CMD_START:
		break;
	case VHCI_CMD_STOP:
		break;
	case VHCI_CMD_RESET:
		vhci->hc_reset(vhci);
		break;
	}
}


/*
 * vhci_io_port_read - handle port register read
 */
static uint32_t vhci_io_port_read(VhciState *vhci,unsigned int port,
		unsigned int reg)
{
	switch(reg){
	case VHCI_PORT_STATUS_:
		return vhci->port_status[port];
	}
	return 0;
}

/*
 * vhci_io_read - handle register read
 */
uint64_t vhci_io_read(VhciState *vhci, hwaddr offset, unsigned size)
{
	unsigned int port;
	unsigned int reg;

	switch(offset){
	case VHCI_PORT_BASE ... VHCI_PORT_LIMIT(NR_PORTS):
		/*
		 *  the register be read is a port register,
		 *  find the port nr and port register offset
		 */
		port = (offset - VHCI_PORT_BASE)/VHCI_PORT_SIZE;
		reg = offset - (port * VHCI_PORT_SIZE) - VHCI_PORT_BASE;

		return vhci_io_port_read(vhci,port,reg);
	case VHCI_STATUS:
		return vhci->status;
	case VHCI_FEATURE:
		return vhci->feature;
	case VHCI_INTMASK:
		return vhci->intmask;
	case VHCI_RING_BASE:
		return vhci->ring_base;
	case VHCI_RING_SIZE:
		return vhci->ring_size_power2;
	case VHCI_DONE:
		return vhci->xfer_done(vhci);
	}

	return 0;
}

/*
 * vhci_io_port_write - handle port register write
 */
static void vhci_io_port_write(VhciState *vhci,unsigned int port,
		unsigned int reg,uint32_t value)
{
	switch(reg){
	case VHCI_PORT_STATUS_:
		vhci->port_status[port] &= ~value;
		break;
	case VHCI_PORT_CMD_:
		vhci->port_command(vhci,port,value);
		break;
	}
}

/*
 * vhci_io_write - handle register write
 */
void vhci_io_write(VhciState *vhci, hwaddr offset, uint64_t value,
	unsigned size)
{
	unsigned int port;
	unsigned int reg;

	switch(offset){
	case VHCI_PORT_BASE ... VHCI_PORT_LIMIT(NR_PORTS):
		/*
		 *  the register be writen is a port register,
		 *  find the port nr and port register offset
		 */
		port = (offset - VHCI_PORT_BASE)/VHCI_PORT_SIZE;
		reg = offset - (port * VHCI_PORT_SIZE) - VHCI_PORT_BASE;

		return vhci_io_port_write(vhci,port,reg,(uint32_t)value);
	case VHCI_CMD:
		vhci_command(vhci,(uint32_t)value,vhci->cmd_param);
		break;
	case VHCI_STATUS:
		vhci->status &= ~(uint32_t)value;
		vhci_update_irq(vhci);
		break;
	case VHCI_INTMASK:
		vhci->intmask = (uint32_t)value;
		vhci_update_irq(vhci);
		break;
	case VHCI_RING_BASE:
		vhci->ring_base = (uint32_t)value;
		break;
	case VHCI_RING_SIZE:
		vhci->ring_size_power2 = (uint32_t)value;
		vhci->ring_qemu_size = (uint32_t)(sizeof(union vhci_ringnode) <<value);
		break;
	case VHCI_PARAM:
		vhci->cmd_param = (uint32_t)value;
		break;

	}
}



