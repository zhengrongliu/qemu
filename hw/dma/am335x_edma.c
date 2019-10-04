/*
 * am335x_edma.c
 *
 *  Created on: Apr 15, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "exec/address-spaces.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/irq.h"
#include "hw/sysbus.h"

#define NR_EVENTS 64

#define TYPE_AM335X_EDMA "am335x-edma"

#define AM335X_EDMA(obj)  \
	OBJECT_CHECK(Am335xEdmaState, (obj), TYPE_AM335X_EDMA)

struct edma_param_set;

typedef struct Am335xEdmaState {
	/* <private> */
	SysBusDevice parent_obj;

	/* <public> */
	MemoryRegion iomem;
	MemoryRegion prmset_mem;

	qemu_irq ccint_irq;
	qemu_irq mpint_irq;
	qemu_irq errint_irq;

	uint32_t addr;

	uint64_t reg_er;
	uint64_t reg_eer;
	uint64_t reg_esr;
	uint64_t reg_cer;

	struct edma_param_set *prmset;

}Am335xEdmaState;


/*
 * dma channel controller
 */
#define EDMA_CC_PID		0x0
#define EDMA_CC_CCCFG		0x4
#define EDMA_CC_SYSCONFIG	0x10
#define EDMA_CC_DCHMAP(x)	(0x100 + 0x4*(x))
#define EDMA_CC_QCHMAP(x)	(0x200 + 0x4*(x))
#define EDMA_CC_DMAQNUM(x)	(0x240 + 0x4*(x))
#define EDMA_CC_QDMAQNUM	0x260
#define EDMA_CC_QUEPRI		0x284
#define EDMA_CC_EMR		0x300
#define EDMA_CC_EMRH		0x304
#define EDMA_CC_EMCR		0x308
#define EDMA_CC_EMCRH		0x30c
#define EDMA_CC_QEMR		0x310
#define EDMA_CC_QEMCR		0x314
#define EDMA_CC_CCERR		0x318
#define EDMA_CC_CCERRCLR	0x31c
#define EDMA_CC_EEVAL		0x320
#define EDMA_CC_DRAE0		0x340
#define EDMA_CC_DRAEH0		0x344
#define EDMA_CC_DRAE1		0x348
#define EDMA_CC_DRAEH1		0x34c
#define EDMA_CC_DRAE2		0x350
#define EDMA_CC_DRAEH2		0x354
#define EDMA_CC_DRAE3		0x358
#define EDMA_CC_DRAEH3		0x35c
#define EDMA_CC_DRAE4		0x360
#define EDMA_CC_DRAEH4		0x364
#define EDMA_CC_DRAE5		0x368
#define EDMA_CC_DRAEH5		0x36c
#define EDMA_CC_DRAE6		0x370
#define EDMA_CC_DRAEH6		0x374
#define EDMA_CC_DRAE7		0x378
#define EDMA_CC_DRAEH7		0x37c
#define EDMA_CC_QRAE(x)		(0x380 + 0x4*(x))
#define EDMA_CC_EQE(x,y)	(0x400 + (16*(x)+(y))*0x4)
#define EDMA_CC_QSTAT(x)	(0x600 + 0x4*(x))
#define EDMA_CC_QWMTHRA		0x620
#define EDMA_CC_CCSTAT		0x640
#define EDMA_CC_MPFAR		0x800
#define EDMA_CC_MPFSR		0x804
#define EDMA_CC_MPFCR		0x808
#define EDMA_CC_MPPAG		0x80c
#define EDMA_CC_MPPA(x)		(0x810 + 0x4*(x))
#define EDMA_CC_ER		0x1000
#define EDMA_CC_ERH		0x1004
#define EDMA_CC_ECR		0x1008
#define EDMA_CC_ECRH		0x100c
#define EDMA_CC_ESR		0x1010
#define EDMA_CC_ESRH		0x1014
#define EDMA_CC_CER		0x1018
#define EDMA_CC_CERH		0x101c
#define EDMA_CC_EER		0x1020
#define EDMA_CC_EERH		0x1024
#define EDMA_CC_EECR		0x1028
#define EDMA_CC_EECRH		0x102c
#define EDMA_CC_EESR		0x1030
#define EDMA_CC_EESRH		0x1034
#define EDMA_CC_SER		0x1038
#define EDMA_CC_SERH		0x103c
#define EDMA_CC_SECR		0x1040
#define EDMA_CC_SECRH		0x1044
#define EDMA_CC_IER		0x1050
#define EDMA_CC_IERH		0x1054
#define EDMA_CC_IECR		0x1058
#define EDMA_CC_IECRH		0x105c
#define EDMA_CC_IESR		0x1060
#define EDMA_CC_IESRH		0x1064
#define EDMA_CC_IPR		0x1068
#define EDMA_CC_IPRH		0x106c
#define EDMA_CC_ICR		0x1070
#define EDMA_CC_ICRH		0x1074
#define EDMA_CC_IEVAL		0x1078
#define EDMA_CC_QER		0x1080
#define EDMA_CC_QEER		0x1084
#define EDMA_CC_QEECR		0x1088
#define EDMA_CC_QEESR		0x108c
#define EDMA_CC_QSER		0x1090
#define EDMA_CC_QSECR		0x1094


