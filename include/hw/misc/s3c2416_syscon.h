/*
 * s3c2416_syscon.h
 *
 *  Created on: Jan 6, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_MISC_S3C2416_SYSCON_H_
#define INCLUDE_HW_MISC_S3C2416_SYSCON_H_

#define TYPE_S3C2416_SYSCON "s3c2416-syscon"
#define S3C2416_SYSCON(obj)                                                    \
	OBJECT_CHECK(S3C2416SysconState, (obj), TYPE_S3C2416_SYSCON)

typedef struct S3C2416SysconState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;

	uint64_t clock_freq;
	uint64_t crystal_freq;
	uint64_t osc_freq;

	uint32_t regs[64];

	void *timer_device;

} S3C2416SysconState;

#endif /* INCLUDE_HW_MISC_S3C2416_SYSCON_H_ */
