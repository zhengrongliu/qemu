/*
 * s3c2416_timer.c
 *
 *  Created on: Dec 28, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/timer.h"
#include "qapi/visitor.h"
#include "qapi/error.h"
#include "hw/timer/s3c2416_timer.h"

#define TIMER_REGION_SIZE 0x100

#define TCFG0 (0x00)
#define TCFG1 (0x04)
#define TCON (0x08)
#define TCNTB0 (0x0c)
#define TCMPB0 (0x10)
#define TCNTO0 (0x14)
#define TCNTB1 (0x18)
#define TCMPB1 (0x1c)
#define TCNTO1 (0x20)
#define TCNTB2 (0x24)
#define TCMPB2 (0x28)
#define TCNTO2 (0x2c)
#define TCNTB3 (0x30)
#define TCMPB3 (0x34)
#define TCNTO3 (0x38)
#define TCNTB4 (0x3c)
#define TCNTO4 (0x40)

#define TCON_TIMER0_START (1 << 0)
#define TCON_TIMER0_MANUAL_UPDATE (1 << 1)
#define TCON_TIMER0_OUTPUT_INVERT (1 << 2)
#define TCON_TIMER0_AUTO_RELOAD (1 << 3)
#define TCON_TIMER0_DEAD_ZONE (1 << 4)

inline static uint32_t TCON_TIMER_START(int id)
{
	if (id == 0) {
		return 1 << id;
	}
	id++;
	return 1 << (id << 2);
}
inline static uint32_t TCON_TIMER_MANUAL_UPDATE(int id)
{
	if (id == 0) {
		return 2 << id;
	}
	id++;
	return 2 << (id << 2);
}
inline static uint32_t TCON_TIMER_OUTPUT_INVERT(int id)
{
	if (id == 0) {
		return 4 << id;
	} else if (id == 4) {
		return 0;
	}
	id++;
	return 4 << (id << 2);
}
inline static uint32_t TCON_TIMER_AUTO_RELOAD(int id)
{
	if (id == 0) {
		return 8 << id;
	} else if (id == 4) {
		return 4 << (id << 2);
	}
	id++;
	return 8 << (id << 2);
}

static uint64_t s3c2416_timer_read(void *opaque, hwaddr addr, unsigned size)
{
	S3C2416TimerState *ts = S3C2416_TIMER(opaque);
	int index;
	if (!ts->enabled) {
		return 0;
	}

	switch (addr) {
	case TCFG0:
		return ts->reg_cfg0;
	case TCFG1:
		return ts->reg_cfg1;
	case TCON:
		return ts->reg_con;
	case TCNTB0:
	case TCNTB1:
	case TCNTB2:
	case TCNTB3:
	case TCNTB4:
		index = (addr - TCNTB0) / 0x0c;
		return ts->timer_nodes[index].count_base;
	case TCMPB0:
	case TCMPB1:
	case TCMPB2:
	case TCMPB3:
		index = (addr - TCMPB0) / 0x0c;
		return ts->timer_nodes[index].compare_base;
	default:
		break;
	}
	return 0;
}

static void s3c2416_timer_update_expire(S3C2416TimerState *ts, int id)
{
	S3C2416TimerNode *node = &ts->timer_nodes[id];
	uint64_t tmp;

	if (node->compare_base < node->count_base) {
		node->expire_delta = 0;
	} else {
		tmp = 1000000;
		tmp *= node->compare_base - node->count_base;
		node->expire_delta = tmp / node->freq;
		node->expire_delta *= 1000;
	}
}

static void s3c2416_timer_update_node_freq(S3C2416TimerState *ts, int id)
{
	S3C2416TimerNode *node = &ts->timer_nodes[id];

	node->freq = ts->clock_freq /
		     ((node->prescaler + 1) * (1 << (node->divider + 1)));

	s3c2416_timer_update_expire(ts, id);
}

static void s3c2416_timer_start(S3C2416TimerState *ts, int id)
{
	S3C2416TimerNode *node = &ts->timer_nodes[id];

	node->expire_time =
	    qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + node->expire_delta;
	timer_mod(&node->timer, node->expire_time);
}

static void s3c2416_timer_stop(S3C2416TimerState *ts, int id)
{
	S3C2416TimerNode *node = &ts->timer_nodes[id];
	int64_t tmp;

	tmp = node->expire_time - qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
	if (tmp < 0) {
		node->expire_delta = 0;
	} else {
		node->expire_delta = tmp;
	}
	timer_del(&(ts->timer_nodes[id].timer));
}

static void s3c2416_timer_write(void *opaque, hwaddr addr, uint64_t data,
				unsigned size)
{
	S3C2416TimerState *ts = S3C2416_TIMER(opaque);
	uint32_t tmp;
	int i = 0;

	if (!ts->enabled) {
		return;
	}

	switch (addr) {
	case TCFG0:
		if (ts->reg_cfg0 == data) {
			break;
		}

		tmp = ts->reg_cfg0 ^ data;
		if ((tmp & 0xff) != 0) {
			ts->timer_nodes[0].prescaler = data & 0xff;
			ts->timer_nodes[1].prescaler = data & 0xff;

			s3c2416_timer_update_node_freq(ts, 0);
			s3c2416_timer_update_node_freq(ts, 1);
		}
		if ((tmp & 0xff00) != 0) {
			ts->timer_nodes[2].prescaler = (data & 0xff00) >> 8;
			ts->timer_nodes[3].prescaler = (data & 0xff00) >> 8;
			ts->timer_nodes[4].prescaler = (data & 0xff00) >> 8;

			s3c2416_timer_update_node_freq(ts, 2);
			s3c2416_timer_update_node_freq(ts, 3);
			s3c2416_timer_update_node_freq(ts, 4);
		}

		ts->reg_cfg0 = data;
		break;
	case TCFG1:
		if (ts->reg_cfg1 == data) {
			break;
		}

		tmp = ts->reg_cfg0 ^ data;
		for (i = 0; i < 5; i++) {
			if ((tmp & (0x0f << (i * 4))) == 0) {
				continue;
			}
			ts->timer_nodes[i].divider = (data >> (i * 4)) & 0x0f;
			s3c2416_timer_update_node_freq(ts, i);
		}

		ts->reg_cfg1 = data;
		break;
	case TCON:
		if (data == ts->reg_con) {
			break;
		}

		for (i = 0; i < 5; i++) {
			ts->timer_nodes[i].auto_reload =
			    !!(data & TCON_TIMER_AUTO_RELOAD(i));
			ts->timer_nodes[i].enable =
			    !!(data & TCON_TIMER_START(i));

			if (data & TCON_TIMER_MANUAL_UPDATE(i)) {
				s3c2416_timer_update_expire(ts, i);
			}

			if (data & TCON_TIMER_START(i)) {
				s3c2416_timer_start(ts, i);
			} else {
				s3c2416_timer_stop(ts, i);
			}
		}
		ts->reg_con = data;

		break;
	case TCNTB0:
	case TCNTB1:
	case TCNTB2:
	case TCNTB3:
	case TCNTB4:
		i = (addr - TCNTB0) / 0x0c;
		ts->timer_nodes[i].count_base = data;
		break;
	case TCMPB0:
	case TCMPB1:
	case TCMPB2:
	case TCMPB3:
		i = (addr - TCMPB0) / 0x0c;
		ts->timer_nodes[i].compare_base = data;
		break;
	case TCNTO0:
	case TCNTO1:
	case TCNTO2:
	case TCNTO3:
	case TCNTO4:
	default:
		break;
	}
}

static void s3c2416_timer_expire(void *opaque)
{
	S3C2416TimerNode *node = (S3C2416TimerNode *)opaque;

	qemu_set_irq(node->irq, 1);

	if (node->auto_reload) {
		s3c2416_timer_update_expire(node->timer_ctrl, node->id);
		s3c2416_timer_start(node->timer_ctrl, node->id);
	}
}

static const MemoryRegionOps s3c2416_timer_ops = {
    .read = s3c2416_timer_read,
    .write = s3c2416_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void timer_prop_set_enabled(Object *obj, Visitor *v, const char *name,
				   void *opaque, Error **errp)
{
	S3C2416TimerState *ts = S3C2416_TIMER(obj);
	bool value;
	int i = 0;

	visit_type_bool(v, name, &value, errp);

	if (value == ts->enabled) {
		return;
	}

	ts->enabled = value;

	for (i = 0; i < S3C2416_NR_TIMERS; i++) {
		if (value && ts->timer_nodes[i].enable) {
			s3c2416_timer_start(ts, i);
		} else if (!value && ts->timer_nodes[i].enable) {
			s3c2416_timer_stop(ts, i);
		}
	}
}

static void timer_prop_get_enabled(Object *obj, Visitor *v, const char *name,
				   void *opaque, Error **errp)
{
	S3C2416TimerState *ts = S3C2416_TIMER(obj);
	bool value = ts->enabled;
	visit_type_bool(v, name, &value, errp);
}

static void timer_prop_set_clock(Object *obj, Visitor *v, const char *name,
				 void *opaque, Error **errp)
{
	uint64_t value;
	Error *error = NULL;
	S3C2416TimerState *ts = S3C2416_TIMER(obj);
	int i;

	visit_type_uint64(v, name, &value, &error);

	if (error) {
		error_propagate(errp, error);
	}

	if (ts->clock_freq == value) {
		return;
	}

	ts->clock_freq = value;

	for (i = 0; i < S3C2416_NR_TIMERS; i++) {
		s3c2416_timer_update_node_freq(ts, i);
	}
}

static void timer_prop_get_clock(Object *obj, Visitor *v, const char *name,
				 void *opaque, Error **errp)
{
	S3C2416TimerState *ts = S3C2416_TIMER(obj);
	uint64_t value = ts->clock_freq;

	visit_type_uint64(v, name, &value, errp);
}

static void s3c2416_timer_realize(DeviceState *state, Error **err)
{
	S3C2416TimerState *ts = S3C2416_TIMER(state);
	int i = 0;

	memory_region_init_io(&ts->iomem, OBJECT(ts), &s3c2416_timer_ops, ts,
			      TYPE_S3C2416_TIMER, TIMER_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ts), &ts->iomem);

	for (i = 0; i < S3C2416_NR_TIMERS; i++) {
		sysbus_init_irq(SYS_BUS_DEVICE(ts), &(ts->timer_nodes[i].irq));
	}
}

static void s3c2416_timer_reset(DeviceState *state) {}

static void s3c2416_timer_init(Object *obj)
{
	S3C2416TimerState *ts = S3C2416_TIMER(obj);
	int i = 0;

	for (i = 0; i < S3C2416_NR_TIMERS; i++) {
		ts->timer_nodes[i].timer_ctrl = ts;
		ts->timer_nodes[i].id = i;
		timer_init_ns(&(ts->timer_nodes[i].timer), QEMU_CLOCK_VIRTUAL,
			      s3c2416_timer_expire, &(ts->timer_nodes[i]));
	}

	object_property_add(obj, "clock-freq", "int", timer_prop_get_clock,
			    timer_prop_set_clock, NULL, obj, NULL);

	object_property_add(obj, "enabled", "bool", timer_prop_get_enabled,
			    timer_prop_set_enabled, NULL, obj, NULL);

	ts->nr_irqs = S3C2416_NR_TIMERS;
}

static Property s3c2416_timer_properties[] = {

    DEFINE_PROP_END_OF_LIST(),
};

static void s3c2416_timer_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = s3c2416_timer_realize;
	dc->reset = s3c2416_timer_reset;
	dc->props = s3c2416_timer_properties;
}

static const TypeInfo s3c2416_timer_type_info = {
    .name = TYPE_S3C2416_TIMER,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S3C2416TimerState),
    .instance_init = s3c2416_timer_init,
    .class_init = s3c2416_timer_class_init,
};

static void s3c2416_timer_register_types(void)
{
	type_register_static(&s3c2416_timer_type_info);
}

type_init(s3c2416_timer_register_types)
