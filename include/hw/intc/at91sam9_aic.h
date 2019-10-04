/*
 * at91sam9_aic.h
 *
 *  Created on: Apr 12, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_INTC_AT91SAM9_AIC_H_
#define INCLUDE_HW_INTC_AT91SAM9_AIC_H_
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "qemu/typedefs.h"

#define NR_IRQS 32

#define TYPE_AT91SAM9_AIC "at91sam9-aic"
#define AT91SAM9_AIC(obj)                                                      \
	OBJECT_CHECK(At91sam9AicState, (obj), TYPE_AT91SAM9_AIC)

typedef struct At91sam9AicState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;
	uint32_t addr;

	qemu_irq arm_irq;
	qemu_irq arm_fiq;

	uint8_t src_type[NR_IRQS];
	uint8_t src_prior[NR_IRQS];
	uint32_t src_vec[NR_IRQS];

	uint32_t src_pending;
	uint32_t src_mask;

	uint32_t spu_vec;
	int protect_mode;
	int general_mask;

	int top;
	uint8_t src_nr_stack[8];
	uint8_t src_prior_stack[8];

} At91sam9AicState;

#endif /* INCLUDE_HW_INTC_AT91SAM9_AIC_H_ */
