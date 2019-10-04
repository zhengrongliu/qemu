/*
 * at91sam9_pmc.h
 *
 *  Created on: Jan 6, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_MISC_AT91SAM9_PIO_H_
#define INCLUDE_HW_MISC_AT91SAM9_PIO_H_

#define TYPE_AT91SAM9_PIO "at91sam9-pio"
#define AT91SAM9_PIO(obj)                                                      \
	OBJECT_CHECK(At91sam9PioState, (obj), TYPE_AT91SAM9_PIO)

typedef struct At91sam9PioState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;
	uint32_t addr;
} At91sam9PioState;

#endif /* INCLUDE_HW_MISC_AT91SAM9_PMC_H_ */
