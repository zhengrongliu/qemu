/*
 * am335x_uart.c
 *
 *  Created on: Feb 5, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "chardev/char-fe.h"
#include "hw/irq.h"
#include "hw/sysbus.h"

#define TYPE_AM335X_UART "am335x-uart"

#define AM335X_UART(obj)                                                     \
	OBJECT_CHECK(Am335xUartState, (obj), TYPE_AM335X_UART)

typedef struct Am335xUartState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	CharBackend chr;
	MemoryRegion iomem;
	qemu_irq irq;
	uint32_t addr;

	uint32_t reg_lsr;
	uint32_t reg_rhr;
	uint32_t reg_ier;
	uint32_t reg_iir;
} Am335xUartState;

#define UART_REGION_SIZE 0x100

#define UART_THR	0x0
#define UART_RHR	0x0
#define UART_IER	0x4
#define UART_IIR	0x8
#define UART_LSR	0x14


/*
 * UART_LSR_UART register
 */
#define UART_LSR_RXFIFOE	BIT(0)
#define UART_LSR_RXOE		BIT(1)
#define UART_LSR_RXPE		BIT(2)
#define UART_LSR_RXFE		BIT(3)
#define UART_LSR_RXBI		BIT(4)
#define UART_LSR_TXFIFOE	BIT(5)
#define UART_LSR_TXSRE		BIT(6)
#define UART_LSR_RXFIFOSTS	BIT(7)

/*
 * UART_IER_UART register
 */
#define UART_IER_RHRIT		BIT(0)
#define UART_IER_THRIT		BIT(1)
#define UART_IER_LINESTSIT	BIT(2)
#define UART_IER_MODEMSTSIT	BIT(3)

/*
 * UART_IIR_UART register
 */
#define UART_IIR_IT_PENDING		BIT(0)
#define UART_IIR_IT_TYPE_SHIFT		1
#define UART_IIR_IT_TYPE_WIDTH		5
#define UART_IIR_IT_TYPE_THR		1
#define UART_IIR_IT_TYPE_RHR		2


static void am335x_uart_irq_update(Am335xUartState *us)
{
	us->reg_iir = UART_IIR_IT_PENDING;

	if((us->reg_lsr & UART_LSR_TXFIFOE) &&
			(us->reg_ier & UART_IER_THRIT)){
		us->reg_iir = UART_IIR_IT_TYPE_THR<<UART_IIR_IT_TYPE_SHIFT;
	}

	if((us->reg_lsr & UART_LSR_RXFIFOE) &&
			(us->reg_ier & UART_IER_RHRIT)){
		us->reg_iir = UART_IIR_IT_TYPE_RHR<<UART_IIR_IT_TYPE_SHIFT;
	}



	if(us->reg_iir != UART_IIR_IT_PENDING)
		qemu_irq_raise(us->irq);
	else
		qemu_irq_lower(us->irq);


}

static uint64_t am335x_uart_read(void *opaque, hwaddr offset, unsigned size)
{
	Am335xUartState *us = AM335X_UART(opaque);
	uint64_t value = 0;

	switch(offset){
	case UART_RHR:
		value = us->reg_rhr;
		us->reg_lsr &= ~UART_LSR_RXFIFOE;
		am335x_uart_irq_update(us);
		break;
	case UART_LSR:
		value = us->reg_lsr;
		break;
	case UART_IER:
		value = us->reg_ier;
		break;
	case UART_IIR:
		value = us->reg_iir;
		break;

	}

	return value;
}

static void am335x_uart_write(void *opaque, hwaddr offset, uint64_t value,
				unsigned size)
{
	Am335xUartState *us = AM335X_UART(opaque);
	uint8_t byte;

	switch(offset){
	case UART_THR:
		if (qemu_chr_fe_get_driver(&us->chr)) {
			byte = (uint8_t)value;
			qemu_chr_fe_write_all(&us->chr, &byte, 1);
			us->reg_lsr |= UART_LSR_TXSRE|UART_LSR_TXFIFOE;
			am335x_uart_irq_update(us);
		}
		break;
	case UART_IER:
		us->reg_ier = value;
		am335x_uart_irq_update(us);
		break;
	}
}

static int am335x_uart_can_receive(void *opaque)
{
	return 1;
}

static void am335x_uart_receive(void *opaque, const uint8_t *buf, int size)
{
	Am335xUartState *us = AM335X_UART(opaque);

	us->reg_rhr = buf[0];

	us->reg_lsr |= UART_LSR_RXFIFOE;

	am335x_uart_irq_update(us);
}

static void am335x_uart_reset(DeviceState *opaque)
{
	Am335xUartState *us = AM335X_UART(opaque);


	us->reg_lsr |= UART_LSR_TXSRE|UART_LSR_TXFIFOE;
}

static const MemoryRegionOps am335x_uart_ops = {
	.read = am335x_uart_read,
	.write = am335x_uart_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static Property am335x_uart_properties[] = {
	DEFINE_PROP_CHR("chardev", Am335xUartState, chr),
	DEFINE_PROP_END_OF_LIST(),
};

static void am335x_uart_init(Object *obj)
{
	Am335xUartState *us = AM335X_UART(obj);

	memory_region_init_io(&us->iomem, OBJECT(us), &am335x_uart_ops, us,
			      TYPE_AM335X_UART, UART_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(us), &us->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(us), &(us->irq));

}

static void am335x_uart_realize(DeviceState *dev, Error **errp)
{
	Am335xUartState *us = AM335X_UART(dev);


	qemu_chr_fe_set_handlers(&us->chr, am335x_uart_can_receive,
				 am335x_uart_receive, NULL, NULL, us, NULL,
				 true);
}

static void am335x_uart_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = am335x_uart_realize;
	dc->reset = am335x_uart_reset;
	dc->props = am335x_uart_properties;
}

static const TypeInfo am335x_uart_type_info = {
	.name = TYPE_AM335X_UART,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(Am335xUartState),
	.instance_init = am335x_uart_init,
	.class_init = am335x_uart_class_init,
};

static void am335x_uart_register_types(void)
{
	type_register_static(&am335x_uart_type_info);
}

type_init(am335x_uart_register_types)
