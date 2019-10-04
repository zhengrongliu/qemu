/*
 * at91sam9_dma.h
 *
 *  Created on: Dec 10, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_DMA_AT91SAM9_DMA_H_
#define INCLUDE_HW_DMA_AT91SAM9_DMA_H_

#include "hw/irq.h"
#include "hw/sysbus.h"

#define TYPE_AT91SAM9_DMAC "at91sam9-dmac"

#define AT91SAM9_DMAC(obj)  \
	OBJECT_CHECK(At91sam9DmacState, (obj), TYPE_AT91SAM9_DMAC)

#define AT91SAM9_DMAC_CHAN_MAX 8
typedef struct At91sam9DmacState At91sam9DmacState;

typedef struct At91sam9DmacChan{
	/*< private >*/
	Object parent_obj;
	/* <public> */
	MemoryRegion iomem;
	At91sam9DmacState *dmac;
	int chan_id;

	uint32_t saddr;
	uint32_t daddr;
	uint32_t dscr;
	uint32_t ctrla;
	uint32_t ctrlb;
	uint32_t cfg;
	uint32_t spip;
	uint32_t dpip;
}At91sam9DmacChan;

typedef struct At91sam9DmacState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	MemoryRegion iomem;
	qemu_irq irq;

	uint32_t addr;

	At91sam9DmacChan chan[AT91SAM9_DMAC_CHAN_MAX];

	uint32_t reg_gcfg;
	uint32_t reg_chsr;
	uint32_t reg_ebcimr;
	uint32_t reg_ebcisr;

}At91sam9DmacState;



#endif /* INCLUDE_HW_DMA_AT91SAM9_DMA_H_ */
