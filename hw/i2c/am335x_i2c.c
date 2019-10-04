/*
 * am335x_i2c.c
 *
 *  Created on: Apr 2, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/i2c/i2c.h"

#define TYPE_AM335X_I2C "am335x-i2c"

#define AM335X_I2C(obj)                                                     \
	OBJECT_CHECK(Am335xI2cState, (obj), TYPE_AM335X_I2C)

typedef struct Am335xI2cState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	MemoryRegion iomem;
	qemu_irq irq;
	uint32_t addr;
	/*
	 * regs
	 */
	uint32_t reg_syss;
} Am335xI2cState;


#define I2C_REGION_SIZE 0x1000


#define I2C_REVNB_LO		0x0
#define I2C_REVNB_HI		0x4
#define I2C_SYSC		010
#define I2C_IRQSTATUS_RAW	0x24
#define I2C_IRQSTATUS		0x28
#define I2C_IRQENABLE_SET	0x2c
#define I2C_IRQENABLE_CLR	0x30
#define I2C_WE			0x34
#define I2C_DMARXENABLE_SET	0x38
#define I2C_DMATXENABLE_SET	0x3c
#define I2C_DMARXENABLE_CLR	0x40
#define I2C_DMATXENABLE_CLR	0x44
#define I2C_DMARXWAKE_EN	0x48
#define I2C_DMATXWAKE_EN	0x4c
#define I2C_SYSS		0x90
#define I2C_BUF			0x94
#define I2C_CNT			0x98
#define I2C_DATA		0x9c
#define I2C_CON			0xa4
#define I2C_OA			0xa8
#define I2C_SA			0xac
#define I2C_PSC			0xb0
#define I2C_SCLL		0xb4
#define I2C_SCLH		0xb8
#define I2C_SYSTEST		0xbc
#define I2C_BUFSTAT		0xc0
#define I2C_OA1			0xc4
#define I2C_OA2			0xc8
#define I2C_OA3			0xcc
#define I2C_ACTOA		0xd0
#define I2C_SBLOCK		0xd4

/*
 * irq event
 */
#define I2C_EVENT_AL		BIT(0)
#define I2C_EVENT_NACK		BIT(1)
#define I2C_EVENT_ARDY		BIT(2)
#define I2C_EVENT_RRDY		BIT(3)
#define I2C_EVENT_XRDY		BIT(4)
#define I2C_EVENT_GC		BIT(5)
#define I2C_EVENT_STC		BIT(6)
#define I2C_EVENT_AERR		BIT(7)
#define I2C_EVENT_BF		BIT(8)
#define I2C_EVENT_AAS		BIT(9)
#define I2C_EVENT_XUDF		BIT(10)
#define I2C_EVENT_ROVR		BIT(11)
#define I2C_EVENT_BB		BIT(12)
#define I2C_EVENT_RDR		BIT(13)
#define I2C_EVENT_XDR		BIT(14)

/*
 * I2C_SYSC register
 */
#define I2C_SYSC_AUTOIDLE_ENABLE	(1<<0)
#define I2C_SYSC_AUTOIDLE_DISABLE	(0<<0)
#define I2C_SYSC_SRST_ON		(1<<1)
#define I2C_SYSC_SRST_OFF		(0<<1)
#define I2C_SYSC_ENWAKEUP_ENABLE	(1<<2)
#define I2C_SYSC_ENWAKEUP_DISABLE	(0<<2)
#define I2C_SYSC_IDLEMODE_IDLE		(0<<3)
#define I2C_SYSC_IDLEMODE_NONE		(1<<3)
#define I2C_SYSC_IDLEMODE_SMART		(2<<3)
#define I2C_SYSC_IDLEMODE_WAKEUP	(3<<3)
#define I2C_SYSC_CLKACTIVITY_BOTH_OFF	(0<<8)
#define I2C_SYSC_CLKACTIVITY_SYS_OFF	(1<<8)
#define I2C_SYSC_CLKACTIVITY_OCP_OFF	(2<<8)
#define I2C_SYSC_CLKACTIVITY_BOTH_ON	(3<<8)

/*
 * I2C_SYSS register
 */
#define I2C_SYSS_RDONE			BIT(0)

/*
 * I2C_CON register
 */
#define I2C_CON_STT			BIT(0)
#define I2C_CON_STP			BIT(1)
#define I2C_CON_XOA3			BIT(4)
#define I2C_CON_XOA2			BIT(5)
#define I2C_CON_XOA1			BIT(6)
#define I2C_CON_XOA0			BIT(7)
#define I2C_CON_SA_7BIT			(0<<8)
#define I2C_CON_SA_10BIT		(1<<8)
#define I2C_CON_TRASMIT			(1<<9)
#define I2C_CON_RECEIVE			(0<<9)
#define I2C_CON_MASTER			(1<<10)
#define I2C_CON_SLAVE			(0<<10)
#define I2C_CON_STB			BIT(11)
#define I2C_CON_OPMODE_FS		(0<<12)
#define I2C_CON_EN			BIT(15)



static void am335x_i2c_reset(DeviceState *opaque);

static uint64_t am335x_i2c_read(void *opaque, hwaddr offset, unsigned size)
{
	Am335xI2cState *state = AM335X_I2C(opaque);
	switch(offset){
	case I2C_SYSS:
		return state->reg_syss;
	}
	return 0;
}

static void am335x_i2c_write(void *opaque, hwaddr offset, uint64_t value,
				unsigned size)
{

	switch(offset){
	case I2C_SYSC:
		if(value & I2C_SYSC_SRST_ON){
			am335x_i2c_reset(opaque);
		}
		break;

	}
}


static void am335x_i2c_reset(DeviceState *opaque)
{
	Am335xI2cState *state = AM335X_I2C(opaque);

	state->reg_syss |= I2C_SYSS_RDONE;
}

static const MemoryRegionOps am335x_i2c_ops = {
	.read = am335x_i2c_read,
	.write = am335x_i2c_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static Property am335x_i2c_properties[] = {
	DEFINE_PROP_UINT32("addr",Am335xI2cState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void am335x_i2c_init(Object *obj)
{
	Am335xI2cState *us = AM335X_I2C(obj);

	memory_region_init_io(&us->iomem, OBJECT(us), &am335x_i2c_ops, us,
			      TYPE_AM335X_I2C, I2C_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(us), &us->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(us), &us->irq);


}

static void am335x_i2c_realize(DeviceState *dev, Error **errp)
{
	Am335xI2cState *us = AM335X_I2C(dev);


	sysbus_mmio_map(SYS_BUS_DEVICE(us), 0, us->addr);

}

static void am335x_i2c_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = am335x_i2c_realize;
	dc->reset = am335x_i2c_reset;
	dc->props = am335x_i2c_properties;
}

static const TypeInfo am335x_i2c_type_info = {
	.name = TYPE_AM335X_I2C,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(Am335xI2cState),
	.instance_init = am335x_i2c_init,
	.class_init = am335x_i2c_class_init,
};

static void am335x_i2c_register_types(void)
{
	type_register_static(&am335x_i2c_type_info);
}

type_init(am335x_i2c_register_types)


