/*
 * am335x_timer.c
 *
 *  Created on: Feb 5, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"


#define TYPE_AM335X_TIMER "am335x-timer"

#define AM335X_TIMER(obj)  \
	OBJECT_CHECK(Am335xTimerState, (obj), TYPE_AM335X_TIMER)

typedef struct Am335xTimerState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	MemoryRegion iomem;
	QEMUTimer timer;
	qemu_irq irq;

	unsigned long freq;
	unsigned long last_start_time;
	unsigned long reload_interval;
	uint32_t reg_tclr;
	uint32_t reg_tcrr;
	uint32_t reg_irq_status;
	uint32_t reg_irq_status_raw;
	uint32_t reg_irq_enable;
	uint32_t reg_tiocp_cfg ;
	uint32_t reg_tldr;

}Am335xTimerState;

#define TIMER_REGION_SIZE 0x100

#define TIMER_TIDR		0x0
#define TIMER_TIOCP_CFG		0x10
#define TIMER_IRQ_EOI		0x20
#define TIMER_IRQSTATUS_RAW	0x24
#define TIMER_IRQSTATUS		0x28
#define TIMER_IRQENABLE_SET	0x2c
#define TIMER_IRQENABLE_CLR	0x30
#define TIMER_IRQWAKEEN		0x34
#define TIMER_TCLR		0x38
#define TIMER_TCRR		0x3c
#define TIMER_TLDR		0x40
#define TIMER_TTGR		0x44
#define TIMER_TWPS		0x48
#define TIMER_TMAR		0x4c
#define TIMER_TCAR1		0x50
#define TIMER_TSICR		0x54
#define TIMER_TCAR2		0x58

/*
 * TIMER_TIDR register
 */
#define TIMER_TIDR_Y_MINOR_OFFSET	0
#define TIMER_TIDR_Y_MINOR_WIDTH	6

/*
 * TIMER_TIOCP_CFG register
 */
#define TIMER_TIOCP_CFG_SOFTRESET	BIT(0)
#define TIMER_TIOCP_CFG_EMUFREE		BIT(1)
#define TIMER_TIOCP_CFG_IDLEMODE_OFFSET 2

/*
 * TIMER_IRQ_EOI register
 */
#define TIMER_IRQ_EOI_DMAEVENT_ACK	BIT(0)

/*
 * TIMER_IRQSTATUS register
 */
#define TIMER_IRQSTATUS_MAT		BIT(0)
#define TIMER_IRQSTATUS_OVF		BIT(1)
#define TIMER_IRQSTATUS_TCAR		BIT(2)

/*
 * TIMER_TCLR register
 */

#define TIMER_TCLR_START		BIT(0)
#define TIMER_TCLR_AUTORELOAD		BIT(1)
#define TIMER_TCLR_ONESHOT		0
#define TIMER_TCLR_PTV			BIT(2)
#define TIMER_TCLR_PRE			BIT(3)
#define TIMER_TCLR_CE			BIT(4)
#define TIMER_TCLR_SCPWM		BIT(5)
#define TIMER_TCLR_TCM_OFFSET		8
#define TIMER_TCLR_TCM_NOCAP		(0<<TIMER_TCLR_TCM_OFFSET)
#define TIMER_TCLR_TCM_RISING_EDGE	(1<<TIMER_TCLR_TCM_OFFSET)
#define TIMER_TCLR_TCM_FALLING_EDGE	(2<<TIMER_TCLR_TCM_OFFSET)
#define TIMER_TCLR_TCM_BOTH_EDGE	(3<<TIMER_TCLR_TCM_OFFSET)
#define TIMER_TCLR_TRG_OFFSET		10
#define TIMER_TCLR_TRG_NOTRG		(0<<TIMER_TCLR_TRG_OFFSET)
#define TIMER_TCLR_TRG_OVF		(1<<TIMER_TCLR_TRG_OFFSET)
#define TIMER_TCLR_TRG_OVF_MAT		(2<<TIMER_TCLR_TRG_OFFSET)
#define TIMER_TCLR_PT			BIT(12)
#define TIMER_TCLR_CAPT_MODE		BIT(13)
#define TIMER_TCLR_GPO_CFG		BIT(14)

/*
 * TIMER_TSICR register
 */
#define TIMER_TSICR_SET			BIT(1)
#define TIMER_TSICR_POSTED		BIT(2)

static void am335x_timer_reset(DeviceState *dev);

static unsigned long  am335x_timer_tick2us(Am335xTimerState *ts ,unsigned long ticks)
{
	return (ticks * 1000000ul)/ts->freq;
}

static unsigned long am335x_timer_us2tick(Am335xTimerState *ts,unsigned long us)
{
	return (us * ts->freq)/1000000ul;
}

static void am335x_timer_start(Am335xTimerState *ts,unsigned long interval)
{
	if(!(ts->reg_tclr & TIMER_TCLR_START)){
		return ;
	}



	ts->last_start_time = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL);
	timer_mod(&ts->timer,ts->last_start_time + interval);
}

