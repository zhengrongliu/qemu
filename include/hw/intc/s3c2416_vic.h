/*
 * s3c2416_vic.h
 *
 *  Created on: Dec 28, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_INTC_S3C2416_VIC_H_
#define INCLUDE_HW_INTC_S3C2416_VIC_H_
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "qemu/typedefs.h"

#define HWIRQ_N_UART0 (28)
#define HWIRQ_N_UART1 (23)
#define HWIRQ_N_UART2 (15)
#define HWIRQ_N_UART3 (18)

#define HWIRQ_N_UART0_RXD 64
#define HWIRQ_N_UART0_TXD 65
#define HWIRQ_N_UART0_ERR 66
#define HWIRQ_N_UART1_RXD 67
#define HWIRQ_N_UART1_TXD 68
#define HWIRQ_N_UART1_ERR 69
#define HWIRQ_N_UART2_RXD 70
#define HWIRQ_N_UART2_TXD 71
#define HWIRQ_N_UART2_ERR 72
#define HWIRQ_N_UART3_RXD 88
#define HWIRQ_N_UART3_TXD 89
#define HWIRQ_N_UART3_ERR 90

#define HWSUBIRQ_N_UART0_RXD 0
#define HWSUBIRQ_N_UART0_TXD 1
#define HWSUBIRQ_N_UART0_ERR 2
#define HWSUBIRQ_N_UART1_RXD 3
#define HWSUBIRQ_N_UART1_TXD 4
#define HWSUBIRQ_N_UART1_ERR 5
#define HWSUBIRQ_N_UART2_RXD 6
#define HWSUBIRQ_N_UART2_TXD 7
#define HWSUBIRQ_N_UART2_ERR 8
#define HWSUBIRQ_N_UART3_RXD 24
#define HWSUBIRQ_N_UART3_TXD 25
#define HWSUBIRQ_N_UART3_ERR 26

#define TYPE_S3C2416_VIC "s3c2416-vic"
#define S3C2416_VIC(obj) OBJECT_CHECK(S3C2416VicState, (obj), TYPE_S3C2416_VIC)

typedef struct S3C2416VicState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;

	QEMUTimer timer;

	qemu_irq arm_irq;
	qemu_irq arm_fiq;
	uint32_t reg_srcpnd1;
	uint32_t reg_srcpnd2;
	uint32_t reg_intmod1;
	uint32_t reg_intmod2;
	uint32_t reg_intmsk1;
	uint32_t reg_intmsk2;
	uint32_t reg_intpnd1;
	uint32_t reg_intpnd2;

	uint32_t reg_intoffset1;
	uint32_t reg_intoffset2;

	uint32_t reg_subsrcpnd;
	uint32_t reg_intsubmsk;

	uint32_t reg_priority_mode1;
	uint32_t reg_priority_mode2;

	uint32_t reg_priority_update1;
	uint32_t reg_priority_update2;

	int arbiter_mode[13];
	int arbiter_sel[13];
	int arbiter_rotate[13];

} S3C2416VicState;

#endif /* INCLUDE_HW_INTC_S3C2416_VIC_H_ */
