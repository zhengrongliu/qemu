/*
 * s3c2416_vic.c
 *
 *  Created on: Dec 28, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/intc/s3c2416_vic.h"

#define VIC_REGION_SIZE 0x100

/*
 * vic reg offset
 */
#define SRCPND1 (0x00)
#define INTMOD1 (0x04)
#define INTMSK1 (0x08)
#define INTPND1 (0x10)
#define INTOFFSET1 (0x14)
#define SUBSRCPND (0x18)
#define INTSUBMSK (0x1c)
#define PRIORITY_MODE1 (0x30)
#define PRIORITY_UPDATE1 (0x34)
#define SRCPND2 (0x40)
#define INTMOD2 (0x44)
#define INTMSK2 (0x48)
#define INTPND2 (0x50)
#define INTOFFSET2 (0x54)
#define PRIORITY_MODE2 (0x70)
#define PRIORITY_UPDATE2 (0x74)

#define NR_IRQS (96)

static const int irq_arbiter_index[NR_IRQS][2] = {
	[10] = {2, 0}, [11] = {2, 1}, [12] = {2, 2},
	[13] = {2, 3}, [14] = {2, 4},
};

static const int ariter_priority_order_mr[4][6] = {
    {0, 1, 2, 3, 4, 5},
    {0, 4, 1, 2, 3, 5},
    {0, 3, 4, 1, 2, 5},
    {0, 2, 3, 4, 1, 5},
};

static const int ariter_priority_order_ar[6][6] = {
    {0, 1, 2, 3, 4, 5}, {5, 0, 1, 2, 3, 4}, {4, 5, 0, 1, 2, 3},
    {3, 4, 5, 0, 1, 2}, {2, 3, 4, 5, 0, 1}, {1, 2, 3, 4, 5, 0},
};

static void s3c2416_vic_irq_arbiter_index(int irq_no, int *arb_1st,
					  int *arb_2nd)
{

	*arb_1st = irq_arbiter_index[irq_no][0];
	*arb_2nd = irq_arbiter_index[irq_no][1];
}

static int s3c2416_vic_irq_priority(S3C2416VicState *vs, int irq_no)
{
	int arb_1st;
	int arb_2nd;
	int val1, val2;

	s3c2416_vic_irq_arbiter_index(irq_no, &arb_1st, &arb_2nd);

	if (vs->arbiter_mode[6]) {
		val1 = ariter_priority_order_ar[vs->arbiter_sel[6]][arb_1st];
	} else {
		val1 = ariter_priority_order_mr[vs->arbiter_sel[6]][arb_1st];
	}

	if (vs->arbiter_mode[arb_1st]) {
		val2 =
		    ariter_priority_order_ar[vs->arbiter_sel[arb_1st]][arb_2nd];
	} else {
		val2 =
		    ariter_priority_order_mr[vs->arbiter_sel[arb_1st]][arb_2nd];
	}

	if (irq_no < 32) {
		return val1 * 6 + val2;
	}

	return (val1 + 6) * 6 + val2;
}

static void s3c2416_vic_irq_tigger(S3C2416VicState *vs)
{
	qemu_irq irq_up = vs->arm_irq;
	int i = 0;
	int tmp;
	int irq_no = -1, prior = 0x7fffffff, p;
	int arb_1st, arb_2nd;

	tmp = vs->reg_srcpnd1 & ~(vs->reg_intmsk1);
	for (i = 0; i < 32; i++) {
		if ((tmp & (1 << i)) == 0) {
			continue;
		}

		p = s3c2416_vic_irq_priority(vs, i);
		if (p < prior) {
			irq_no = i;
			prior = p;
		}
	}

	if (irq_no != -1) {
		if (vs->reg_intmod1 & (1 << irq_no)) {
			irq_up = vs->arm_fiq;
		} else {
			vs->reg_intpnd1 = 1 << irq_no;
			vs->reg_intoffset1 = irq_no;
		}
		goto has_int;
	}

	tmp = vs->reg_srcpnd2 & ~(vs->reg_intmsk2);
	for (i = 0; i < 32; i++) {
		if ((tmp & (1 << i)) == 0) {
			continue;
		}

		p = s3c2416_vic_irq_priority(vs, i);
		if (p < prior) {
			irq_no = i;
			prior = p;
		}
	}

	if (irq_no == -1) {
		return;
	}

	if (vs->reg_intmod2 & (1 << irq_no)) {
		irq_up = vs->arm_fiq;
	} else {
		vs->reg_intpnd2 = 1 << irq_no;
		vs->reg_intoffset2 = irq_no;
	}

	irq_no += 32;

has_int:

	s3c2416_vic_irq_arbiter_index(irq_no, &arb_1st, &arb_2nd);

	if (vs->arbiter_rotate[arb_1st]) {
		if (vs->arbiter_mode[arb_1st]) {
			vs->arbiter_sel[arb_1st] = (arb_1st + 1) % 6;
		} else if (arb_1st != 0 && arb_1st != 5) {
			vs->arbiter_sel[arb_1st] = (arb_1st + 1) % 4;
		}
	}

	if (vs->arbiter_rotate[arb_2nd]) {
		if (vs->arbiter_mode[arb_2nd]) {
			vs->arbiter_sel[arb_2nd] = (arb_2nd + 1) % 6;
		} else if (arb_1st != 0 && arb_2nd != 5) {
			vs->arbiter_sel[arb_2nd] = (arb_2nd + 1) % 4;
		}
	}

#if 0
	if(irq_no != 13){
        fprintf(stderr,"QEMU:qemu_set_irq %d\n",irq_no);
	}
#endif
	qemu_set_irq(irq_up, 1);
}

