/*
 * am335x_prcm.c
 *
 *  Created on: Feb 12, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"
#include "qapi/error.h"

#define TYPE_AM335X_PRCM "am335x-prcm"
#define AM335X_PRCM(obj)                                                      \
	OBJECT_CHECK(Am335xPrcmState, (obj), TYPE_AM335X_PRCM)

#define PRCM_REGION_SIZE 0x2000

typedef struct Am335xPrcmState {
	/* <private> */
	SysBusDevice parent_obj;
	/* <public> */
	MemoryRegion iomem;
	uint32_t addr;
	uint32_t regs[PRCM_REGION_SIZE/4];
} Am335xPrcmState;



#define CM_PER_BASE		0x00
#define CM_WKUP_BASE		0x400
#define CM_DPLL_BASE		0x500
#define CM_MPU_BASE		0x600
#define CM_DEVICE_BASE		0x700
#define CM_RTC_BASE		0x800
#define CM_GFX_BASE		0x900
#define CM_CEFUSE_BASE		0xa00


#define CM_PER_OFFSET(x)	(CM_PER_BASE + (x))
#define CM_WKUP_OFFSET(x) 	(CM_WKUP_BASE + (x))

/*
 * CM_PER registers
 */
#define CM_PER_L4LS_CLKSTCTRL		CM_PER_OFFSET(0x0)
#define CM_PER_L3S_CLKSTCTRL		CM_PER_OFFSET(0x4)
#define CM_PER_L3_CLKSTCTRL		CM_PER_OFFSET(0xc)

/*
 * CM_WKUP registers
 */
#define CM_CLKSEL_DPLL_CORE		CM_WKUP_OFFSET(0x68)
#define CM_IDLEST_DPLL_PER		CM_WKUP_OFFSET(0x70)
#define CM_DIV_M4_DPLL_CORE		CM_WKUP_OFFSET(0x80)
#define CM_DIV_M5_DPLL_CORE		CM_WKUP_OFFSET(0x84)
#define CM_CLKMODE_DPLL_MPU		CM_WKUP_OFFSET(0x88)
#define CM_CLKMODE_DPLL_PER		CM_WKUP_OFFSET(0x8c)
#define CM_CLKMODE_DPLL_CORE		CM_WKUP_OFFSET(0x90)
#define CM_CLKMODE_DPLL_DDR		CM_WKUP_OFFSET(0x94)
#define CM_CLKMODE_DPLL_DISP		CM_WKUP_OFFSET(0x98)
#define CM_CLKSEL_DPLL_PERIPH		CM_WKUP_OFFSET(0x9c)
#define CM_DIV_M2_DPLL_DDR		CM_WKUP_OFFSET(0xa0)
#define CM_DIV_M2_DPLL_DISP		CM_WKUP_OFFSET(0xa4)
#define CM_DIV_M2_DPLL_MPU		CM_WKUP_OFFSET(0xa8)
#define CM_DIV_M2_DPLL_PER		CM_WKUP_OFFSET(0xac)
#define CM_DIV_M6_DPLL_CORE		CM_WKUP_OFFSET(0xb8)

/*
 * CM_CLKMODE_DPLL_PER register
 */
