/*
 * at91sam9_soc.h
 *
 *  Created on: Mar 4, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_ARM_AT91SAM9_SOC_H_
#define INCLUDE_HW_ARM_AT91SAM9_SOC_H_

#include "cpu.h"
#include "hw/arm/arm.h"
#include "hw/char/at91sam9_uart.h"
#include "hw/intc/at91sam9_aic.h"
#include "hw/dma/at91sam9_dma.h"
#include "hw/gpio/at91sam9_pio.h"
#include "hw/misc/at91sam9_pmc.h"
#include "hw/misc/at91sam9_sckc.h"
#include "hw/net/at91sam9_emac.h"
#include "hw/timer/at91sam9_pit.h"
#include "hw/sd/at91sam9_hsmci.h"

#define TYPE_AT91SAM9_SOC "at91sam9-soc"
#define AT91SAM9_SOC(obj)                                                      \
	OBJECT_CHECK(At91sam9SocState, (obj), TYPE_AT91SAM9_SOC)

typedef struct At91sam9SocState {
	/*< private >*/
	DeviceState parent;
	/* <public> */
	ARMCPU cpu;

	At91sam9UartState dbgu;
	At91sam9AicState aic;
	At91sam9DmacState dmac;
	At91sam9PmcState pmc;
	At91sam9SckcState sckc;
	At91sam9PitState pit;
	At91sam9EmacState emac;
	At91sam9HsmciState hsmci;
	At91sam9PioState pio[4];
} At91sam9SocState;

#endif /* INCLUDE_HW_ARM_AT91SAM9_SOC_H_ */
