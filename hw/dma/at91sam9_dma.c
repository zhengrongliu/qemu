/*
 * at91sam9_dma.c
 *
 *  Created on: Dec 10, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/dma/at91sam9_dma.h"

#define DMAC_REGION_SIZE 	0x200
#define DMAC_CHAN_REGION_SIZE	0x28

#define NR_PERIPH		16

#define DMAC_GCFG		0x00
#define DMAC_EN			0x04
#define DMAC_SREQ		0x08
#define DMAC_CREQ		0x0c
#define DMAC_LAST		0x10
#define DMAC_EBCIER		0x18
#define DMAC_EBCIDR		0x1c
#define DMAC_EBCIMR		0x20
#define DMAC_EBCISR		0x24
#define DMAC_CHER		0x28
#define DMAC_CHDR		0x2c
#define DMAC_CHSR		0x30
#define DMAC_CHAN_SADDR		0x00
#define DMAC_CHAN_DADDR		0x04
#define DMAC_CHAN_DSCR		0x08
#define DMAC_CHAN_CTRLA		0x0c
#define DMAC_CHAN_CTRLB		0x10
#define DMAC_CHAN_CFG		0x14
#define DMAC_CHAN_SPIP		0x18
#define DMAC_CHAN_DPIP		0x1c
#define DMAC_SADDR(x)		(0x3c + (x)*0x28 + DMAC_CHAN_SADDR)
#define DMAC_DADDR(x)		(0x3c + (x)*0x28 + DMAC_CHAN_DADDR)
#define DMAC_DSCR(x)		(0x3c + (x)*0x28 + DMAC_CHAN_DSCR)
#define DMAC_CTRLA(x)		(0x3c + (x)*0x28 + DMAC_CHAN_CTRLA)
#define DMAC_CTRLB(x)		(0x3c + (x)*0x28 + DMAC_CHAN_CTRLB)
#define DMAC_CFG(x)		(0x3c + (x)*0x28 + DMAC_CHAN_CFG)
#define DMAC_SPIP(x)		(0x3c + (x)*0x28 + DMAC_CHAN_SPIP)
#define DMAC_DPIP(x)		(0x3c + (x)*0x28 + DMAC_CHAN_DPIP)
#define DMAC_WPMR		0x1e4
#define DMAC_WPSR		0x1e8


/*
 * DMAC Global Configuration Register
 */
#define DMAC_GCFG_ARB_CFG_FIXED		(0<<4)
#define DMAC_GCFG_ARB_CFG_RR		(1<<4)
#define DMAC_GCFG_ARB_CFG_MASK		(1<<4)

/*
 * DMAC Enable Register
 */
#define DMAC_EN_ENABLE 			(0<<0)
#define DMAC_EN_DISABLE 		(1<<0)

/*
 * DMAC Software Single Request Register
 */
#define DMAC_SREG_SSREG(x)		(1<<((x)*2))
#define DMAC_SREG_DSREG(x)		(1<<((x)*2+1))

/*
 * DMAC Software Chunk Transfer Request Register
 */
#define DMAC_CREG_SCREG(x)		(1<<((x)*2))
#define DMAC_CREG_DCREG(x)		(1<<((x)*2+1))


/*
 * DMAC Software Last Transfer Flag Register
 */
#define DMAC_LAST_SLAST(x)		(1<<((x)*2))
#define DMAC_LAST_DLAST(x)		(1<<((x)*2+1))

/*
 * DMAC Error,Buffer Transfer and Chained Buffer Transfer Event
 */
#define DMAC_EVENT_BTC(x)		(1<<(x))
#define DMAC_EVENT_CBTC(x)		(1<<((x)+8))
#define DMAC_EVENT_ERR(x)		(1<<((x)+16))

/*
 * DMAC Channel Handler Enable Register
 */
#define DMAC_CHER_ENA(x)		(1<<(x))
#define DMAC_CHER_SUSP(x)		(1<<((x)+8))
#define DMAC_CHER_KEEP(x)		(1<<((x)+24))

/*
 * DMAC Channel Handler Disable Register
 */
#define DMAC_CHDR_DIS(x)		(1<<(x))
#define DMAC_CHDR_RES(x)		(1<<((x)+8))

/*
 * DMAC Channel Handler Status Register
 */
