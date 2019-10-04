/*
 * at91sam9_pmc.h
 *
 *  Created on: Jan 6, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_MISC_AT91SAM9_PMC_H_
#define INCLUDE_HW_MISC_AT91SAM9_PMC_H_

#define TYPE_AT91SAM9_PMC "at91sam9-pmc"
#define AT91SAM9_PMC(obj)                                                      \
	OBJECT_CHECK(At91sam9PmcState, (obj), TYPE_AT91SAM9_PMC)

typedef struct At91sam9PmcState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;
	uint32_t reg_ckgr_mor;
	uint32_t reg_ckgr_uckr;
	uint32_t reg_ckgr_pllar;
	uint32_t reg_ckgr_mcfr;
	uint32_t reg_pmc_sr;
	uint32_t reg_pmc_mckr;
	uint32_t reg_pmc_pllicpr;
	uint32_t addr;
} At91sam9PmcState;

#endif /* INCLUDE_HW_MISC_AT91SAM9_PMC_H_ */
