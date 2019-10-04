/*
 * at91sam9_uart.c
 *
 *  Created on: Apr 12, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/irq.h"
#include "hw/char/at91sam9_uart.h"

#define UART_REGION_SIZE 0x100

#define UART_CR 0x00
#define UART_MR 0x04
#define UART_IER 0x08
#define UART_IDR 0x0c
#define UART_IMR 0x10
#define UART_SR 0x14
#define UART_RHR 0x18
#define UART_THR 0x1c
#define UART_BRGR 0x20

/*
 * Uart Control register
 */
#define UART_CR_RSTRX (1 << 2)
#define UART_CR_RSTTX (1 << 3)
#define UART_CR_RXEN (1 << 4)
#define UART_CR_RXDIS (1 << 5)
#define UART_CR_TXEN (1 << 6)
#define UART_CR_TXDIS (1 << 7)
#define UART_CR_RSTSTA (1 << 8)

/*
 * Uart Mode register
 */
#define UART_MR_PAR_SHIFT 9
#define UART_MR_PAR_MASK 0x7
#define UART_MR_PAR_EVEN 0x0
#define UART_MR_PAR_ODD 0x1
#define UART_MR_PAR_SPACE 0x2
#define UART_MR_PAR_MARK 0x3
#define UART_MR_PAR_NONE 0x4
#define UART_MR_CHMODE_SHIFT 14
#define UART_MR_CHMODE_MASK 0x3
#define UART_MR_CHMODE_NORM 0x0
#define UART_MR_CHMODE_AUTO 0x1
#define UART_MR_CHMODE_LOCLOOP 0x2
#define UART_MR_CHMODE_REMLOOP 0x3

/*
 * Uart EVENT bit
 */
#define UART_EVENT_RXRDY (1 << 0)
#define UART_EVENT_TXRDY (1 << 1)
#define UART_EVENT_OVRE (1 << 5)
#define UART_EVENT_FRAME (1 << 6)
#define UART_EVENT_PARE (1 << 7)
#define UART_EVENT_TXEMPTY (1 << 9)
#define UART_EVENT_COMMTX (1 << 30)
#define UART_EVENT_COMMRX (1 << 31)

/*
 * Uart Baud Rate Generator register
 */
#define UART_BRGR_CD_SHIFT 0
#define UART_BRGR_CD_MASK 0xffff

static void at91sam9_uart_CR_write(At91sam9UartState *us, uint32_t value)
{
	if (value & UART_CR_RSTSTA) {
		us->reg_sr &=
		    ~(UART_EVENT_FRAME | UART_EVENT_OVRE | UART_EVENT_PARE);
	}

	if (value & (UART_CR_RSTRX | UART_CR_RXDIS)) {
		us->rx_enabled = false;
		us->reg_sr &= ~(UART_EVENT_RXRDY);
	}
	if (value & (UART_CR_RSTTX | UART_CR_TXDIS)) {
		us->tx_enabled = false;
		us->reg_sr &= ~(UART_EVENT_TXRDY);
		us->reg_sr &= ~(UART_EVENT_TXEMPTY);
	}

	if (value & UART_CR_RXEN) {
		us->rx_enabled = true;
	}

	if (value & UART_CR_TXEN) {
		us->tx_enabled = true;
		us->reg_sr |= (UART_EVENT_TXRDY | UART_EVENT_TXEMPTY);

		if (us->reg_imr & (UART_EVENT_TXRDY | UART_EVENT_TXEMPTY))
			qemu_irq_raise(us->irq);
	}
}

static uint64_t at91sam9_uart_read(void *opaque, hwaddr offset, unsigned size)
{
	At91sam9UartState *us = AT91SAM9_UART(opaque);
	uint64_t value = 0;

	switch (offset) {
	case UART_SR:
		value = us->reg_sr;
		break;
	case UART_RHR:
		value = us->reg_rhr;
		us->reg_sr &= ~UART_EVENT_RXRDY;
		break;
	case UART_IMR:
		value = us->reg_imr;
		break;
	case UART_MR:
		value = us->reg_mr;
		break;
	}

	return value;
}

