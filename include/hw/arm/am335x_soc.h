/*
 * am335x_soc.h
 *
 *  Created on: Mar 4, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_ARM_AM335X_SOC_H_
#define INCLUDE_HW_ARM_AM335X_SOC_H_

#include "cpu.h"
#include "hw/arm/arm.h"

#define TYPE_AM335X_SOC "am335x-soc"
#define AM335X_SOC(obj) OBJECT_CHECK(AM335XSocState, (obj), TYPE_AM335X_SOC)

typedef struct AM335XSocState {
	/*< private >*/
	DeviceState parent;
	/* <public> */
	ARMCPU cpu;
} AM335XSocState;

#endif /* INCLUDE_HW_ARM_AM335X_SOC_H_ */
