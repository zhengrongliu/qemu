/*
 * at91sam9_hsmci.h
 *
 *  Created on: Nov 28, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_CHAR_AT91SAM9_HSMCI_H_
#define INCLUDE_HW_CHAR_AT91SAM9_HSMCI_H_
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/sd/sd.h"

#define TYPE_AT91SAM9_HSMCI "at91sam9-hsmci"

#define AT91SAM9_HSMCI(obj)  \
	OBJECT_CHECK(At91sam9HsmciState, (obj), TYPE_AT91SAM9_HSMCI)

typedef struct At91sam9HsmciState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	MemoryRegion iomem;
	qemu_irq irq;

	uint32_t addr;

	uint32_t reg_cmdr;
	uint32_t reg_argr;
	uint32_t reg_sr;
	uint32_t reg_imr;
	uint32_t reg_blkr;
	uint32_t reg_rdr;
	uint32_t reg_rspr[4];

	unsigned int xfer_size;

	SDBus sdbus;

} At91sam9HsmciState;

#endif /* INCLUDE_HW_ARM_AT91SAM9_HSMCI_H_ */
