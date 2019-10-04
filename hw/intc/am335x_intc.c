/*
 * am335x_intc.c
 *
 *  Created on: Feb 5, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/timer.h"
#include "qemu/typedefs.h"
#include "hw/irq.h"
#include "hw/sysbus.h"

#define TYPE_AM335X_INTC "am335x-intc"
#define AM335X_INTC(obj)                                                      \
	OBJECT_CHECK(Am335xIntcState, (obj), TYPE_AM335X_INTC)

#define NR_IRQS 128
#define INTC_BANKS DIV_ROUND_UP(NR_IRQS,32)

typedef struct Am335xIntcState Am335xIntcState;

typedef struct Am335xIntcBank{
	/*< private >*/
	Object parent_obj;
	/* <public> */
	MemoryRegion iomem;

	Am335xIntcState *intc;

	uint32_t reg_mask;
	uint32_t reg_isr;
	uint32_t reg_pending_irq;
	uint32_t reg_pending_fiq;
}Am335xIntcBank;

typedef struct Am335xIntcState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;
	uint32_t addr;

	qemu_irq arm_irq;
	qemu_irq arm_fiq;

	uint8_t priorities[NR_IRQS];
	uint8_t pending_srcs[NR_IRQS];

	uint32_t reg_sir_irq;
	uint32_t reg_priority_irq;
	uint32_t reg_threshold;

	Am335xIntcBank	banks[INTC_BANKS];


} Am335xIntcState;


#define INTC_REGION_SIZE	0x300
#define INTC_BANK_REGION_SIZE	0x20


#define INTC_REVISION 		0x0
#define INTC_SYSCONFIG		0x10
#define INTC_SYSSTATUS		0x14
#define INTC_SIR_IRQ		0x40
#define INTC_SIR_FIQ		0x44
#define INTC_CONTROL		0x48
#define INTC_PROTECTION		0x4c
#define INTC_IDLE		0x50
#define INTC_IRQ_PRIORITY	0x60
#define INTC_FIQ_PRIORITY	0x64
#define INTC_THRESHOLD		0x68
#define INTC_BANK_ITR		0x0
#define INTC_BANK_MIR		0x4
#define INTC_BANK_MIR_CLEAR	0x8
#define INTC_BANK_MIR_SET	0xc
#define INTC_BANK_ISR_SET	0x10
#define INTC_BANK_ISR_CLEAR	0x14
#define INTC_BANK_PENDING_IRQ	0x18
#define INTC_BANK_PENDING_FIQ	0x1c
#define INTC_ITR(x)		(0x80+0x20*(x))
#define INTC_MIR(x)		(0x84+0x20*(x))
#define INTC_MIR_CLEAR(x)	(0x88+0x20*(x))
#define INTC_MIR_SET(x)		(0x8c+0x20*(x))
#define INTC_ISR_SET(x)		(0x90+0x20*(x))
#define INTC_ISR_CLEAR(x)	(0x94+0x20*(x))
#define INTC_PENDING_IRQ(x)	(0x98+0x20*(x))
#define INTC_PENDING_FIQ(x)	(0x9c+0x20*(x))
#define INTC_ILR(x)		(0x100 + 4*(x))

/*
 * INTC_REVISION register
 */
#define INTC_REVISION_REV_OFFSET	0x0
#define INTC_REVISION_REV_WIDTH		8
/*
 * INTC_SYSCONFIG register
 */
#define INTC_SYSCONFIG_AUTOIDLE		BIT(0)
#define INTC_SYSCONFIG_SOFTRESET	BIT(1)
/*
 * INTC_SYSSTATUS register
 */
#define INTC_SYSSTATUS_RESETDONE	BIT(0)
/*
 * INTC_SIR_IRQ register
 */
#define INTC_SIR_IRQ_ACTIVEIRQ_OFFSET	0
#define INTC_SIR_IRQ_ACTIVEIRQ_WIDTH	7
#define INTC_SIR_IRQ_ACTIVEIRQ_MASK	MAKE_64BIT_MASK(0,INTC_SIR_IRQ_ACTIVEIRQ_WIDTH)
#define INTC_SIR_IRQ_SPU_OFFSET 	7
#define INTC_SIR_IRQ_SPU_WIDTH		25
#define INTC_SIR_IRQ_SPU_MASK		MAKE_64BIT_MASK(0,INTC_SIR_IRQ_SPU_WIDTH)
/*
 * INTC_SIR_FIQ register
 */
