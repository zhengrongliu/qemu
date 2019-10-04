/*
 * s3c2416_nand.h
 *
 *  Created on: Jan 6, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_MISC_S3C2416_NAND_H_
#define INCLUDE_HW_MISC_S3C2416_NAND_H_
#include "hw/block/nand_flash.h"

#define TYPE_S3C2416_NAND "s3c2416-nand"
#define S3C2416_NAND(obj)                                                      \
	OBJECT_CHECK(S3C2416NandState, (obj), TYPE_S3C2416_NAND)

typedef struct S3C2416NandState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;
	const MemoryRegionOps *iomem_ops;
	uint32_t regs[0x64];

	NandFlashState *flash;

} S3C2416NandState;

#endif /* INCLUDE_HW_MISC_S3C2416_NAND_H_ */