#define DMAC_CHSR_ENA(x)		(1<<(x))
#define DMAC_CHSR_SUSP(x)		(1<<((x)+8))
#define DMAC_CHSR_EMPT(x)		(1<<((x)+16))
#define DMAC_CHSR_STAL(x)		(1<<((x)+24))
#define DMAC_CHSR_ENA_SHIFT		0
#define DMAC_CHSR_ENA_WIDTH		8
#define DMAC_CHSR_ENA_MASK		((1<<DMAC_CHSR_ENA_WIDTH)-1)
#define DMAC_CHSR_SUSP_SHIFT		8
#define DMAC_CHSR_SUSP_WIDTH		8
#define DMAC_CHSR_SUSP_MASK		((1<<DMAC_CHSR_ENA_WIDTH)-1)
#define DMAC_CHSR_EMPT_SHIFT		16
#define DMAC_CHSR_EMPT_WIDTH		8
#define DMAC_CHSR_EMPT_MASK		((1<<DMAC_CHSR_ENA_WIDTH)-1)
#define DMAC_CHSR_STAL_SHIFT		24
#define DMAC_CHSR_STAL_WIDTH		8
#define DMAC_CHSR_STAL_MASK		((1<<DMAC_CHSR_ENA_WIDTH)-1)


/*
 * DMAC Descriptor Address Register
 */
#define DMAC_DSCR_AHB_IF0		(0<<0)
#define DMAC_DSCR_AHB_IF1		(1<<0)
#define DMAC_DSCR_DSCR_SHIFT		2
#define DMAC_DSCR_DSCR_WIDTH		30
#define DMAC_DSCR_DSCR_MASK		((1<<DMAC_DSCR_DSCR_WIDTH)-1)

/*
 * DMAC Channel Control A Register
 */
#define DMAC_CTRLA_BTSIZE_SHIFT		0
#define DMAC_CTRLA_BTSIZE_WIDTH		16
#define DMAC_CTRLA_BTSIZE_MASK		((1<<DMAC_CTRLA_BTSIZE_WIDTH)-1)
#define DMAC_CTRLA_SCSIZE_SHIFT		16
#define DMAC_CTRLA_SCSIZE_WIDTH		3
#define DMAC_CTRLA_SCSIZE_MASK		((1<<DMAC_CTRLA_SCSIZE_WIDTH)-1)
#define DMAC_CTRLA_SCSIZE_CHK_1		0
#define DMAC_CTRLA_SCSIZE_CHK_4		1
#define DMAC_CTRLA_SCSIZE_CHK_8		2
#define DMAC_CTRLA_SCSIZE_CHK_16	3
#define DMAC_CTRLA_DCSIZE_SHIFT		20
#define DMAC_CTRLA_DCSIZE_WIDTH		3
#define DMAC_CTRLA_DCSIZE_MASK		((1<<DMAC_CTRLA_DCSIZE_WIDTH)-1)
#define DMAC_CTRLA_DCSIZE_CHK_1		0
#define DMAC_CTRLA_DCSIZE_CHK_4		1
#define DMAC_CTRLA_DCSIZE_CHK_8		2
#define DMAC_CTRLA_DCSIZE_CHK_16	3
#define DMAC_CTRLA_SRC_WIDTH_SHIFT	24
#define DMAC_CTRLA_SRC_WIDTH_WIDTH	2
#define DMAC_CTRLA_SRC_WIDTH_MASK	((1<<DMAC_CTRLA_SRC_WIDTH_WIDTH)-1)
#define DMAC_CTRLA_SRC_WIDTH_BYTE	0
#define DMAC_CTRLA_SRC_WIDTH_HALF_WORD	1
#define DMAC_CTRLA_SRC_WIDTH_WORD	2
#define DMAC_CTRLA_DST_WIDTH_SHIFT	28
#define DMAC_CTRLA_DST_WIDTH_WIDTH	2
#define DMAC_CTRLA_DST_WIDTH_MASK	((1<<DMAC_CTRLA_DST_WIDTH_WIDTH)-1)
#define DMAC_CTRLA_DST_WIDTH_BYTE	0
#define DMAC_CTRLA_DST_WIDTH_HALF_WORD	1
#define DMAC_CTRLA_DST_WIDTH_WORD	2
#define DMAC_CTRLA_DONE_SHIFT		31
#define DMAC_CTRLA_DONE_WIDTH		1
#define DMAC_CTRLA_DONE_MASK		((1<<DMAC_CTRLA_DONE_WIDTH)-1)
#define DMAC_CTRLA_DONE_FALSE		1
#define DMAC_CTRLA_DONE_TRUE		1