#define INTC_SIR_FIQ_ACTIVEFIQ_OFFSET	0x0
#define INTC_SIR_FIQ_ACTIVEFIQ_WIDTH	7
#define INTC_SIR_FIQ_SPU_OFFSET 	0x7
#define INTC_SIR_FIQ_SPU_WIDTH		25
/*
 * INTC_CONTROL register
 */
#define INTC_CONTROL_NEWIRQAGR		BIT(0)
#define INTC_CONTROL_NEWFIQAGR		BIT(1)
/*
 * INTC_PROTECTION register
 */
#define INTC_PROTECTION_PROTECTION	BIT(0)
/*
 * INTC_IDLE register
 */
#define INTC_IDLE_FUNCIDLE		BIT(0)
#define INTC_IDLE_TURBO			BIT(1)
/*
 * INTC_IRQ_PRIORITY register
 */
#define INTC_IRQ_PRIORITY_PRIO_OFFSET	0
#define INTC_IRQ_PRIORITY_PRIO_WIDTH	7
#define INTC_IRQ_PRIORITY_SPU_OFFSET	7
#define INTC_IRQ_PRIORITY_SPU_WIDTH	9
/*
 * INTC_FIQ_PRIORITY register
 */
#define INTC_FIQ_PRIORITY_PRIO_OFFSET	0
#define INTC_FIQ_PRIORITY_PRIO_WIDTH	7
#define INTC_FIQ_PRIORITY_SPU_OFFSET	7
#define INTC_FIQ_PRIORITY_SPU_WIDTH	9
/*
 * INTC_THRESHOLD register
 */
#define INTC_THRESHOLD_PTH_OFFSET	0
#define INTC_THRESHOLD_PTH_WIDTH	8
/*
 * INTC_ILR register
 */
#define INTC_ILR_FIQnIRQ_OFFSET		0
#define INTC_ILR_FIQnIRQ_WIDTH		1
#define INTC_ILR_FIQ			1
#define INTC_ILR_IRQ			0
#define INTC_ILR_PRIORITY_OFFSET	2
#define INTC_ILR_PRIORITY_WIDTH		6
#define INTC_ILR_PRIORITY_MASK		MAKE_64BIT_MASK(0,INTC_ILR_PRIORITY_WIDTH)



static void intc_irq_update(Am335xIntcState *as);

static uint64_t am335x_intc_bank_read(void *opaque,hwaddr addr,unsigned size)
{
	Am335xIntcBank *bank = opaque;

	switch(addr){
	case INTC_BANK_ITR:
		return bank->reg_isr;
	case INTC_BANK_MIR:
		return bank->reg_mask;
	case INTC_BANK_PENDING_IRQ:
		return bank->reg_pending_irq;
	case INTC_BANK_PENDING_FIQ:
		return bank->reg_pending_fiq;
	}
	return 0;
}
static void am335x_intc_bank_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	Am335xIntcBank *bank = opaque;
	Am335xIntcState *as = bank->intc;

	switch(addr){
	case INTC_BANK_MIR_SET:
		bank->reg_mask |= data;
		intc_irq_update(as);
		break;
	case INTC_BANK_MIR_CLEAR:
		bank->reg_mask &= ~data;
		intc_irq_update(as);
		break;
	case INTC_BANK_ISR_SET:
		bank->reg_isr |= data;
		break;
	case INTC_BANK_ISR_CLEAR:
		bank->reg_isr &= ~data;
		break;
	}
}

static uint64_t am335x_intc_read(void *opaque, hwaddr addr, unsigned size)
{
	Am335xIntcState *as = AM335X_INTC(opaque);

	switch(addr){
	case INTC_SIR_IRQ:
		return as->reg_sir_irq;
	case INTC_IRQ_PRIORITY:
		return as->reg_priority_irq;
	case INTC_THRESHOLD:
		return as->reg_threshold;
	}
	return 0;
}

static void am335x_intc_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	Am335xIntcState *as = AM335X_INTC(opaque);

	switch(addr){
	case INTC_CONTROL:
		qemu_set_irq(as->arm_irq, 0);
		intc_irq_update(as);
		break;
	case INTC_ILR(0) ... INTC_ILR(127):
		as->priorities[(addr-INTC_ILR(0))/4] =
				(data >> INTC_ILR_PRIORITY_OFFSET)&INTC_ILR_PRIORITY_MASK;
		intc_irq_update(as);
		break;
	case INTC_THRESHOLD:
		as->reg_threshold = data;
		intc_irq_update(as);
		break;
	}
}

