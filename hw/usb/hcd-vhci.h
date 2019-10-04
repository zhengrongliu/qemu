/*
 * hcd-vhci.h
 *
 *  Created on: Feb 16, 2019
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef HW_USB_HCD_VHCI_H_
#define HW_USB_HCD_VHCI_H_

#include "hw/usb.h"
#include "hw/pci/pci.h"
#include "hw/sysbus.h"
#include "qemu/queue.h"
#include "qemu/error-report.h"
#include "chardev/char-fe.h"

#include "vhci.h"

#define VHCI_IO_REGION_SIZE 0x1000

#define NR_PORTS 31

#define TYPE_VHCI_SYSBUS "vhci-sysbus"
#define VHCI_SYSBUS(obj)  \
	OBJECT_CHECK(VhciSysbusState, (obj), TYPE_VHCI_SYSBUS)

#define TYPE_VHCI_PCI "vhci-pci"
#define VHCI_PCI(obj)  \
	OBJECT_CHECK(VhciPCIState, (obj), TYPE_VHCI_PCI)

#define VHCI_ASYNC_NONE 	0
#define VHCI_ASYNC_INFLIGHT	1
#define VHCI_ASYNC_FINISHED	2

#define vhci_err(dev,...) \
    do { \
        if (dev->debug >= vhci_dbglvl_error) { \
            error_report( __VA_ARGS__); \
        } \
    } while (0)
#define vhci_info(dev,...) \
    do { \
        if (dev->debug >= vhci_dbglvl_info) { \
            info_report(__VA_ARGS__); \
        } \
    } while (0)
#define vhci_warn(dev,...) \
    do { \
        if (dev->debug >= vhci_dbglvl_warn) { \
            warn_report( __VA_ARGS__); \
        } \
    } while (0)
#define vhci_dbg(dev,...) \
    do { \
        if (dev->debug >= vhci_dbglvl_debug) { \
            info_report(__VA_ARGS__); \
        } \
    } while (0)


enum vhci_dbglvl_level{
	vhci_dbglvl_error,
	vhci_dbglvl_info,
	vhci_dbglvl_warn,
	vhci_dbglvl_debug,
};


struct vhci_transfer;
struct vhci_packet;

typedef struct VhciState VhciState;

struct VhciState{
	MemoryRegion iomem;
	qemu_irq irq;
	AddressSpace *as;
	DeviceState *device;
	QEMUBH *async_bh;
	char *mode;

	int debug;

	USBBus bus;
	USBPort ports[NR_PORTS];
	uint32_t port_status[NR_PORTS];

	void *ring_qemu_base;
	hwaddr ring_qemu_size;

	uint32_t status;
	uint32_t feature;
	uint32_t intmask;

	uint32_t cmd_param;

	hwaddr ring_base;
	uint32_t ring_size;
	uint32_t ring_size_power2;



	QTAILQ_HEAD(,vhci_transfer) finished;
	QTAILQ_HEAD(,vhci_transfer) inflight;

	void (*hc_reset)(VhciState *vhci);
	int (*xfer_submit)(VhciState *vhci,int index);
	int (*xfer_cancel)(VhciState *vhci,int index);
	int (*xfer_done)(VhciState *vhci);
	int (*port_command)(VhciState *vhci,unsigned int port,unsigned int cmd);

};

typedef struct VhciSysbusState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	VhciState vhci;

}VhciSysbusState;

typedef struct VhciPCIState {
	/* <private> */
	PCIDevice parent_obj;

	/* <public> */
	VhciState vhci;

}VhciPCIState;

struct vhci_transfer{
	int head;
	int async_status;

	QTAILQ_ENTRY(vhci_transfer) queue;
	QTAILQ_HEAD(,vhci_packet) packets;
};

struct vhci_packet{
	struct vhci_transfer *transfer;
	int index;
	int async_status;

	QTAILQ_ENTRY(vhci_packet) queue;
	USBPacket packet;
	QEMUSGList sgl;
};







uint64_t vhci_io_read(VhciState *vhci, hwaddr offset, unsigned size);
void vhci_io_write(VhciState *vhci, hwaddr offset, uint64_t value,
	unsigned size);
void vhci_qemu_init(VhciState *s, DeviceState *dev, Error **errp);
void vhci_vhci_init(VhciState *s, DeviceState *dev, Error **errp);
void vhci_update_irq(VhciState *vhci);





#endif /* HW_USB_HCD_VHCI_H_ */