/*
 * DMAC Channel Control B Register
 */
#define DMAC_CTRLB_SIF_SHIFT		0
#define DMAC_CTRLB_SIF_WIDTH		2
#define DMAC_CTRLB_SIF_MASK		((1<<DMAC_CTRLB_SIF_WIDTH)-1)
#define DMAC_CTRLB_SIF_AHB_IF0		0
#define DMAC_CTRLB_SIF_AHB_IF1		1
#define DMAC_CTRLB_DIF_SHIFT		4
#define DMAC_CTRLB_DIF_WIDTH		2
#define DMAC_CTRLB_DIF_MASK		((1<<DMAC_CTRLB_DIF_WIDTH)-1)
#define DMAC_CTRLB_DIF_AHB_IF0		0
#define DMAC_CTRLB_DIF_AHB_IF1		1
#define DMAC_CTRLB_SRC_PIP_SHIFT	8
#define DMAC_CTRLB_SRC_PIP_WIDTH	1
#define DMAC_CTRLB_SRC_PIP_MASK		((1<<DMAC_CTRLB_SRC_PIP_WIDTH)-1)
#define DMAC_CTRLB_DST_PIP_SHIFT	12
#define DMAC_CTRLB_DST_PIP_WIDTH	1
#define DMAC_CTRLB_DST_PIP_MASK		((1<<DMAC_CTRLB_DST_PIP_WIDTH)-1)
#define DMAC_CTRLB_SRC_DSCR_SHIFT	16
#define DMAC_CTRLB_SRC_DSCR_WIDTH	1
#define DMAC_CTRLB_SRC_DSCR_MASK	((1<<DMAC_CTRLB_SRC_DSCR_WIDTH)-1)
#define DMAC_CTRLB_DST_DSCR_SHIFT	20
#define DMAC_CTRLB_DST_DSCR_WIDTH	1
#define DMAC_CTRLB_DST_DSCR_MASK	((1<<DMAC_CTRLB_DST_DSCR_WIDTH)-1)
#define DMAC_CTRLB_FC_SHIFT		21
#define DMAC_CTRLB_FC_WIDTH		2
#define DMAC_CTRLB_FC_MASK		((1<<DMAC_CTRLB_FC_WIDTH)-1)
#define DMAC_CTRLB_FC_MEM2MEM		0
#define DMAC_CTRLB_FC_MEM2PER		1
#define DMAC_CTRLB_FC_PER2MEM		2
#define DMAC_CTRLB_FC_PER2PER		3
#define DMAC_CTRLB_SRC_INCR_SHIFT	24
#define DMAC_CTRLB_SRC_INCR_WIDTH	2
#define DMAC_CTRLB_SRC_INCR_MASK	((1<<DMAC_CTRLB_SRC_INCR_WIDTH)-1)
#define DMAC_CTRLB_SRC_INCR_INCREMENT	0
#define DMAC_CTRLB_SRC_INCR_DECREMENT	1
#define DMAC_CTRLB_SRC_INCR_FIXED		2
#define DMAC_CTRLB_DST_INCR_SHIFT	28
#define DMAC_CTRLB_DST_INCR_WIDTH	2
#define DMAC_CTRLB_DST_INCR_MASK	((1<<DMAC_CTRLB_DST_INCR_WIDTH)-1)
#define DMAC_CTRLB_DST_INCR_INCREMENT	0
#define DMAC_CTRLB_DST_INCR_DECREMENT	1
#define DMAC_CTRLB_DST_INCR_FIXED	2
#define DMAC_CTRLB_IEN_SHIFT		30
#define DMAC_CTRLB_IEN_WIDTH		1
#define DMAC_CTRLB_IEN_MASK		((1<<DMAC_CTRLB_IEN_WIDTH)-1)
#define DMAC_CTRLB_AUTO_SHIFT		31
#define DMAC_CTRLB_AUTO_WIDTH		1
#define DMAC_CTRLB_AUTO_MASK		((1<<DMAC_CTRLB_AUTO_WIDTH)-1)

/*
 * DMAC Channel Configuration Register
 */