#define CM_CLKMODE_DPLL_PER_DPLL_EN_SHIFT		0
#define CM_CLKMODE_DPLL_PER_DPLL_EN_WIDTH		3
#define CM_CLKMODE_DPLL_PER_DPLL_EN_STOP		1
#define CM_CLKMODE_DPLL_PER_DPLL_EN_BYPASS		4
#define CM_CLKMODE_DPLL_PER_DPLL_EN_BYPASS_LOW		5
#define CM_CLKMODE_DPLL_PER_DPLL_EN_ENABLE		7
#define CM_CLKMODE_DPLL_PER_DPLL_SSC_EN_SHIFT		12
#define CM_CLKMODE_DPLL_PER_DPLL_SSC_EN_WIDTH		1
#define CM_CLKMODE_DPLL_PER_DPLL_SSC_ACK_SHIFT		13
#define CM_CLKMODE_DPLL_PER_DPLL_SSC_ACK_WIDTH		1
#define CM_CLKMODE_DPLL_PER_DPLL_SSC_DOWNSPREAD_SHIFT	14
#define CM_CLKMODE_DPLL_PER_DPLL_SSC_DOWNSPREAD_WIDTH	1
#define CM_CLKMODE_DPLL_PER_DPLL_SSC_TYPE_SHIFT		15
#define CM_CLKMODE_DPLL_PER_DPLL_SSC_TYPE_WIDTH		1


/*
 * CM_CLKSEL_DPLL_CORE register
 */
#define CM_CLKSEL_DPLL_CORE_DPLL_DIV_SHIFT	0
#define CM_CLKSEL_DPLL_CORE_DPLL_DIV_WIDTH	7
#define CM_CLKSEL_DPLL_CORE_DPLL_MULT_SHIFT	8
#define CM_CLKSEL_DPLL_CORE_DPLL_MULT_WIDTH	11

/*
 * CM_CLKSEL_DPLL_PERIPH register
 */
#define CM_CLKSEL_DPLL_PERIPH_DPLL_DIV_SHIFT 	0
#define CM_CLKSEL_DPLL_PERIPH_DPLL_DIV_WIDTH 	8
#define CM_CLKSEL_DPLL_PERIPH_DPLL_MULT_SHIFT 	8
#define CM_CLKSEL_DPLL_PERIPH_DPLL_MULT_WIDTH 	12

/*
 * CM_IDLEST_DPLL_PER register
 */
#define CM_IDLEST_DPLL_PER_ST_DPLL_CLK_SHIFT	0
#define CM_IDLEST_DPLL_PER_ST_DPLL_CLK_WIDTH	1
#define CM_IDLEST_DPLL_PER_ST_DPLL_CLK_LOCKED	1
#define CM_IDLEST_DPLL_PER_ST_DPLL_CLK_UNLOCKED	0

#define CM_IDLEST_DPLL_PER_ST_MN_BYPASS_SHIFT	8
#define CM_IDLEST_DPLL_PER_ST_MN_BYPASS_WIDTH	1
#define CM_IDLEST_DPLL_PER_ST_MN_BYPASS_TRUE	1
#define CM_IDLEST_DPLL_PER_ST_MN_BYPASS_FALSE	0

/*
 * CM_DIV_M2_DPLL_PER register
 */
#define CM_DIV_M2_DPLL_PER_DIV_SHIFT		0
#define CM_DIV_M2_DPLL_PER_DIV_WIDTH		7
#define CM_DIV_M2_DPLL_PER_DIVCHACK_SHIFT	7
#define CM_DIV_M2_DPLL_PER_DIVCHACK_WIDTH	1
#define CM_DIV_M2_DPLL_PER_DIVCHACK_TRUE	1
#define CM_DIV_M2_DPLL_PER_GATE_SHIFT		8
#define CM_DIV_M2_DPLL_PER_GATE_WIDTH		1
#define CM_DIV_M2_DPLL_PER_GATE_ALWAYS		1
#define CM_DIV_M2_DPLL_PER_GATE_AUTO		0
#define CM_DIV_M2_DPLL_PER_STATUS_SHIFT		9
#define CM_DIV_M2_DPLL_PER_STATUS_WIDTH		1
#define CM_DIV_M2_DPLL_PER_STATUS_DISABLED	0
#define CM_DIV_M2_DPLL_PER_STATUS_ENABLED	1


/*
 * CM_DIV_Mx_DPLL_CORE register
 */