#define EDMA_CC_Q0E0		0x400
#define EDMA_CC_Q0E1		0x404
#define EDMA_CC_Q0E2		0x408
#define EDMA_CC_Q0E3		0x40c
#define EDMA_CC_Q0E4		0x410
#define EDMA_CC_Q0E5		0x414
#define EDMA_CC_Q0E6		0x418
#define EDMA_CC_Q0E7		0x41c
#define EDMA_CC_Q0E8		0x420
#define EDMA_CC_Q0E9		0x424
#define EDMA_CC_Q0E10		0x428
#define EDMA_CC_Q0E11		0x42c
#define EDMA_CC_Q0E12		0x430
#define EDMA_CC_Q0E13		0x434
#define EDMA_CC_Q0E14		0x438
#define EDMA_CC_Q0E15		0x43c


/*
 * dma transfer controller
 */
#define EDMA_TC_PID		0x0
#define EDMA_TC_TCCFG		0x4
#define EDMA_TC_SYSCONFIG	0x10
#define EDMA_TC_TCSTAT		0x100
#define EDMA_TC_ERRSTAT		0x120
#define EDMA_TC_ERREN		0x124
#define EDMA_TC_ERRCLR		0x128
#define EDMA_TC_ERRDET		0x12c
#define EDMA_TC_ERRCMD		0x130
#define EDMA_TC_RDRATE		0x140
#define EDMA_TC_SAOPT		0x240
#define EDMA_TC_SASRC		0x244
#define EDMA_TC_SACNT		0x248
#define EDMA_TC_SADST		0x24c
#define EDMA_TC_SABIDX		0x250
#define EDMA_TC_SAMPPRXY	0x254
#define EDMA_TC_SACNTRLD	0x258
#define EDMA_TC_SASRCBREF	0x25c
#define EDMA_TC_SADSTBREF	0x260
#define EDMA_TC_DFCNTRLD	0x280
#define EDMA_TC_DFSRCBREF	0x284
#define EDMA_TC_DFDSTBREF	0x288
#define EDMA_TC_DFOPT0		0x300
#define EDMA_TC_DFSRC0		0x304
#define EDMA_TC_DFCNT0		0x308
#define EDMA_TC_DFDST0		0x30c
#define EDMA_TC_DFBIDX0		0x310
#define EDMA_TC_DFMPPRXY0	0x314
#define EDMA_TC_DFOPT1		0x340
#define EDMA_TC_DFSRC1		0x344
#define EDMA_TC_DFCNT1		0x348
#define EDMA_TC_DFDST1		0x34c
#define EDMA_TC_DFBIDX1		0x350
#define EDMA_TC_DFMPPRXY1	0x354
#define EDMA_TC_DFOPT2		0x380
#define EDMA_TC_DFSRC2		0x384
#define EDMA_TC_DFCNT2		0x388
#define EDMA_TC_DFDST2		0x38c
#define EDMA_TC_DFBIDX2		0x390
#define EDMA_TC_DFMPPRXY2	0x394
#define EDMA_TC_DFOPT3		0x3c0
#define EDMA_TC_DFSRC3		0x3c4
#define EDMA_TC_DFCNT3		0x3c8
#define EDMA_TC_DFDST3		0x3cc
#define EDMA_TC_DFBIDX3		0x3d0
#define EDMA_TC_DFMPPRXY3	0x3d4



/*
 * dma channel map register
 */
#define EDMA_CC_DCHMAP_PAENTRY_SHIFT	5
#define EDMA_CC_DCHMAP_PAENTRY_WIDTH	8

/*
 * dma q channel map register
 */
#define EDMA_CC_QCHMAP_PAENTRY_SHIFT	5
#define EDMA_CC_QCHMAP_PAENTRY_WIDTH	8
#define EDMA_CC_QCHMAP_TRWORD_SHIFT	2
#define EDMA_CC_QCHMAP_TRWORD_WIDTH	3