#define DMAC_CFG_SRC_PER_SHIFT		0
#define DMAC_CFG_SRC_PER_WIDTH		4
#define DMAC_CFG_SRC_PER_MASK		((1<<DMAC_CFG_SRC_PER_WIDTH)-1)
#define DMAC_CFG_DST_PER_SHIFT		4
#define DMAC_CFG_DST_PER_WIDTH		4
#define DMAC_CFG_DST_PER_MASK		((1<<DMAC_CFG_DST_PER_WIDTH)-1)
#define DMAC_CFG_SRC_REP_SHIFT		8
#define DMAC_CFG_SRC_REP_WIDTH		1
#define DMAC_CFG_SRC_REP_MASK		((1<<DMAC_CFG_SRC_REP_WIDTH)-1)
#define DMAC_CFG_SRC_REP_CONTIGUOUS	0
#define DMAC_CFG_SRC_REP_RELOAD		1
#define DMAC_CFG_SRC_H2SEL_SHIFT	9
#define DMAC_CFG_SRC_H2SEL_WIDTH	1
#define DMAC_CFG_SRC_H2SEL_MASK		((1<<DMAC_CFG_SRC_H2SEL_WIDTH)-1)
#define DMAC_CFG_SRC_H2SEL_SW		0
#define DMAC_CFG_SRC_H2SEL_HW		1
#define DMAC_CFG_DST_REP_SHIFT		12
#define DMAC_CFG_DST_REP_WIDTH		1
#define DMAC_CFG_DST_REP_MASK		((1<<DMAC_CFG_DST_REP_WIDTH)-1)
#define DMAC_CFG_DST_REP_CONTIGUOUS	0
#define DMAC_CFG_DST_REP_RELOAD		1
#define DMAC_CFG_DST_H2SEL_SHIFT	13
#define DMAC_CFG_DST_H2SEL_WIDTH	1
#define DMAC_CFG_DST_H2SEL_MASK		((1<<DMAC_CFG_DST_H2SEL_WIDTH)-1)
#define DMAC_CFG_DST_H2SEL_SW		0
#define DMAC_CFG_DST_H2SEL_HW		1
#define DMAC_CFG_SOD_SHIFT		16
#define DMAC_CFG_SOD_WIDTH		1
#define DMAC_CFG_SOD_MASK		((1<<DMAC_CFG_SOD_WIDTH)-1)
#define DMAC_CFG_SOD_DISABLE		0
#define DMAC_CFG_SOD_ENABLE		1
#define DMAC_CFG_LOCK_IF_SHIFT		20
#define DMAC_CFG_LOCK_IF_WIDTH		1
#define DMAC_CFG_LOCK_IF_MASK		((1<<DMAC_CFG_LOCK_IF_WIDTH)-1)
#define DMAC_CFG_LOCK_IF_DISABLE	0
#define DMAC_CFG_LOCK_IF_ENABLE		1
#define DMAC_CFG_LOCK_B_SHIFT		21
#define DMAC_CFG_LOCK_B_WIDTH		1
#define DMAC_CFG_LOCK_B_MASK		((1<<DMAC_CFG_LOCK_B_WIDTH)-1)
#define DMAC_CFG_LOCK_B_DISABLE		0
#define DMAC_CFG_LOCK_B_ENABLE		1
#define DMAC_CFG_LOCK_IF_L_SHIFT	22
#define DMAC_CFG_LOCK_IF_L_WIDTH	1
#define DMAC_CFG_LOCK_IF_L_MASK		((1<<DMAC_CFG_LOCK_IF_L_WIDTH)-1)
#define DMAC_CFG_LOCK_IF_L_DISABLE	0
#define DMAC_CFG_LOCK_IF_L_ENABLE	1
#define DMAC_CFG_AHB_PROT_SHIFT		24
#define DMAC_CFG_AHB_PROT_WIDTH		3
#define DMAC_CFG_AHB_PROT_MASK		((1<<DMAC_CFG_AHB_PROT_WIDTH)-1)
#define DMAC_CFG_FIFOCFG_SHIFT		28
#define DMAC_CFG_FIFOCFG_WIDTH		2
#define DMAC_CFG_FIFOCFG_MASK		((1<<DMAC_CFG_FIFOCFG_WIDTH)-1)
#define DMAC_CFG_FIFOCFG_ALAP_CFG	0
#define DMAC_CFG_FIFOCFG_HALF_CFG	1
#define DMAC_CFG_FIFOCFG_ASAP_CFG	2

/*
 * DMAC Channel Source Picture-in-Picture Configuration Register
 */