#define CM_DIV_Mx_DPLL_CORE_DIV_SHIFT		0
#define CM_DIV_Mx_DPLL_CORE_DIV_WIDTH		5
#define CM_DIV_Mx_DPLL_CORE_DIVCHACK_SHIFT	5
#define CM_DIV_Mx_DPLL_CORE_DIVCHACK_WIDTH	1
#define CM_DIV_Mx_DPLL_CORE_DIVCHACK_TRUE	1
#define CM_DIV_Mx_DPLL_CORE_GATE_SHIFT		8
#define CM_DIV_Mx_DPLL_CORE_GATE_WIDTH		1
#define CM_DIV_Mx_DPLL_CORE_GATE_ALWAYS		1
#define CM_DIV_Mx_DPLL_CORE_GATE_AUTO		0
#define CM_DIV_Mx_DPLL_CORE_STATUS_SHIFT	9
#define CM_DIV_Mx_DPLL_CORE_STATUS_WIDTH	1
#define CM_DIV_Mx_DPLL_CORE_STATUS_DISABLED	0
#define CM_DIV_Mx_DPLL_CORE_STATUS_ENABLED	1
#define CM_DIV_Mx_DPLL_CORE_POWER_SHIFT		12
#define CM_DIV_Mx_DPLL_CORE_POWER_WIDTH		1
#define CM_DIV_Mx_DPLL_CORE_POWER_KEEP		0
#define CM_DIV_Mx_DPLL_CORE_POWER_AUTO		1




/*
 * auxiliary function
 */

