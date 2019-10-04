/*
 * s3c2416_timer.h
 *
 *  Created on: Dec 28, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_TIMER_S3C2416_TIMER_H_
#define INCLUDE_HW_TIMER_S3C2416_TIMER_H_
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "qemu/typedefs.h"

#define TYPE_S3C2416_TIMER "s3c2416-timer"
#define S3C2416_TIMER(obj)                                                     \
	OBJECT_CHECK(S3C2416TimerState, (obj), TYPE_S3C2416_TIMER)

#define S3C2416_NR_TIMERS 5

struct S3C2416TimerState;
typedef struct S3C2416TimerNode {
	qemu_irq irq;
	uint32_t freq;
	uint32_t prescaler;
	uint32_t divider;
	uint32_t compare_base;
	uint32_t count_base;
	uint64_t expire_time;
	uint64_t expire_delta;
	int auto_reload;
	int enable;
	QEMUTimer timer;
	struct S3C2416TimerState *timer_ctrl;
	int id;
} S3C2416TimerNode;

typedef struct S3C2416TimerState {
	/*< private >*/
	SysBusDevice parent_obj;

	/*< public >*/
	MemoryRegion iomem;
	S3C2416TimerNode timer_nodes[S3C2416_NR_TIMERS];
	int nr_irqs;
	uint32_t reg_cfg0;
	uint32_t reg_cfg1;
	uint32_t reg_con;
	uint64_t clock_freq;
	bool enabled;

} S3C2416TimerState;

#endif /* INCLUDE_HW_TIMER_S3C2416_TIMER_H_ */