#define DMAC_SPIP_SPIP_HOLD_SHIFT	0
#define DMAC_SPIP_SPIP_HOLD_WIDTH	16
#define DMAC_SPIP_SPIP_HOLD_MASK	((1<<DMAC_SPIP_SPIP_HOLD_WIDTH)-1)
#define DMAC_SPIP_SPIP_BOUNDARY_SHIFT	16
#define DMAC_SPIP_SPIP_BOUNDARY_WIDTH	10
#define DMAC_SPIP_SPIP_BOUNDARY_MASK	((1<<DMAC_SPIP_SPIP_BOUNDARY_WIDTH)-1)

/*
 * DMAC Channel Destination Picture-in-Picture Configuration Register
 */
#define DMAC_DPIP_DPIP_HOLD_SHIFT	0
#define DMAC_DPIP_DPIP_HOLD_WIDTH	16
#define DMAC_DPIP_DPIP_HOLD_MASK	((1<<DMAC_DPIP_DPIP_HOLD_WIDTH)-1)
#define DMAC_DPIP_DPIP_BOUNDARY_SHIFT	16
#define DMAC_DPIP_DPIP_BOUNDARY_WDITH	10
#define DMAC_DPIP_DPIP_BOUNDARY_MASK	((1<<DMAC_DPIP_DPIP_BOUNDARY_WIDTH)-1)

/*
 * DMAC Write Protection Mode Register
 */
#define DMAC_WPMR_WPEN_SHIFT		0
#define DMAC_WPMR_WPEN_WIDTH		1
#define DMAC_WPMR_WPEN_MASK		((1<<DMAC_WPMR_WPEN_WIDTH)-1)
#define DMAC_WPMR_WPKEY_SHIFT		8
#define DMAC_WPMR_WPKEY_WIDTH		24
#define DMAC_WPMR_WPKEY_MASK		((1<<DMAC_WPMR_WPKEY_WIDTH)-1)

/*
 * DMAC Write Protection Status Register
 */
#define DMAC_WPSR_WPVS_SHIFT		0
#define DMAC_WPSR_WPVS_WIDTH		1
#define DMAC_WPSR_WPVS_MASK		((1<<DMAC_WPSR_WPVS_WIDTH)-1)
#define DMAC_WPSR_WPVSRC_SHIFT		8
#define DMAC_WPSR_WPVSRC_WIDTH		16
#define DMAC_WPSR_WPVSRC_MASK		((1<<DMAC_WPSR_WPVSRC_WIDTH)-1)

