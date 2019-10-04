/*
 * at91sam9_pit.h
 *
 *  Created on: Apr 12, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_TIMER_AT91SAM9_PIT_H_
#define INCLUDE_HW_TIMER_AT91SAM9_PIT_H_
#include "hw/irq.h"
#include "qemu/typedefs.h"

#define TYPE_AT91SAM9_PIT "at91sam9-pit"
#define AT91SAM9_PIT(obj)                                                      \
	OBJECT_CHECK(At91sam9PitState, (obj), TYPE_AT91SAM9_PIT)

typedef struct At91sam9PitState {
	/*< private >*/
	SysBusDevice parent_obj;
	/*< public >*/
	MemoryRegion iomem;
	QEMUTimer timer;
	qemu_irq irq;
	uint32_t addr;
	unsigned long interval;
	uint32_t reg_pit_mr;
	uint32_t reg_pit_sr;
	uint32_t reg_pit_pivr;
	uint32_t reg_pit_piir;
} At91sam9PitState;

#endif /* INCLUDE_HW_TIMER_AT91SAM9_PIT_H_ */
