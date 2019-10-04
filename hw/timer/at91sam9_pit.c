/*
 * at91sam9_pit.c
 *
 *  Created on: Apr 12, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"
#include "hw/timer/at91sam9_pit.h"

#define PIT_MR 0x0
#define PIT_SR 0x4
#define PIT_PIVR 0x8
#define PIT_PIIR 0xc

#define PIT_MR_PIV_SHIFT 0
#define PIT_MR_PIV_MASK 0xfffff
#define PIT_MR_PITEN_SHIFT 24
#define PIT_MR_PITEN_ENABLE 1
#define PIT_MR_PITEN_DISABLE 0
#define PIT_MR_PITEN_MASK 1
#define PIT_MR_PITIEN_SHIFT 25
#define PIT_MR_PITIEN_ENABLE 1
#define PIT_MR_PITIEN_DISABLE 0
#define PIT_MR_PITIEN_MASK 1

#define PIT_SR_PITS (1 << 0)

#define PIT_PIVR_CPIV_SHIFT 0
#define PIT_PIVR_CPIV_MASK 0xfffff
#define PIT_PIVR_PICNT_SHIFT 20
#define PIT_PIVR_PICNT_MASK 0xfff

#define PIT_PIIR_CPIV_SHIFT 0
#define PIT_PIIR_CPIV_MASK 0xfffff
#define PIT_PIIR_PICNT_SHIFT 20
#define PIT_PIIR_PICNT_MASK 0xfff

#define PIT_REGION_SIZE 0x10

static void at91sam9_pit_timer_expire(void *opaque)
{
	At91sam9PitState *ps = AT91SAM9_PIT(opaque);

	if (ps->reg_pit_mr & (PIT_MR_PITIEN_ENABLE << PIT_MR_PITIEN_SHIFT)) {
		ps->reg_pit_sr |= PIT_SR_PITS;
		qemu_irq_raise(ps->irq);
	}

	timer_mod(&ps->timer,
		  qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + ps->interval);
}

static void at91sam9_PIT_MR_write(At91sam9PitState *ps, uint32_t value)
{
	value = (ps->reg_pit_mr ^ value) & value;

	ps->interval = 10000;

	if (value & (PIT_MR_PITEN_ENABLE << PIT_MR_PITEN_SHIFT)) {
		timer_mod(&ps->timer,
			  qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + ps->interval);
	} else {
		timer_del(&ps->timer);
	}
}

static uint64_t at91sam9_pit_read(void *opaque, hwaddr addr, unsigned size)
{
	At91sam9PitState *ps = AT91SAM9_PIT(opaque);

	// fprintf(stderr,"pit read %lx\n",addr);
	switch (addr) {
	case PIT_MR:
		return ps->reg_pit_mr;
	case PIT_SR:
		return ps->reg_pit_sr;
	case PIT_PIVR:
		ps->reg_pit_sr &= ~PIT_SR_PITS;
		return ps->reg_pit_pivr;
	case PIT_PIIR:
		return ps->reg_pit_piir;
	}
	return 0;
}

static void at91sam9_pit_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	At91sam9PitState *ps = AT91SAM9_PIT(opaque);

	// fprintf(stderr,"pit write %lx %lx\n",addr,data);
	switch (addr) {
	case PIT_MR:
		at91sam9_PIT_MR_write(ps, data);
		ps->reg_pit_mr = data;
		break;
	}
}

static const MemoryRegionOps at91sam9_pit_ops = {
    .read = at91sam9_pit_read,
    .write = at91sam9_pit_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void at91sam9_pit_realize(DeviceState *state, Error **err)
{
	At91sam9PitState *ps = AT91SAM9_PIT(state);

	sysbus_mmio_map(SYS_BUS_DEVICE(ps), 0, ps->addr);

}

static void at91sam9_pit_reset(DeviceState *state) {}

static void at91sam9_pit_init(Object *obj)
{
	At91sam9PitState *ps = AT91SAM9_PIT(obj);

	memory_region_init_io(&ps->iomem, OBJECT(ps), &at91sam9_pit_ops, ps,
			      TYPE_AT91SAM9_PIT, PIT_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ps), &ps->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(ps), &(ps->irq));

	timer_init_us(&ps->timer, QEMU_CLOCK_VIRTUAL, at91sam9_pit_timer_expire,
		      ps);
}

static Property at91sam9_pit_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9PitState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void at91sam9_pit_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = at91sam9_pit_realize;
	dc->reset = at91sam9_pit_reset;
	dc->props = at91sam9_pit_properties;
}

static const TypeInfo at91sam9_timer_type_info = {
    .name = TYPE_AT91SAM9_PIT,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(At91sam9PitState),
    .instance_init = at91sam9_pit_init,
    .class_init = at91sam9_pit_class_init,
};

static void at91sam9_timer_register_types(void)
{
	type_register_static(&at91sam9_timer_type_info);
}

type_init(at91sam9_timer_register_types)