#define __FIELD_MASK(regname,fieldname) \
    (regname##_##fieldname##_MASK<<regname##_##fieldname##_SHIFT)

#define __FIELD_VALUE_FIXED(regname,fieldname,valname) \
    (regname##_##fieldname##_##valname<<regname##_##fieldname##_SHIFT)

#define __FIELD_IS_SET(regname,fieldname,valname,value) \
    (((value) &(regname##_##fieldname##_MASK<<regname##_##fieldname##_SHIFT)) == \
    (regname##_##fieldname##_##valname<<regname##_##fieldname##_SHIFT))

#define __FIELD_VALUE_SET(regname,fieldname,value) \
    (((value)&regname##_##fieldname##_MASK)<<regname##_##fieldname##_SHIFT)

#define __FIELD_VALUE_GET(regname,fieldname,value) \
    (((value)>>regname##_##fieldname##_SHIFT)&regname##_##fieldname##_MASK)

typedef struct At91sam9DmacChanHwDesc{
	uint32_t saddr;
	uint32_t daddr;
	uint32_t ctrla;
	uint32_t ctrlb;
	uint32_t dscr;
} At91sam9DmacChanHwDesc;

static void dmac_update_irq(At91sam9DmacState *ds)
{
	uint32_t irq;

	irq = ds->reg_ebcisr & ds->reg_ebcimr;

	qemu_set_irq(ds->irq,!!irq);

}

static void dmac_chan_hwdesc_xfer(At91sam9DmacChan *chan)
{

	int width = 1;
	int size;
	int fc;
	uint8_t buf[4];

	fc 	= __FIELD_VALUE_GET(DMAC_CTRLB,FC,chan->ctrlb);
	size 	= __FIELD_VALUE_GET(DMAC_CTRLA,BTSIZE,chan->ctrla);

	switch(fc){
	case DMAC_CTRLB_FC_MEM2PER:
		width = 1<<__FIELD_VALUE_GET(DMAC_CTRLA,DST_WIDTH,chan->ctrla);
		break;
	case DMAC_CTRLB_FC_PER2MEM:
		width = 1<<__FIELD_VALUE_GET(DMAC_CTRLA,SRC_WIDTH,chan->ctrla);
		break;
	}

	if(width > sizeof(buf)){
		width = sizeof(buf);
	}

	while(size--){
		cpu_physical_memory_read(chan->saddr,buf,width);
		cpu_physical_memory_write(chan->daddr,buf,width);


		switch(__FIELD_VALUE_GET(DMAC_CTRLB,SRC_INCR,chan->ctrlb)){
		case DMAC_CTRLB_SRC_INCR_INCREMENT:
			chan->saddr += width;
			break;
		case DMAC_CTRLB_SRC_INCR_DECREMENT:
			chan->saddr -= width;
			break;
		}

		switch(__FIELD_VALUE_GET(DMAC_CTRLB,DST_INCR,chan->ctrlb)){
		case DMAC_CTRLB_DST_INCR_INCREMENT:
			chan->daddr += width;
			break;
		case DMAC_CTRLB_DST_INCR_DECREMENT:
			chan->daddr -= width;
			break;
		}
	}

	chan->ctrla &= ~(DMAC_CTRLA_BTSIZE_MASK<<DMAC_CTRLA_BTSIZE_SHIFT);
	chan->ctrla |= __FIELD_VALUE_FIXED(DMAC_CTRLA,DONE,TRUE);


	chan->dmac->reg_ebcisr |= DMAC_EVENT_BTC(chan->chan_id);


}

static void dmac_chan_start(At91sam9DmacChan * chan)
{
	At91sam9DmacChanHwDesc desc;
	uint32_t dscr;

	if(chan->dscr == 0){
		/*
		 * single desc
		 */
		dmac_chan_hwdesc_xfer(chan);

	} else {
		/*
		 * multi desc
		 */
		do {
			dscr = chan->dscr;

			cpu_physical_memory_read(dscr,&desc,sizeof(desc));

			chan->saddr = desc.saddr;
			chan->daddr = desc.daddr;
			chan->ctrla = desc.ctrla;
			chan->ctrlb = desc.ctrlb;
			chan->dscr = desc.dscr;

			dmac_chan_hwdesc_xfer(chan);

			dmac_update_irq(chan->dmac);

			desc.ctrla = chan->ctrla;

			cpu_physical_memory_write(dscr,&desc,sizeof(desc));

		} while(chan->dscr != 0);

	}

	chan->dmac->reg_chsr &= ~DMAC_CHSR_ENA(chan->chan_id);
	chan->dmac->reg_ebcisr |= DMAC_EVENT_CBTC(chan->chan_id);

	dmac_update_irq(chan->dmac);
}



static uint64_t dmac_chan_read(void *opaque, hwaddr offset, unsigned size)
{
	At91sam9DmacChan * chan = (At91sam9DmacChan*)opaque;


	switch(offset){
	case DMAC_CHAN_SADDR:
		return chan->saddr;
	case DMAC_CHAN_DADDR:
		return chan->daddr;
	case DMAC_CHAN_DSCR:
		return chan->dscr;
	case DMAC_CHAN_CTRLA:
		return chan->ctrla;
	case DMAC_CHAN_CTRLB:
		return chan->ctrlb;
	case DMAC_CHAN_CFG:
		return chan->cfg;
	case DMAC_CHAN_SPIP:
		return chan->spip;
	case DMAC_CHAN_DPIP:
		return chan->dpip;
	}
	return 0;
}

static void dmac_chan_write(void *opaque, hwaddr offset, uint64_t value,
	unsigned size)
{
	At91sam9DmacChan * chan = (At91sam9DmacChan*)opaque;

	switch(offset){
	case DMAC_CHAN_SADDR:
		chan->saddr = value;
		break;
	case DMAC_CHAN_DADDR:
		chan->daddr = value;
		break;
	case DMAC_CHAN_DSCR:
		chan->dscr = value;
		break;
	case DMAC_CHAN_CTRLA:
		chan->ctrla = value;
		break;
	case DMAC_CHAN_CTRLB:
		chan->ctrlb = value;
		break;
	case DMAC_CHAN_CFG:
		chan->cfg = value;
		break;
	case DMAC_CHAN_SPIP:
		chan->spip = value;
		break;
	case DMAC_CHAN_DPIP:
		chan->dpip = value;
		break;
	}
}


static uint64_t dmac_read(void *opaque, hwaddr offset, unsigned size)
{
	At91sam9DmacState * ds = AT91SAM9_DMAC(opaque);
	uint32_t value;


	switch(offset){
	case DMAC_GCFG:
		return ds->reg_gcfg;
	case DMAC_EBCIMR:
		return ds->reg_ebcimr;
	case DMAC_EBCISR:
		value = ds->reg_ebcisr;
		ds->reg_ebcisr = 0;
		return value;
	case DMAC_CHSR:
		return ds->reg_chsr;
	}
	return 0;
}

static void dmac_write(void *opaque, hwaddr offset, uint64_t value,
	unsigned size)
{
	At91sam9DmacState * ds = AT91SAM9_DMAC(opaque);
	uint32_t tmp;
	int i;
	switch(offset){
	case DMAC_GCFG:
		ds->reg_gcfg = value;
		break;
	case DMAC_SREQ:
	case DMAC_CREQ:
	case DMAC_LAST:
		break;
	case DMAC_EBCIER:
		ds->reg_ebcimr |= value;
		break;
	case DMAC_EBCIDR:
		ds->reg_ebcimr &= ~value;
		break;
	case DMAC_CHER:
		ds->reg_chsr |= value;
		tmp = __FIELD_VALUE_GET(DMAC_CHSR,ENA,value);
		if(tmp == 0){
			break;
		}
		for(i = 0;i < AT91SAM9_DMAC_CHAN_MAX;i++){
			if(tmp & DMAC_CHSR_ENA(i))
				dmac_chan_start(&ds->chan[i]);
		}
		break;
	case DMAC_CHDR:
		ds->reg_chsr &= ~value;
		break;
	}
}

static void dmac_irqhandler(void *opaque, int n, int level)
{
}

static const MemoryRegionOps dmac_chan_ops = {
	.read = dmac_chan_read,
	.write = dmac_chan_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static const MemoryRegionOps dmac_ops = {
	.read = dmac_read,
	.write = dmac_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static Property dmac_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9DmacState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void dmac_reset(DeviceState *opaque)
{

}

static void dmac_chan_init(At91sam9DmacChan *chan)
{
	char * name;

	name = g_strdup_printf("%s-%d",TYPE_AT91SAM9_DMAC,chan->chan_id);


	memory_region_init_io(&chan->iomem,OBJECT(chan->dmac),&dmac_chan_ops,chan,
		name,DMAC_CHAN_REGION_SIZE);


	g_free(name);

}

static void dmac_init(Object *obj)
{
	At91sam9DmacState * ds = AT91SAM9_DMAC(obj);
	int i = 0;

	sysbus_init_irq(SYS_BUS_DEVICE(ds),&(ds->irq));

	memory_region_init_io(&ds->iomem,OBJECT(ds),&dmac_ops,ds,
		TYPE_AT91SAM9_DMAC,DMAC_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ds),&ds->iomem);

	for(i = 0;i < ARRAY_SIZE(ds->chan);i++){
		ds->chan[i].chan_id = i;
		ds->chan[i].dmac = ds;
		dmac_chan_init(&ds->chan[i]);
	}

	qdev_init_gpio_in(DEVICE(ds), dmac_irqhandler, NR_PERIPH);

}

static void dmac_realize(DeviceState *dev, Error **errp)
{
	At91sam9DmacState * ds = AT91SAM9_DMAC(dev);
	int i;


	sysbus_mmio_map(SYS_BUS_DEVICE(ds), 0, ds->addr);

	for(i = 0;i< ARRAY_SIZE(ds->chan);i++){
		memory_region_add_subregion(&ds->iomem,DMAC_SADDR(i),&ds->chan[i].iomem);
	}

}

static void dmac_class_init(ObjectClass *c ,void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

	dc->realize = dmac_realize;
	dc->reset = dmac_reset;
	dc->props = dmac_properties;
}

static const TypeInfo dmac_type_info = {
	.name = TYPE_AT91SAM9_DMAC,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(At91sam9DmacState),
	.instance_init = dmac_init,
	.class_init = dmac_class_init,
};


static void at91sam9_dmac_register_types(void)
{
	type_register_static(&dmac_type_info);
}

type_init(at91sam9_dmac_register_types)

