/*
 * s3c2416_soc.c
 *
 *  Created on: Dec 5, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */
#ifndef HW_ARM_S3C2416_SOC_H
#define HW_ARM_S3C2416_SOC_H

#include "hw/arm/arm.h"
#include "hw/char/s3c2416_uart.h"
#include "hw/display/s3c2416_lcd.h"
#include "hw/intc/s3c2416_vic.h"
#include "hw/misc/s3c2416_nand.h"
#include "hw/misc/s3c2416_syscon.h"
#include "hw/timer/s3c2416_timer.h"

#define TYPE_S3C2416_SOC "s3c2416-soc"
#define S3C2416_SOC(obj) OBJECT_CHECK(S3C2416SocState, (obj), TYPE_S3C2416_SOC)

#define S3C2416_NR_UARTS 4

typedef struct S3C2416SocState {
	/*< private >*/
	DeviceState parent;
	/*< public >*/

	ARMCPU cpu;
	/*
	 * peripheral devices
	 */
	S3C2416VicState vic;
	S3C2416TimerState timer;
	S3C2416UartState uarts[S3C2416_NR_UARTS];
	S3C2416SysconState syscon;
	S3C2416NandState nand;
	S3C2416LCDState lcd;
} S3C2416SocState;

#endif
