/*
 * at91sam9_aic.c
 *
 *  Created on: Apr 12, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/intc/at91sam9_aic.h"

#define AIC_REGION_SIZE 0x200

#define AIC_REG_WIDTH 4

#define AIC_SMR0 0x0
#define AIC_SMR31 0x7c
#define AIC_SVR0 0x80
#define AIC_SVR31 0xfc
#define AIC_IVR 0x100
#define AIC_FVR 0x104
#define AIC_ISR 0x108
#define AIC_IPR 0x10c
#define AIC_IMR 0x110
#define AIC_CISR 0x114
#define AIC_IECR 0x120
#define AIC_IDCR 0x124
#define AIC_ICCR 0x128
#define AIC_ISCR 0x12c
#define AIC_EOICR 0x130
#define AIC_SPU 0x134
#define AIC_DCR 0x138
#define AIC_FFER 0x140
#define AIC_FFDR 0x144
#define AIC_FFSR 0x148

#define AIC_SMR_PRIOR_SHIFT 0x0
#define AIC_SMR_PRIOR_MASK 0x7
#define AIC_SMR_SRCTYPE_SHIFT 0x5
#define AIC_SMR_SRCTYPE_MASK 0x3

#define AIC_DCR_PROT_SHIFT 0x0
#define AIC_DCR_GMSK_SHIFT 0x1

static int aic_active_interrupt_get(struct At91sam9AicState *as)
{
	int i;
	int prior = 0;
	int src = -1;

	for (i = NR_IRQS - 1; i >= 1; i--) {
		if (!(as->src_pending & (1 << i))) {
			continue;
		}

		if (as->src_prior[i] >= prior) {
			prior = as->src_prior[i];
			src = i;
		}
	}

	return src;
}

static void aic_interrupt_ack(struct At91sam9AicState *as, int src)
{
	as->src_pending &= ~(1 << src);
	qemu_set_irq(as->arm_irq, 0);
}

static uint32_t aic_read_ivr(struct At91sam9AicState *as)
{
	int src;

	src = aic_active_interrupt_get(as);

	// fprintf(stderr,"active src =  %d\n",src);
	if (src < 0) {
		return as->spu_vec;
	}

	if (as->top == 0 || src != as->src_nr_stack[as->top]) {
		as->top++;
		as->src_nr_stack[as->top] = src;
		as->src_prior_stack[as->top] = as->src_prior[src];
	}

	aic_interrupt_ack(as, src);

	return as->src_vec[src];
}

static uint32_t aic_read_fvr(struct At91sam9AicState *as) { return 0; }

static void aic_write_eoi(struct At91sam9AicState *as)
{
	as->top--;
	if (as->src_pending) {
		qemu_set_irq(as->arm_irq, 1);
	}
}

static uint64_t at91sam9_aic_read(void *opaque, hwaddr addr, unsigned size)
{
	struct At91sam9AicState *as = AT91SAM9_AIC(opaque);
	int index;

	// fprintf(stderr,"aic read %lx\n",addr);
	switch (addr) {
	case AIC_SMR0... AIC_SMR31:
		index = (addr - AIC_SMR0) / AIC_REG_WIDTH;
		return (as->src_type[index] << AIC_SMR_SRCTYPE_SHIFT) |
		       (as->src_prior[index] << AIC_SMR_PRIOR_SHIFT);
	case AIC_SVR0... AIC_SVR31:
		index = (addr - AIC_SVR0) / AIC_REG_WIDTH;
		return as->src_vec[index];
	case AIC_IVR:
		return aic_read_ivr(as);
	case AIC_FVR:
		return aic_read_fvr(as);
	case AIC_ISR:
		return as->src_nr_stack[as->top];
	case AIC_IPR:
		return as->src_pending;
	case AIC_IMR:
		return as->src_mask;
	case AIC_CISR:
		return 0;
	case AIC_SPU:
		return as->spu_vec;
	case AIC_DCR:
		return (as->protect_mode << AIC_DCR_PROT_SHIFT) |
		       (as->general_mask << AIC_DCR_GMSK_SHIFT);
	case AIC_FFSR:
		break;
	}

	return 0;
}

static void at91sam9_aic_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	struct At91sam9AicState *as = AT91SAM9_AIC(opaque);
	int index;

	// fprintf(stderr,"aic write %lx %lx\n",addr,data);
	switch (addr) {
	case AIC_SMR0... AIC_SMR31:
		index = (addr - AIC_SMR0) / AIC_REG_WIDTH;
		as->src_type[index] =
		    (data >> AIC_SMR_SRCTYPE_SHIFT) & AIC_SMR_SRCTYPE_MASK;
		as->src_prior[index] =
		    (data >> AIC_SMR_PRIOR_SHIFT) & AIC_SMR_PRIOR_MASK;
		break;
	case AIC_SVR0... AIC_SVR31:
		index = (addr - AIC_SVR0) / AIC_REG_WIDTH;
		as->src_vec[index] = data;
		break;
	case AIC_IECR:
		as->src_mask &= ~data;
		break;
	case AIC_IDCR:
		as->src_mask |= data;
		break;
	case AIC_ICCR:
		as->src_pending &= ~data;
		break;
	case AIC_ISCR:
		as->src_pending |= data;
		break;
	case AIC_EOICR:
		aic_write_eoi(as);
		break;
	case AIC_SPU:
		as->spu_vec = data;
		break;
	case AIC_DCR:
		as->protect_mode = (data & (1 << AIC_DCR_PROT_SHIFT)) ? 1 : 0;
		as->general_mask = (data & (1 << AIC_DCR_GMSK_SHIFT)) ? 1 : 0;
		break;
	case AIC_FFER:
	case AIC_FFDR:
	case AIC_FFSR:
		break;
	}
}

static void aic_irqhandler(void *opaque, int n, int level)
{
	At91sam9AicState *as = AT91SAM9_AIC(opaque);

	as->src_pending |= 1 << n;

	qemu_set_irq(as->arm_irq, 1);
}

static const MemoryRegionOps at91sam9_aic_ops = {
    .read = at91sam9_aic_read,
    .write = at91sam9_aic_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void at91sam9_aic_realize(DeviceState *state, Error **err)
{
	At91sam9AicState *as = AT91SAM9_AIC(state);

	sysbus_mmio_map(SYS_BUS_DEVICE(as), 0, as->addr);

}

static void at91sam9_aic_reset(DeviceState *state)
{
	struct At91sam9AicState *as = AT91SAM9_AIC(state);

	as->top = 0;
}

static void at91sam9_aic_init(Object *obj)
{
	struct At91sam9AicState *as = AT91SAM9_AIC(obj);

	memory_region_init_io(&as->iomem, OBJECT(as), &at91sam9_aic_ops, as,
			      TYPE_AT91SAM9_AIC, AIC_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(as), &as->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(as), &as->arm_irq);
	sysbus_init_irq(SYS_BUS_DEVICE(as), &as->arm_fiq);

	qdev_init_gpio_in(DEVICE(as), aic_irqhandler, NR_IRQS);

}

static Property at91sam9_aic_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9AicState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void at91sam9_aic_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = at91sam9_aic_realize;
	dc->reset = at91sam9_aic_reset;
	dc->props = at91sam9_aic_properties;
}

static const TypeInfo at91sam9_aic_type_info = {
    .name = TYPE_AT91SAM9_AIC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(At91sam9AicState),
    .instance_init = at91sam9_aic_init,
    .class_init = at91sam9_aic_class_init,
};

static void at91sam9_aic_register_types(void)
{
	type_register_static(&at91sam9_aic_type_info);
}

type_init(at91sam9_aic_register_types)
