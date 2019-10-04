/*
 * s3c2416_uart.c
 *
 *  Created on: Dec 22, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/irq.h"
#include "hw/char/s3c2416_uart.h"

#define UART_REGION_SIZE 0x100

#define ULCON 0x0000    /* Line Control             */
#define UCON 0x0004     /* Control                  */
#define UFCON 0x0008    /* FIFO Control             */
#define UMCON 0x000C    /* Modem Control            */
#define UTRSTAT 0x0010  /* Tx/Rx Status             */
#define UERSTAT 0x0014  /* UART Error Status        */
#define UFSTAT 0x0018   /* FIFO Status              */
#define UMSTAT 0x001C   /* Modem Status             */
#define UTXH 0x0020     /* Transmit Buffer          */
#define URXH 0x0024     /* Receive Buffer           */
#define UBRDIV 0x0028   /* Baud Rate Divisor        */
#define UDIVSLOT 0x002c /* Baud Rate Divisor(dec)   */

#define UCON_RX_MODE_SHIFT 0x0
#define UCON_RX_MODE_MSK 0x3
#define UCON_RX_MODE_NORMAL 0x1
#define UCON_TX_MODE_SHIFT 0x2
#define UCON_TX_MODE_NORMAL 0x1
#define UCON_TX_MODE_MSK 0x3
#define UCON_SND_BRK_SHIFT 0x4
#define UCON_LOOP_SHIFT 0x5
#define UCON_RXERR_INT_SHIFT 0x6
#define UCON_RXTO_INT_SHIFT 0x7

#define UFCON_RX_FIFO_TRG_LVL_SHIFT 0x4
#define UFCON_RX_FIFO_TRG_LVL_MSK 0x3
#define UFCON_TX_FIFO_TRG_LVL_SHIFT 0x6
#define UFCON_TX_FIFO_TRG_LVL_MSK 0x3
#define UFCON_FIFO_ENABLE 0x1

/*
 * UART TX/RX Status
 */
#define UTRSTAT_TRANSMITTER_EMPTY 0x4
#define UTRSTAT_TX_BUFFER_EMPTY 0x2
#define UTRSTAT_RX_BUFFER_DATA_READY 0x1

/*
 * UART FIFO Status
 */
#define UFSTAT_TXFIFO_FULL_SHIFT (0xe)
#define UFSTAT_TXFIFO_COUNT_MSK (0x3f)
#define UFSTAT_TXFIFO_COUNT_SHIFT (0x08)
#define UFSTAT_RXFIFO_FULL_SHIFT (0x6)
#define UFSTAT_RXFIFO_COUNT_MSK (0x3f)
#define UFSTAT_RXFIFO_COUNT_SHIFT (0x00)

#define REG_INDEX(x) ((x) / sizeof(((S3C2416UartState *)(0))->regs[0]))

static void fifo_append(S3C2416UartFifo *fifo, uint8_t d)
{
	fifo->data[fifo->tail++] = d;
	fifo->tail = fifo->tail % fifo->size;
	fifo->count++;
}

static uint8_t fifo_retrieve(S3C2416UartFifo *fifo)
{
	uint8_t d;

	d = fifo->data[fifo->head++];
	fifo->head = fifo->head % fifo->size;
	fifo->count--;
	return d;
}

static uint64_t s3c2416_uart_read(void *opaque, hwaddr offset, unsigned size)
{
	S3C2416UartState *s = S3C2416_UART(opaque);
	int index = REG_INDEX(offset);
	uint32_t reg_value = 0;

	switch (offset) {
	case UERSTAT:
		reg_value = s->regs[index];
		s->regs[index] = 0;
		break;
	case URXH:
		if (!s->fifo_enable) {
			s->regs[REG_INDEX(UTRSTAT)] &=
			    ~UTRSTAT_RX_BUFFER_DATA_READY;
			reg_value = s->regs[REG_INDEX(URXH)];
		} else {
			if (s->rx.count == 0) {
				break;
			}

			reg_value = fifo_retrieve(&s->rx);
			if (s->rx.count == 0) {
				s->regs[REG_INDEX(UTRSTAT)] &=
				    ~UTRSTAT_RX_BUFFER_DATA_READY;
			} else {
				s->regs[REG_INDEX(UTRSTAT)] |=
				    UTRSTAT_RX_BUFFER_DATA_READY;
			}

			s->regs[REG_INDEX(UFSTAT)] &=
			    ~(1 << UFSTAT_RXFIFO_FULL_SHIFT);
			s->regs[REG_INDEX(UFSTAT)] &=
			    ~(UFSTAT_RXFIFO_COUNT_MSK
			      << UFSTAT_RXFIFO_COUNT_SHIFT);
			s->regs[REG_INDEX(UFSTAT)] |=
			    s->rx.count << UFSTAT_RXFIFO_COUNT_SHIFT;
		}
		break;
	default:
		reg_value = s->regs[index];
		break;
	}

	return reg_value;
}

