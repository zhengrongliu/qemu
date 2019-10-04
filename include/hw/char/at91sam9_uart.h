/*
 * at91sam9_uart.h
 *
 *  Created on: Dec 22, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_CHAR_AT91SAM9_UART_H_
#define INCLUDE_HW_CHAR_AT91SAM9_UART_H_
#include "chardev/char-fe.h"
#include "hw/irq.h"
#include "hw/sysbus.h"

#define TYPE_AT91SAM9_UART "at91sam9-uart"

#define AT91SAM9_UART(obj)                                                     \
	OBJECT_CHECK(At91sam9UartState, (obj), TYPE_AT91SAM9_UART)

typedef struct At91sam9UartState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	CharBackend chr;
	MemoryRegion iomem;
	qemu_irq irq;
	uint32_t addr;

	uint32_t reg_sr;
	uint32_t reg_cr;
	uint32_t reg_mr;
	uint32_t reg_imr;
	uint32_t reg_rhr;
	uint32_t reg_brgr;

	bool rx_enabled;
	bool tx_enabled;

} At91sam9UartState;

#endif /* INCLUDE_HW_ARM_AT91SAM9_USART_H_ */