#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define _F_MASK(regname,fieldname) \
    GENMASK(regname##_##fieldname##_SHIFT+regname##_##fieldname##_WIDTH - 1,regname##_##fieldname##_SHIFT)

#define _F_MAX(regname,fieldname) \
    GENMASK(regname##_##fieldname##_SHIFT+regname##_##fieldname##_WIDTH - 1,0)

#define _F_CONST(regname,fieldname,constname) \
    (regname##_##fieldname##_##constname<<regname##_##fieldname##_SHIFT)

#define _F_CHECK(regname,fieldname,valname,value) \
    (((value) &GENMASK(regname##_##fieldname##_SHIFT+regname##_##fieldname##_WIDTH - 1,regname##_##fieldname##_SHIFT)) == \
    (regname##_##fieldname##_##valname<<regname##_##fieldname##_SHIFT))

#define _F_VAR(regname,fieldname,value) \
    (((value)&GENMASK(regname##_##fieldname##_WIDTH - 1,0))<<regname##_##fieldname##_SHIFT)

#define _F_GET(regname,fieldname,value) \
    (((value)>>regname##_##fieldname##_SHIFT)&GENMASK(regname##_##fieldname##_WIDTH - 1,0))


#define _REG(x)	((x)/4)



static uint64_t am335x_prcm_read(void *opaque, hwaddr addr, unsigned size)
{
	Am335xPrcmState *ps = AM335X_PRCM(opaque);

	return ps->regs[_REG(addr)];
}

static void am335x_prcm_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	Am335xPrcmState *ps = AM335X_PRCM(opaque);
	uint32_t value;

	switch(addr){
	case CM_CLKMODE_DPLL_PER:
		if(_F_CHECK(CM_CLKMODE_DPLL_PER,DPLL_EN,BYPASS,data)){
			ps->regs[_REG(CM_IDLEST_DPLL_PER)] =
					_F_CONST(CM_IDLEST_DPLL_PER,ST_DPLL_CLK,UNLOCKED)|
					_F_CONST(CM_IDLEST_DPLL_PER,ST_MN_BYPASS,TRUE);
		} else if(_F_CHECK(CM_CLKMODE_DPLL_PER,DPLL_EN,ENABLE,data)){
			ps->regs[_REG(CM_IDLEST_DPLL_PER)] =
					_F_CONST(CM_IDLEST_DPLL_PER,ST_DPLL_CLK,LOCKED)|
					_F_CONST(CM_IDLEST_DPLL_PER,ST_MN_BYPASS,FALSE);
		}
		break;
	case CM_DIV_M2_DPLL_PER:
		value = data ^ ps->regs[_REG(addr)];
		if(value & _F_MASK(CM_DIV_M2_DPLL_PER,DIV)){
			ps->regs[_REG(addr)] &= 
				~_F_MASK(CM_DIV_M2_DPLL_PER,DIV);
			ps->regs[_REG(addr)] |= 
				_F_VAR(CM_DIV_M2_DPLL_PER,DIV,_F_GET(CM_DIV_M2_DPLL_PER,DIV,data));
			ps->regs[_REG(addr)] ^=
				_F_CONST(CM_DIV_M2_DPLL_PER,DIVCHACK,TRUE);
		}

		if(value & _F_CONST(CM_DIV_M2_DPLL_PER,GATE,ALWAYS)){
			ps->regs[_REG(addr)] |= _F_CONST(CM_DIV_M2_DPLL_PER,STATUS,ENABLED);
		}
		
		break;
	case CM_DIV_M4_DPLL_CORE:
		value = data ^ ps->regs[_REG(addr)];
		if(value & _F_MASK(CM_DIV_Mx_DPLL_CORE,DIV)){
			ps->regs[_REG(addr)] &= 
				~_F_MASK(CM_DIV_Mx_DPLL_CORE,DIV);
			ps->regs[_REG(addr)] |= 
				_F_VAR(CM_DIV_Mx_DPLL_CORE,DIV,_F_GET(CM_DIV_Mx_DPLL_CORE,DIV,data));
			ps->regs[_REG(addr)] ^=
				_F_CONST(CM_DIV_Mx_DPLL_CORE,DIVCHACK,TRUE);
		}
		
		if(value & _F_CONST(CM_DIV_Mx_DPLL_CORE,GATE,ALWAYS)){
			ps->regs[_REG(addr)] |= _F_CONST(CM_DIV_Mx_DPLL_CORE,STATUS,ENABLED);
		}
		break;
	default:
		ps->regs[_REG(addr)] = data;
		break;
	}

}

static const MemoryRegionOps am335x_prcm_ops = {
	.read = am335x_prcm_read,
	.write = am335x_prcm_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static void am335x_prcm_init(Object *obj)
{
	Am335xPrcmState *ps = AM335X_PRCM(obj);

	memory_region_init_io(&ps->iomem, OBJECT(ps), &am335x_prcm_ops, ps,
			      TYPE_AM335X_PRCM, PRCM_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ps), &ps->iomem);
}

static void am335x_prcm_realize(DeviceState *dev, Error **errp)
{
	Am335xPrcmState *ps = AM335X_PRCM(dev);

	sysbus_mmio_map(SYS_BUS_DEVICE(ps), 0, ps->addr);
}

static void am335x_prcm_reset(DeviceState *dev)
{
	Am335xPrcmState *ps = AM335X_PRCM(dev);

	ps->regs[_REG(CM_CLKSEL_DPLL_CORE)] = 
		_F_VAR(CM_CLKSEL_DPLL_CORE,DPLL_DIV,2)|
		_F_VAR(CM_CLKSEL_DPLL_CORE,DPLL_MULT,250);
}

static Property am335x_prcm_properties[] = {
	DEFINE_PROP_UINT32("addr",Am335xPrcmState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void am335x_prcm_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = am335x_prcm_realize;
	dc->props = am335x_prcm_properties;
	dc->reset = am335x_prcm_reset;
}

static const TypeInfo am335x_prcm_info = {
	.name = TYPE_AM335X_PRCM,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_init = am335x_prcm_init,
	.instance_size = sizeof(Am335xPrcmState),
	.class_init = am335x_prcm_class_init,
};

static void am335x_prcm_register_types(void)
{
	type_register_static(&am335x_prcm_info);
}

type_init(am335x_prcm_register_types)