static void s3c2416_uart_write(void *opaque, hwaddr offset, uint64_t value,
			       unsigned size)
{
	S3C2416UartState *s = S3C2416_UART(opaque);
	uint8_t byte;

	switch (offset) {
	case UCON:
		s->regs[offset / sizeof(s->regs[0])] = value;

		s->rx_enable = !!(((value >> UCON_RX_MODE_SHIFT) &
				   UCON_RX_MODE_MSK) == UCON_RX_MODE_NORMAL);
		s->tx_enable = !!(((value >> UCON_TX_MODE_SHIFT) &
				   UCON_TX_MODE_MSK) == UCON_TX_MODE_NORMAL);
		s->errirq_enable = !!(value & (1 << UCON_RXERR_INT_SHIFT));

		if (s->tx_enable) {
			qemu_set_irq(s->txd_irq, 1);
		}
		break;
	case UFCON:
		s->regs[offset / sizeof(s->regs[0])] = value;

		s->fifo_enable = !!(value & (1 << UFCON_FIFO_ENABLE));
		s->rxfifo_trigger_lvl = (value >> UFCON_RX_FIFO_TRG_LVL_SHIFT) &
					UFCON_RX_FIFO_TRG_LVL_MSK;
		s->rxfifo_trigger_lvl *= 16;
		break;
	case UTXH:
		if (qemu_chr_fe_get_driver(&s->chr)) {
			s->regs[REG_INDEX(UTRSTAT)] &=
			    ~(UTRSTAT_TRANSMITTER_EMPTY |
			      UTRSTAT_TX_BUFFER_EMPTY);

			byte = (uint8_t)value;
			qemu_chr_fe_write_all(&s->chr, &byte, 1);

			s->regs[REG_INDEX(UTRSTAT)] |=
			    (UTRSTAT_TRANSMITTER_EMPTY |
			     UTRSTAT_TX_BUFFER_EMPTY);

			if (s->tx_enable) {
				qemu_set_irq(s->txd_irq, 1);
			}
		}
		break;
	default:
		s->regs[offset / sizeof(s->regs[0])] = value;
		break;
	}
}

static int s3c2416_uart_can_receive(void *opaque)
{
	S3C2416UartState *s = S3C2416_UART(opaque);

	if (s->fifo_enable) {
		return s->rx.size - s->rx.count;
	}

	return 1;
}

static void s3c2416_uart_receive(void *opaque, const uint8_t *buf, int size)
{
	S3C2416UartState *s = S3C2416_UART(opaque);
	int i;

	if (!s->rx_enable) {
		return;
	}

	if (s->fifo_enable) {
		for (i = 0; i < size; i++) {
			fifo_append(&s->rx, buf[i]);
		}

		qemu_set_irq(s->rxd_irq, 1);

		if (s->rx.size == s->rx.count) {
			s->regs[REG_INDEX(UFSTAT)] |=
			    1 << UFSTAT_RXFIFO_FULL_SHIFT;
		}

		s->regs[REG_INDEX(UFSTAT)] &=
		    ~(UFSTAT_RXFIFO_COUNT_MSK << UFSTAT_RXFIFO_COUNT_SHIFT);
		s->regs[REG_INDEX(UFSTAT)] |= s->rx.count
					      << UFSTAT_RXFIFO_COUNT_SHIFT;

	} else {

		s->regs[REG_INDEX(URXH)] = buf[0];
		qemu_set_irq(s->rxd_irq, 1);
	}

	s->regs[REG_INDEX(UTRSTAT)] |= UTRSTAT_RX_BUFFER_DATA_READY;
}

static void s3c2416_uart_reset(DeviceState *opaque)
{
	S3C2416UartState *s = S3C2416_UART(opaque);

	s->regs[REG_INDEX(ULCON)] = 0x0;
	s->regs[REG_INDEX(UCON)] = 0x0;
	s->regs[REG_INDEX(UFCON)] = 0x0;
	s->regs[REG_INDEX(UMCON)] = 0x0;
	s->regs[REG_INDEX(UTRSTAT)] = 0x6;
	s->regs[REG_INDEX(UERSTAT)] = 0x0;
	s->regs[REG_INDEX(UFSTAT)] = 0x0;
	s->regs[REG_INDEX(UMSTAT)] = 0x0;
	s->regs[REG_INDEX(UBRDIV)] = 0x0;
	s->regs[REG_INDEX(UDIVSLOT)] = 0x0;

	s->tx_enable = 0;
	s->rx_enable = 0;
	s->fifo_enable = 0;

	s->rxfifo_trigger_lvl = 0;
}

static const MemoryRegionOps s3c2416_uart_ops = {
    .read = s3c2416_uart_read,
    .write = s3c2416_uart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property s3c2416_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", S3C2416UartState, chr),
    DEFINE_PROP_UINT32("rx_fifo_size", S3C2416UartState, rx.size, 64),
    DEFINE_PROP_END_OF_LIST(),
};

static void s3c2416_uart_init(Object *obj)
{
	S3C2416UartState *s = S3C2416_UART(obj);

	s->regs[REG_INDEX(UTRSTAT)] = 0x00000006;
	s->rx.data = (uint8_t *)g_malloc0(s->rx.size);
}

static void s3c2416_uart_realize(DeviceState *dev, Error **errp)
{
	S3C2416UartState *s = S3C2416_UART(dev);

	memory_region_init_io(&s->iomem, OBJECT(s), &s3c2416_uart_ops, s,
			      TYPE_S3C2416_UART, UART_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(s), &(s->rxd_irq));
	sysbus_init_irq(SYS_BUS_DEVICE(s), &(s->txd_irq));
	sysbus_init_irq(SYS_BUS_DEVICE(s), &(s->err_irq));

	qemu_chr_fe_set_handlers(&s->chr, s3c2416_uart_can_receive,
				 s3c2416_uart_receive, NULL, NULL, s, NULL,
				 true);
}

static void s3c2416_uart_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = s3c2416_uart_realize;
	dc->reset = s3c2416_uart_reset;
	dc->props = s3c2416_uart_properties;
}

static const TypeInfo s3c2416_uart_type_info = {
    .name = TYPE_S3C2416_UART,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S3C2416UartState),
    .instance_init = s3c2416_uart_init,
    .class_init = s3c2416_uart_class_init,
};

static void s3c2416_uart_register_types(void)
{
	type_register_static(&s3c2416_uart_type_info);
}

type_init(s3c2416_uart_register_types)