/*
 * dma channel queue number register
 */
#define EDMA_CC_DMAQNUM_E0_SHIFT		0
#define EDMA_CC_DMAQNUM_E0_WIDTH		3
#define EDMA_CC_DMAQNUM_E1_SHIFT		4
#define EDMA_CC_DMAQNUM_E1_WIDTH		3
#define EDMA_CC_DMAQNUM_E2_SHIFT		8
#define EDMA_CC_DMAQNUM_E2_WIDTH		3
#define EDMA_CC_DMAQNUM_E3_SHIFT		12
#define EDMA_CC_DMAQNUM_E3_WIDTH		3
#define EDMA_CC_DMAQNUM_E4_SHIFT		16
#define EDMA_CC_DMAQNUM_E4_WIDTH		3
#define EDMA_CC_DMAQNUM_E5_SHIFT		20
#define EDMA_CC_DMAQNUM_E5_WIDTH		3
#define EDMA_CC_DMAQNUM_E6_SHIFT		24
#define EDMA_CC_DMAQNUM_E6_WIDTH		3
#define EDMA_CC_DMAQNUM_E7_SHIFT		28
#define EDMA_CC_DMAQNUM_E7_WIDTH		3

/*
 * dma queue priority register
 */
#define EDMA_CC_QUEPRI_PRIQ0_SHIFT		0
#define EDMA_CC_QUEPRI_PRIQ0_WIDTH		3
#define EDMA_CC_QUEPRI_PRIQ1_SHIFT		4
#define EDMA_CC_QUEPRI_PRIQ1_WIDTH		3
#define EDMA_CC_QUEPRI_PRIQ2_SHIFT		8
#define EDMA_CC_QUEPRI_PRIQ2_WIDTH		3

/*
 * dma error register
 */
#define EDMA_CC_CCERR_QTHRXCD0		BIT(0)
#define EDMA_CC_CCERR_QTHRXCD1		BIT(1)
#define EDMA_CC_CCERR_QTHRXCD2		BIT(2)
#define EDMA_CC_CCERR_TCCERR		BIT(16)

/*
 * dma eeval register
 */
#define EDMA_CC_EEVAL_EVAL		BIT(0)

/*
 * dma event queue entry register
 */
#define EDMA_CC_QXEY_ENUM_SHIFT		0
#define EDMA_CC_QXEY_ENUM_WIDTH		6
#define EDMA_CC_QXEY_ETYPE_SHIFT	6
#define EDMA_CC_QXEY_ETYPE_WIDTH	2
#define EDMA_CC_QXEY_ETYPE_EVT_TRG	0
#define EDMA_CC_QXEY_ETYPE_AUTO_TRG	1

/*
 * dma queue status register
 */
#define EDMA_CC_QSTAT_STRTPTR_SHIFT	0
#define EDMA_CC_QSTAT_STRTPTR_WIDTH	4
#define EDMA_CC_QSTAT_NUMVAL_SHIFT	8
#define EDMA_CC_QSTAT_NUMVAL_WIDTH	5
#define EDMA_CC_QSTAT_WM_SHIFT		16
#define EDMA_CC_QSTAT_WM_WIDTH		5
#define EDMA_CC_QSTAT_THRXCD_SHIFT	24
#define EDMA_CC_QSTAT_THRXCD_WIDTH	1

/*
 * dma queue watermark threshold A register
 */
#define EDMA_CC_QWMTHRA_Q0_SHIFT		0
#define EDMA_CC_QWMTHRA_Q0_WIDTH		5
#define EDMA_CC_QWMTHRA_Q1_SHIFT		8
#define EDMA_CC_QWMTHRA_Q1_WIDTH		5
#define EDMA_CC_QWMTHRA_Q2_SHIFT		16
#define EDMA_CC_QWMTHRA_Q2_WIDTH		5

/*
 * dma status register
 */
#define EDMA_CC_CCSTAT_EVTACTV		BIT(0)
#define EDMA_CC_CCSTAT_QEVTACTV		BIT(1)
#define EDMA_CC_CCSTAT_TRACTV		BIT(2)
#define EDMA_CC_CCSTAT_ACTV		BIT(3)
#define EDMA_CC_CCSTAT_COMPACTV_SHIFT	8
#define EDMA_CC_CCSTAT_COMPACTV_WIDTH	6
#define EDMA_CC_CCSTAT_QUEACTV0		BIT(16)
#define EDMA_CC_CCSTAT_QUEACTV1		BIT(17)
#define EDMA_CC_CCSTAT_QUEACTV2		BIT(18)

