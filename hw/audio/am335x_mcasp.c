/*
 * am335x_mcasp.c
 *
 *  Created on: Mar 30, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/audio/soundhw.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "audio/audio.h"

#define TYPE_AM335X_MCASP "am335x-mcasp"

#define AM335X_MCASP(obj)                                                     \
	OBJECT_CHECK(Am335xMcaspState, (obj), TYPE_AM335X_MCASP)

typedef struct Am335xMcaspState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	MemoryRegion iomem;
	qemu_irq tx_irq;
	qemu_irq rx_irq;
	qemu_irq tx_dma;
	qemu_irq rx_dma;
	uint32_t addr;
	QEMUSoundCard card;
} Am335xMcaspState;




#define MCASP_REGION_SIZE 0x2000

/* global reg */
#define MCASP_REV		0x0
#define MCASP_SYSCONFIG		0x4
#define MCASP_PFUNC		0x10
#define MCASP_PDIR		0x14
#define MCASP_PDOUT		0x18
#define MCASP_PDIN		0x1c
#define MCASP_PDCLR		0x20
#define MCASP_GBLCTL		0x44
#define MCASP_AMUTE		0x48
#define MCASP_DLBCTL		0x4c
#define MCASP_DITCTL		0x50
/* receive reg */
#define MCASP_RGBLCTL		0x60
#define MCASP_RMASK		0x64
#define MCASP_RFMT		0x68
#define MCASP_AFSRCTL		0x6c
#define MCASP_ACLKRCTL		0x70
#define MCASP_AHCLKRCTL		0x74
#define MCASP_RTDM		0x78
#define MCASP_PINTCTL		0x7c
#define MCASP_RSTAT		0x80
#define MCASP_RSLOT		0x84
#define MCASP_RCLKCHK		0x88
#define MCASP_REVTCTL		0x8c
/* xfer reg */
#define MCASP_XGBLCTL		0xa0
#define MCASP_XMASK		0xa4
#define MCASP_XFMT		0xa8
#define MCASP_AFSXCTL		0xac
#define MCASP_ACLKXCTL		0xb0
#define MCASP_AHCLKXCTL		0xb4
#define MCASP_XTDM		0xb8
#define MCASP_XINTCTL		0xbc
#define MCASP_XSTAT		0xc0
#define MCASP_XSLOT		0xc4
#define MCASP_XCLKCHK		0xc8
#define MCASP_XEVTCTL		0xcc

#define MCASP_DITCSRA(x)	(0x100+(x)*0x4)
#define MCASP_DITCSRB(x)	(0x118+(x)*0x4)
#define MCASP_DITUDRA(x)	(0x130+(x)*0x4)
#define MCASP_DITUDRB(x)	(0x148+(x)*0x4)
#define MCASP_SRCTL(x)		(0x180+(x)*0x4)
#define MCASP_XBUF(x)		(0x200+(x)*0x4)
#define MCASP_RBUF(x)		(0x280+(x)*0x4)

#define MCASP_WFIFOCTL		0x1000
#define MCASP_WFIFOSTS		0x1004
#define MCASP_RFIFOCTL		0x1008
#define MCASP_RFIFOSTS		0x100c




static uint64_t am335x_mcasp_read(void *opaque, hwaddr offset, unsigned size)
{
	return 0;
}

static void am335x_mcasp_write(void *opaque, hwaddr offset, uint64_t value,
				unsigned size)
{
}


static void am335x_mcasp_reset(DeviceState *opaque)
{
}

static const MemoryRegionOps am335x_mcasp_ops = {
	.read = am335x_mcasp_read,
	.write = am335x_mcasp_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static Property am335x_mcasp_properties[] = {
	DEFINE_PROP_UINT32("addr",Am335xMcaspState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void am335x_mcasp_init(Object *obj)
{
	Am335xMcaspState *us = AM335X_MCASP(obj);

	memory_region_init_io(&us->iomem, OBJECT(us), &am335x_mcasp_ops, us,
			      TYPE_AM335X_MCASP, MCASP_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(us), &us->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(us), &(us->tx_irq));
	sysbus_init_irq(SYS_BUS_DEVICE(us), &(us->rx_irq));
	sysbus_init_irq(SYS_BUS_DEVICE(us), &(us->tx_dma));
	sysbus_init_irq(SYS_BUS_DEVICE(us), &(us->rx_dma));


}

static void am335x_mcasp_realize(DeviceState *dev, Error **errp)
{
	Am335xMcaspState *us = AM335X_MCASP(dev);


	sysbus_mmio_map(SYS_BUS_DEVICE(us), 0, us->addr);

}

static void am335x_mcasp_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = am335x_mcasp_realize;
	dc->reset = am335x_mcasp_reset;
	dc->props = am335x_mcasp_properties;
}

static const TypeInfo am335x_mcasp_type_info = {
	.name = TYPE_AM335X_MCASP,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(Am335xMcaspState),
	.instance_init = am335x_mcasp_init,
	.class_init = am335x_mcasp_class_init,
};

static void am335x_mcasp_register_types(void)
{
	type_register_static(&am335x_mcasp_type_info);
}

type_init(am335x_mcasp_register_types)