static void s3c2416_vic_pending_set(S3C2416VicState *vs, int irq)
{
	if (irq >= 64) {
		vs->reg_subsrcpnd |= 1 << (irq - 64);

		switch (irq) {
		case HWIRQ_N_UART0_RXD:
		case HWIRQ_N_UART0_TXD:
		case HWIRQ_N_UART0_ERR:
			irq = HWIRQ_N_UART0;
			break;
		case HWIRQ_N_UART1_RXD:
		case HWIRQ_N_UART1_TXD:
		case HWIRQ_N_UART1_ERR:
			irq = HWIRQ_N_UART1;
			break;
		case HWIRQ_N_UART2_RXD:
		case HWIRQ_N_UART2_TXD:
		case HWIRQ_N_UART2_ERR:
			irq = HWIRQ_N_UART2;
			break;
		case HWIRQ_N_UART3_RXD:
		case HWIRQ_N_UART3_TXD:
		case HWIRQ_N_UART3_ERR:
			irq = HWIRQ_N_UART3;
			break;
		}
	}

	if (irq < 32) {
		vs->reg_srcpnd1 |= 1 << irq;
	} else if (irq < 64) {
		vs->reg_srcpnd2 |= 1 << (irq - 32);
	}
}

static void s3c2416_vic_irqhandler(void *opaque, int n, int level)
{
	S3C2416VicState *vs = S3C2416_VIC(opaque);

#if 0
	if(n != 13){
        fprintf(stderr,"QEMU:interrupt %d\n",n);
	}
#endif

	s3c2416_vic_pending_set(vs, n);

	s3c2416_vic_irq_tigger(vs);
}

static uint64_t s3c2416_vic_read(void *opaque, hwaddr addr, unsigned size)
{
	S3C2416VicState *vs = S3C2416_VIC(opaque);

	switch (addr) {
	case SRCPND1:
		return vs->reg_srcpnd1;
	case INTPND1:
		return vs->reg_intpnd1;
	case INTMOD1:
		return vs->reg_intmod1;
	case INTMSK1:
		return vs->reg_intmsk1;
	case SRCPND2:
		return vs->reg_srcpnd2;
	case INTMOD2:
		return vs->reg_intmod2;
	case INTMSK2:
		return vs->reg_intmsk2;
	case INTPND2:
		return vs->reg_intpnd2;
	case SUBSRCPND:
		return vs->reg_subsrcpnd;
	case INTSUBMSK:
		return vs->reg_intsubmsk;
	case INTOFFSET1:
		return vs->reg_intoffset1;
	case INTOFFSET2:
		return vs->reg_intoffset2;
	case PRIORITY_MODE1:
		return vs->reg_priority_mode1;
	case PRIORITY_MODE2:
		return vs->reg_priority_mode2;
	case PRIORITY_UPDATE1:
		return vs->reg_priority_update1;
	case PRIORITY_UPDATE2:
		return vs->reg_priority_update2;
	default:
		break;
	}
	return 0;
}