/*
 * dma memory protection fault status register
 */
#define EDMA_CC_MPFSR_UXE			BIT(0)
#define EDMA_CC_MPFSR_UWE			BIT(1)
#define EDMA_CC_MPFSR_URE			BIT(2)
#define EDMA_CC_MPFSR_SXE			BIT(3)
#define EDMA_CC_MPFSR_SWE			BIT(4)
#define EDMA_CC_MPFSR_SRE			BIT(5)
#define EDMA_CC_MPFSR_FID_SHIFT		9
#define EDMA_CC_MPFSR_FID_WIDTH		4

/*
 * dma memory protection fault clear register
 */
#define EDMA_CC_MPFCR_MPFCLR		BIT(0)

/*
 * dma memory protection allow register
 */
#define EDMA_CC_MPPA_UX			BIT(0)
#define EDMA_CC_MPPA_UW			BIT(1)
#define EDMA_CC_MPPA_UR			BIT(2)
#define EDMA_CC_MPPA_SX			BIT(3)
#define EDMA_CC_MPPA_SW			BIT(4)
#define EDMA_CC_MPPA_SR			BIT(5)
#define EDMA_CC_MPPA_EXT		BIT(9)
#define EDMA_CC_MPPA_AID_SHIFT		10
#define EDMA_CC_MPPA_AID_WIDTH		6

/*
 * dma tc state register
 */
#define EDMA_TC_TCSTAT_DFSTRTPTR_SHIFT	12
#define EDMA_TC_TCSTAT_DFSTRTPTR_WIDTH	2
#define EDMA_TC_TCSTAT_DSTACTV_SHIFT	4
#define EDMA_TC_TCSTAT_DSTACTV_WIDTH	3
#define EDMA_TC_TCSTAT_WSACTV		BIT(2)
#define EDMA_TC_TCSTAT_SRCACTV		BIT(1)
#define EDMA_TC_TCSTAT_PROGBUSY		BIT(0)

/*
 * dma tc error register
 */
#define EDMA_TC_ERR_MMRAERR		BIT(3)
#define EDMA_TC_ERR_TRERR		BIT(2)
#define EDMA_TC_ERR_BUSERR		BIT(0)


/*
 * dma tc errdet register
 */
#define EDMA_TC_ERRDET_TCCHEN		BIT(17)
#define EDMA_TC_ERRDET_TCINTEN		BIT(16)
#define EDMA_TC_ERRDET_TCC_SHIFT	8
#define EDMA_TC_ERRDET_TCC_WIDTH	6
#define EDMA_TC_ERRDET_STAT_SHIFT	0
#define EDMA_TC_ERRDET_STAT_WDITH	4

/*
 * dma tc command register
 */
#define EDMA_TC_ERRCMD_EVAL		BIT(0)

/*
 * dma tc read rate register
 */
#define EDMA_TC_RDRATE_ASAP		0
#define EDMA_TC_RDRATE_CYCLES_4		1
#define EDMA_TC_RDRATE_CYCLES_8		2
#define EDMA_TC_RDRATE_CYCLES_16	3
#define EDMA_TC_RDRATE_CYCLES_32	4

/*
 * dma tc source active options register
 */
#define EDMA_TC_SAOPT_TCCHEN		BIT(22)
#define EDMA_TC_SAOPT_TCINTEN		BIT(20)
#define EDMA_TC_SAOPT_TCC_SHIFT		12
#define EDMA_TC_SAOPT_TCC_WIDTH		6
#define EDMA_TC_SAOPT_FWID_SHIFT	8
#define EDMA_TC_SAOPT_FWID_WIDTH	3
#define EDMA_TC_SAOPT_PRI_SHIFT		4
#define EDMA_TC_SAOPT_PRI_WIDTH		3
#define EDMA_TC_SAOPT_DAM		BIT(1)
#define EDMA_TC_SAOPT_SAM		BIT(0)


/*
 * edma params set entry options
 */
