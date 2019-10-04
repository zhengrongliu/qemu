/*
 * s3c2416_uart.h
 *
 *  Created on: Dec 22, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_CHAR_S3C2416_UART_H_
#define INCLUDE_HW_CHAR_S3C2416_UART_H_
#include "chardev/char-fe.h"
#include "hw/sysbus.h"

#define TYPE_S3C2416_UART "s3c2416-uart"

#define S3C2416_UART(obj)                                                      \
	OBJECT_CHECK(S3C2416UartState, (obj), TYPE_S3C2416_UART)
typedef struct S3C2416UartFifo {
	int head;
	int tail;
	uint32_t size;
	uint32_t count;
	int enable;
	uint8_t *data;
} S3C2416UartFifo;

typedef struct S3C2416UartState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	CharBackend chr;
	MemoryRegion iomem;
	uint64_t iomem_base;
	uint32_t regs[12];
	S3C2416UartFifo rx;

	qemu_irq rxd_irq;
	qemu_irq txd_irq;
	qemu_irq err_irq;

	int rx_enable;
	int tx_enable;
	int errirq_enable;

	int fifo_enable;
	int rxfifo_trigger_lvl;

} S3C2416UartState;

#endif /* INCLUDE_HW_ARM_S3C2416_UART_H_ */