static void am335x_timer_stop(Am335xTimerState *ts)
{
	if(ts->reg_tclr & TIMER_TCLR_START){
		return ;
	}

	timer_del(&ts->timer);
	ts->reg_tcrr = am335x_timer_us2tick(ts,qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) - ts->last_start_time);
}

static void am335x_timer_irq_update(Am335xTimerState *ps)
{

	ps->reg_irq_status = ps->reg_irq_status_raw & ps->reg_irq_enable;
	if(ps->reg_irq_status)
		qemu_irq_raise(ps->irq);
	else
		qemu_irq_lower(ps->irq);

}


static uint64_t am335x_timer_read(void *opaque, hwaddr addr, unsigned size)
{
	Am335xTimerState *ts = AM335X_TIMER(opaque);

	switch(addr){
	case TIMER_TIOCP_CFG:
		return ts->reg_tiocp_cfg;
	case TIMER_IRQSTATUS:
		return ts->reg_irq_status;
	case TIMER_IRQSTATUS_RAW:
		return ts->reg_irq_status_raw;
	case TIMER_IRQENABLE_SET:
	case TIMER_IRQENABLE_CLR:
		return ts->reg_irq_enable;
	case TIMER_TCLR:
		return ts->reg_tclr;
	case TIMER_TLDR:
		return ts->reg_tldr;
	}
	return 0;
}

static void am335x_timer_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	Am335xTimerState *ts = AM335X_TIMER(opaque);

	switch(addr){
	case TIMER_TIOCP_CFG:
		if(data & TIMER_TIOCP_CFG_SOFTRESET){
			am335x_timer_reset(DEVICE(ts));
		}

		ts->reg_tiocp_cfg = data & ~TIMER_TIOCP_CFG_SOFTRESET;

		break;
	case TIMER_TCLR:
		ts->reg_tclr = data;
		if(data & TIMER_TCLR_START){
			am335x_timer_start(ts,am335x_timer_tick2us(ts,~0 - ts->reg_tcrr));
		} else {
			am335x_timer_stop(ts);
		}
		break;
	case TIMER_TCRR:
		ts->reg_tcrr = data;
		am335x_timer_start(ts,am335x_timer_tick2us(ts,~0 - ts->reg_tcrr));
		break;
	case TIMER_TLDR:
		ts->reg_tldr = data;
		ts->reload_interval = am335x_timer_tick2us(ts,~0 - ts->reg_tldr);
		break;
	case TIMER_IRQENABLE_SET:
		ts->reg_irq_enable |= data;
		am335x_timer_irq_update(ts);
		break;
	case TIMER_IRQENABLE_CLR:
		ts->reg_irq_enable &= ~data;
		am335x_timer_irq_update(ts);
		break;
	case TIMER_IRQSTATUS:
		ts->reg_irq_status &= ~data;
		ts->reg_irq_status_raw &= ~data;
		am335x_timer_irq_update(ts);
		break;
	case TIMER_IRQSTATUS_RAW:
		ts->reg_irq_status_raw |= data;
		am335x_timer_irq_update(ts);
		break;
	}
}

static void am335x_timer_expire(void *opaque)
{
	Am335xTimerState *ts = AM335X_TIMER(opaque);

	if(ts->reg_tclr & TIMER_TCLR_AUTORELOAD){

		am335x_timer_start(ts,ts->reload_interval);
	}

	ts->reg_irq_status_raw |= TIMER_IRQSTATUS_OVF;
	am335x_timer_irq_update(ts);
}

static const MemoryRegionOps am335x_timer_ops = {
	.read = am335x_timer_read,
	.write = am335x_timer_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static void am335x_timer_realize(DeviceState *state, Error **err)
{
	Am335xTimerState *ts = AM335X_TIMER(state);

	timer_init_us(&ts->timer, QEMU_CLOCK_VIRTUAL, am335x_timer_expire,ts);

	ts->freq = 32000;
}

static void am335x_timer_reset(DeviceState *state)
{
	Am335xTimerState *ts = AM335X_TIMER(state);


	ts->reg_tldr = 0;
	ts->reg_tcrr = 0;
	ts->reg_tclr = 0;
	ts->reg_irq_enable = 0;
	ts->reg_irq_status = 0;
	ts->reg_irq_status_raw = 0;
	ts->reg_tiocp_cfg = 0;
}

static void am335x_timer_init(Object *obj)
{
	Am335xTimerState *ts = AM335X_TIMER(obj);

	memory_region_init_io(&ts->iomem, OBJECT(ts), &am335x_timer_ops, ts,
			      TYPE_AM335X_TIMER, TIMER_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ts), &ts->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(ts), &(ts->irq));

}

static Property am335x_timer_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};

static void am335x_timer_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = am335x_timer_realize;
	dc->reset = am335x_timer_reset;
	dc->props = am335x_timer_properties;
}

static const TypeInfo am335x_timer_type_info = {
	.name = TYPE_AM335X_TIMER,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(Am335xTimerState),
	.instance_init = am335x_timer_init,
	.class_init = am335x_timer_class_init,
};

static void am335x_timer_register_types(void)
{
	type_register_static(&am335x_timer_type_info);
}

type_init(am335x_timer_register_types)