#define EDMA_PARAM_SET_OPT_SAM_INCR	0
#define EDMA_PARAM_SET_OPT_SAM_CONST	BIT(0)
#define EDMA_PARAM_SET_OPT_DAM_INCR	0
#define EDMA_PARAM_SET_OPT_DAM_CONST	BIT(1)
#define EDMA_PARAM_SET_OPT_A_SYNC	0
#define EDMA_PARAM_SET_OPT_AB_SYNC	BIT(2)
#define EDMA_PARAM_SET_OPT_STATIC	BIT(3)
#define EDMA_PARAM_SET_OPT_FIFO_8	(0<<8)
#define EDMA_PARAM_SET_OPT_FIFO_16	(1<<8)
#define EDMA_PARAM_SET_OPT_FIFO_32	(2<<8)
#define EDMA_PARAM_SET_OPT_FIFO_64	(3<<8)
#define EDMA_PARAM_SET_OPT_FIFO_128	(4<<8)
#define EDMA_PARAM_SET_OPT_FIFO_256	(5<<8)
#define EDMA_PARAM_SET_OPT_TCCMODE_NORMAL	0
#define EDMA_PARAM_SET_OPT_TCCMODE_EARLY	BIT(11)
#define EDMA_PARAM_SET_OPT_TCC(x)	((x)<<12)
#define EDMA_PARAM_SET_OPT_TCINTEN	BIT(20)
#define EDMA_PARAM_SET_OPT_ITCINTEN	BIT(21)
#define EDMA_PARAM_SET_OPT_TCCHEN	BIT(22)
#define EDMA_PARAM_SET_OPT_ITCCHEN	BIT(23)
#define EDMA_PARAM_SET_OPT_PRIVID_SHIFT	24
#define EDMA_PARAM_SET_OPT_PRIVID_WIDTH	4
#define EDMA_PARAM_SET_OPT_PRIV		BIT(31)


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



#define EDMA_PARAM_SET_OFFSET	0x4000


#define EDMA_IO_REGION_SIZE 	0x2000
#define EDMA_PARAM_SET_SIZE	0x2000


struct edma_param_set{
	uint32_t opt;
	uint32_t src;
	uint16_t bcnt;
	uint16_t acnt;
	uint32_t dst;
	uint16_t dstbidx;
	uint16_t srcbidx;
	uint16_t bcntrld;
	uint16_t link;
	uint16_t dstcidx;
	uint16_t srccidx;
	uint16_t reserved;
	uint16_t ccnt;
}__packed;

#if 0
static void edma_tc_xfer_exec(Am335xEdmaState *ds,struct edma_param_set *set)
{

}
#endif
static uint64_t edma_read(void *opaque, hwaddr offset, unsigned size)
{
	return 0;
}

static void edma_write(void *opaque, hwaddr offset, uint64_t value,
	unsigned size)
{

}
#if 0
static void edma_event_update(struct Am335xEdmaState *ds)
{

}
#endif
static void edma_event_handler(void *opaque, int irq, int level)
{
	Am335xEdmaState * ds = AM335X_EDMA(opaque);

	if(level){
		ds->reg_eer |= 1<<(uint64_t)irq;
	}

}

static const MemoryRegionOps edma_ops = {
	.read = edma_read,
	.write = edma_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static Property edma_properties[] = {
	DEFINE_PROP_UINT32("addr",Am335xEdmaState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void edma_reset(DeviceState *opaque)
{

}


static void edma_init(Object *obj)
{
	Am335xEdmaState * ds = AM335X_EDMA(obj);


	sysbus_init_irq(SYS_BUS_DEVICE(ds),&(ds->ccint_irq));
	sysbus_init_irq(SYS_BUS_DEVICE(ds),&(ds->mpint_irq));
	sysbus_init_irq(SYS_BUS_DEVICE(ds),&(ds->errint_irq));

	memory_region_init_io(&ds->iomem,OBJECT(ds),&edma_ops,ds,
		TYPE_AM335X_EDMA,EDMA_IO_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ds),&ds->iomem);

	memory_region_init_ram(&ds->prmset_mem,OBJECT(ds),TYPE_AM335X_EDMA".prmset",
			EDMA_PARAM_SET_SIZE,NULL);


	qdev_init_gpio_in(DEVICE(ds), edma_event_handler, NR_EVENTS);


}

static void edma_realize(DeviceState *dev, Error **errp)
{
	Am335xEdmaState * ds = AM335X_EDMA(dev);

	sysbus_mmio_map(SYS_BUS_DEVICE(ds), 0, ds->addr);

	memory_region_add_subregion(get_system_memory(),
			ds->addr + EDMA_PARAM_SET_OFFSET,&ds->prmset_mem);
}

static void edma_class_init(ObjectClass *c ,void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = edma_realize;
	dc->reset = edma_reset;
	dc->props = edma_properties;
}

static const TypeInfo edma_type_info = {
	.name = TYPE_AM335X_EDMA,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(Am335xEdmaState),
	.instance_init = edma_init,
	.class_init = edma_class_init,
};


static void am335x_edma_register_types(void)
{
	type_register_static(&edma_type_info);
}

type_init(am335x_edma_register_types)


