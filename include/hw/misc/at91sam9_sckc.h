/*
 * at91sam9_sckc.h
 *
 *  Created on: Jan 6, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_MISC_AT91SAM9_SCKC_H_
#define INCLUDE_HW_MISC_AT91SAM9_SCKC_H_

#define TYPE_AT91SAM9_SCKC "at91sam9-sckc"
#define AT91SAM9_SCKC(obj)                                                     \
	OBJECT_CHECK(At91sam9SckcState, (obj), TYPE_AT91SAM9_SCKC)

typedef struct At91sam9SckcState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;
	uint32_t reg_sckc_cr;
	uint32_t addr;
} At91sam9SckcState;

#endif /* INCLUDE_HW_MISC_AT91SAM9_SCKC_H_ */