static void intc_active_irq(Am335xIntcState *as,int *irq,int *prior)
{
	int active_prior = as->reg_threshold;
	int active_irq = -1;
	int bank;
	int offset;
	unsigned long value;

	for(bank = 0;bank < INTC_BANKS;bank++){
		if(!as->banks[bank].reg_isr){
			continue;
		}

		value = as->banks[bank].reg_isr & ~as->banks[bank].reg_mask;

		for(offset = find_first_bit(&value,BITS_PER_LONG);
				offset != BITS_PER_LONG;offset = find_next_bit(&value,BITS_PER_LONG,offset+1)){
			int index;

			index = bank*32 + offset;

			if(as->priorities[index] < active_prior){
				active_irq = index;
				active_prior = as->priorities[index];
			}
		}

	}


	*irq = active_irq;
	*prior = active_prior;

}

static void intc_irq_update(Am335xIntcState *as)
{
	int irq,prior;

	intc_active_irq(as,&irq,&prior);
	if(irq < 0){
		return ;
	}

	as->reg_sir_irq = irq << INTC_SIR_IRQ_ACTIVEIRQ_OFFSET;
	as->reg_priority_irq = prior << INTC_IRQ_PRIORITY_PRIO_OFFSET;


	qemu_set_irq(as->arm_irq, 1);
}


static void intc_irqhandler(void *opaque, int irq, int level)
{
	Am335xIntcState *as = AM335X_INTC(opaque);
	int bank;
	int offset;


	bank = irq/32;
	offset = irq%32;

	if(level){
		as->banks[bank].reg_isr |= (1<<offset);
		intc_irq_update(as);
	} else {
		as->banks[bank].reg_isr &= ~(1<<offset);
	}



}

static const MemoryRegionOps am335x_intc_bank_ops = {
	.read = am335x_intc_bank_read,
	.write = am335x_intc_bank_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static const MemoryRegionOps am335x_intc_ops = {
	.read = am335x_intc_read,
	.write = am335x_intc_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static void am335x_intc_realize(DeviceState *state, Error **err)
{
	Am335xIntcState *as = AM335X_INTC(state);
	int i;

	sysbus_mmio_map(SYS_BUS_DEVICE(as), 0, as->addr);


	for(i = 0; i < INTC_BANKS;i++){
		memory_region_add_subregion(&as->iomem,INTC_ITR(i),&as->banks[i].iomem);
	}

}

static void am335x_intc_reset(DeviceState *state)
{
	struct Am335xIntcState *as = AM335X_INTC(state);

	as->reg_threshold = 0xff;
}

static void am335x_intc_init(Object *obj)
{
	struct Am335xIntcState *as = AM335X_INTC(obj);
	char *name;
	int i = 0;

	memory_region_init_io(&as->iomem, OBJECT(as), &am335x_intc_ops, as,
			      TYPE_AM335X_INTC, INTC_REGION_SIZE);

	for(i = 0;i < INTC_BANKS;i++ ){
		as->banks[i].intc = as;
		name = g_strdup_printf("%s-%d",TYPE_AM335X_INTC,i);
		memory_region_init_io(&as->banks[i].iomem, OBJECT(as),
				&am335x_intc_bank_ops, &as->banks[i],name,
				INTC_BANK_REGION_SIZE);
		g_free(name);
	}

	sysbus_init_mmio(SYS_BUS_DEVICE(as), &as->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(as), &as->arm_irq);
	sysbus_init_irq(SYS_BUS_DEVICE(as), &as->arm_fiq);

	qdev_init_gpio_in(DEVICE(as), intc_irqhandler, NR_IRQS);

}

static Property am335x_intc_properties[] = {
	DEFINE_PROP_UINT32("addr",Am335xIntcState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void am335x_intc_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = am335x_intc_realize;
	dc->reset = am335x_intc_reset;
	dc->props = am335x_intc_properties;
}

static const TypeInfo am335x_intc_type_info = {
	.name = TYPE_AM335X_INTC,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(Am335xIntcState),
	.instance_init = am335x_intc_init,
	.class_init = am335x_intc_class_init,
};

static void am335x_intc_register_types(void)
{
	type_register_static(&am335x_intc_type_info);
}

type_init(am335x_intc_register_types)