static void s3c2416_vic_write(void *opaque, hwaddr addr, uint64_t data,
			      unsigned size)
{
	S3C2416VicState *vs = S3C2416_VIC(opaque);
	int i = 0;

	switch (addr) {
	case SRCPND1:
		vs->reg_srcpnd1 &= ~data;
		break;
	case SRCPND2:
		vs->reg_srcpnd2 &= ~data;
		break;
	case SUBSRCPND:
		vs->reg_subsrcpnd &= ~data;
		break;
	case INTPND1:
		vs->reg_intpnd1 &= ~data;
		if (!vs->reg_intpnd1 && !vs->reg_intpnd2) {
			qemu_set_irq(vs->arm_irq, 0);
		}
		break;
	case INTPND2:
		vs->reg_intpnd2 &= ~data;
		if (!vs->reg_intpnd1 && !vs->reg_intpnd2) {
			qemu_set_irq(vs->arm_irq, 0);
		}
		break;

	case INTMSK1:
		vs->reg_intmsk1 = data;
		break;
	case INTMSK2:
		vs->reg_intmsk2 = data;
		break;
	case INTSUBMSK:
		vs->reg_intsubmsk = data;
		break;
	case INTMOD1:
		vs->reg_intmod1 = 0;
		for (i = 0; i < sizeof(vs->reg_intmod1) * 8; i++) {
			if (data & 1) {
				vs->reg_intmod1 = 1 << i;
				break;
			}
			data >>= 1;
		}
		break;
	case INTMOD2:
		vs->reg_intmod2 = 0;
		for (i = 0; i < sizeof(vs->reg_intmod2) * 8; i++) {
			if (data & 1) {
				vs->reg_intmod2 = 1 << i;
				break;
			}
			data >>= 1;
		}
		break;
	case PRIORITY_MODE1:
		vs->reg_priority_mode1 = data;

		for (i = 0; i < 7; i++) {
			vs->arbiter_sel[i] = data & 0x07;
			vs->arbiter_mode[i] = ((data >> 3) & 0x1);

			data >>= 4;
		}

		vs->arbiter_mode[0] = 0;
		vs->arbiter_mode[5] = 0;
		vs->arbiter_sel[0] &= 0x3;

		break;
	case PRIORITY_MODE2:
		vs->reg_priority_mode2 = data;

		for (i = 0; i < 6; i++) {
			vs->arbiter_sel[i + 7] = data & 0x07;
			vs->arbiter_mode[i + 7] = ((data >> 3) & 0x1);

			data >>= 4;
		}

		vs->arbiter_mode[7] = 0;
		vs->arbiter_mode[12] = 0;
		vs->arbiter_sel[7] &= 0x3;
		break;
	case PRIORITY_UPDATE1:
		for (i = 0; i < 7; i++) {
			vs->arbiter_rotate[i] = (data & (1 << i)) ? 1 : 0;
		}

		vs->reg_priority_update1 = data;
		break;
	case PRIORITY_UPDATE2:
		for (i = 0; i < 6; i++) {
			vs->arbiter_rotate[i + 7] = (data & (1 << i)) ? 1 : 0;
		}

		vs->reg_priority_update2 = data;
		break;
	default:
		break;
	}
}

static void s3c2416_vic_time_expire(void *opaque) {}

static const MemoryRegionOps s3c2416_vic_ops = {
    .read = s3c2416_vic_read,
    .write = s3c2416_vic_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void s3c2416_vic_realize(DeviceState *state, Error **err)
{
	S3C2416VicState *vs = S3C2416_VIC(state);

	sysbus_init_mmio(SYS_BUS_DEVICE(vs), &vs->iomem);
	memory_region_init_io(&vs->iomem, OBJECT(vs), &s3c2416_vic_ops, vs,
			      TYPE_S3C2416_VIC, VIC_REGION_SIZE);

	sysbus_init_irq(SYS_BUS_DEVICE(vs), &vs->arm_irq);
	sysbus_init_irq(SYS_BUS_DEVICE(vs), &vs->arm_fiq);
}

static void s3c2416_vic_reset(DeviceState *state)
{
	S3C2416VicState *vs = S3C2416_VIC(state);
	int i;

	vs->reg_intmod1 = 0x0;
	vs->reg_intmod2 = 0x0;
	vs->reg_intmsk1 = 0xffffffff;
	vs->reg_intmsk2 = 0xffffffff;
	vs->reg_intsubmsk = 0xffffffff;

	vs->reg_intpnd1 = 0x0;
	vs->reg_intpnd2 = 0x0;
	vs->reg_srcpnd1 = 0x0;
	vs->reg_srcpnd2 = 0x0;
	vs->reg_subsrcpnd = 0x0;

	vs->reg_intoffset1 = 0x0;
	vs->reg_intoffset2 = 0x0;

	vs->reg_priority_mode1 = 0x0;
	vs->reg_priority_mode2 = 0x0;

	for (i = 0; i < 13; i++) {
		vs->arbiter_mode[i] = 0;
		vs->arbiter_rotate[i] = 0;
		vs->arbiter_sel[i] = 0;
	}
}

static void s3c2416_vic_init(Object *obj)
{
	S3C2416VicState *vs = S3C2416_VIC(obj);

	timer_init_ns(&(vs->timer), QEMU_CLOCK_VIRTUAL, s3c2416_vic_time_expire,
		      &vs);

	qdev_init_gpio_in(DEVICE(vs), s3c2416_vic_irqhandler, NR_IRQS);
}

static void s3c2416_vic_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = s3c2416_vic_realize;
	dc->reset = s3c2416_vic_reset;
}

static const TypeInfo s3c2416_vic_type_info = {
    .name = TYPE_S3C2416_VIC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S3C2416VicState),
    .instance_init = s3c2416_vic_init,
    .class_init = s3c2416_vic_class_init,
};

static void s3c2416_vic_register_types(void)
{
	type_register_static(&s3c2416_vic_type_info);
}

type_init(s3c2416_vic_register_types)