static void at91sam9_uart_write(void *opaque, hwaddr offset, uint64_t value,
				unsigned size)
{
	At91sam9UartState *us = AT91SAM9_UART(opaque);
	uint8_t byte;

	switch (offset) {
	case UART_THR:
		if (!us->tx_enabled) {
			return;
		}
		if (qemu_chr_fe_get_driver(&us->chr)) {
			byte = (uint8_t)value;
			qemu_chr_fe_write_all(&us->chr, &byte, 1);
			us->reg_sr |= UART_EVENT_TXRDY | UART_EVENT_TXEMPTY;
			if (us->reg_imr &
			    (UART_EVENT_TXRDY | UART_EVENT_TXEMPTY)) {
				qemu_irq_raise(us->irq);
			}
		}
		break;
	case UART_IER:
		us->reg_imr |= value;
		if ((us->reg_imr & (UART_EVENT_TXRDY | UART_EVENT_TXEMPTY)) &&
		    us->tx_enabled)
			qemu_irq_raise(us->irq);
		break;
	case UART_IDR:
		us->reg_imr &= ~value;
		break;
	case UART_MR:
		us->reg_mr = value;
		break;
	case UART_CR:
		at91sam9_uart_CR_write(us, value);
		break;
	}
}

static int at91sam9_uart_can_receive(void *opaque)
{
	At91sam9UartState *us = AT91SAM9_UART(opaque);

	if (!us->rx_enabled) {
		return 0;
	}

	return 1;
}

static void at91sam9_uart_receive(void *opaque, const uint8_t *buf, int size)
{
	At91sam9UartState *us = AT91SAM9_UART(opaque);

	us->reg_rhr = buf[0];
	us->reg_sr |= UART_EVENT_RXRDY;

	if (us->reg_imr & UART_EVENT_RXRDY) {
		qemu_irq_raise(us->irq);
	}
}

static void at91sam9_uart_reset(DeviceState *opaque)
{
	At91sam9UartState *us = AT91SAM9_UART(opaque);

	us->reg_sr |= UART_EVENT_TXRDY | UART_EVENT_TXEMPTY;
	us->tx_enabled = true;
	us->rx_enabled = true;
}

static const MemoryRegionOps at91sam9_uart_ops = {
    .read = at91sam9_uart_read,
    .write = at91sam9_uart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property at91sam9_uart_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9UartState,addr,0),
	DEFINE_PROP_CHR("chardev", At91sam9UartState, chr),
	DEFINE_PROP_END_OF_LIST(),
};

static void at91sam9_uart_init(Object *obj)
{
	At91sam9UartState *us = AT91SAM9_UART(obj);

	memory_region_init_io(&us->iomem, OBJECT(us), &at91sam9_uart_ops, us,
			      TYPE_AT91SAM9_UART, UART_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(us), &us->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(us), &(us->irq));

}

static void at91sam9_uart_realize(DeviceState *dev, Error **errp)
{
	At91sam9UartState *us = AT91SAM9_UART(dev);


	sysbus_mmio_map(SYS_BUS_DEVICE(us), 0, us->addr);

	qemu_chr_fe_set_handlers(&us->chr, at91sam9_uart_can_receive,
				 at91sam9_uart_receive, NULL, NULL, us, NULL,
				 true);
}

static void at91sam9_uart_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = at91sam9_uart_realize;
	dc->reset = at91sam9_uart_reset;
	dc->props = at91sam9_uart_properties;
}

static const TypeInfo at91sam9_uart_type_info = {
    .name = TYPE_AT91SAM9_UART,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(At91sam9UartState),
    .instance_init = at91sam9_uart_init,
    .class_init = at91sam9_uart_class_init,
};

static void at91sam9_uart_register_types(void)
{
	type_register_static(&at91sam9_uart_type_info);
}

type_init(at91sam9_uart_register_types)
