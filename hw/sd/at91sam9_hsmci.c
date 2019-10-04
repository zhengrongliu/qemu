/*
 * at91sam9_hsmci.c
 *
 *  Created on: Dec 23, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/sd/sd.h"
#include "hw/sd/at91sam9_hsmci.h"
#include <arpa/inet.h>

#define TYPE_AT91SAM9_HSMCI_SDBUS "at91sam9-hsmci-sdbus"
#define AT91SAM9_HSMCI_SDBUS(obj) \
	OBJECT_CHECK(SDBus,(obj),TYPE_AT91SAM9_HSMCI_SDBUS)

#define HSMCI_REGION_SIZE 0x600


#define HSMCI_CR	0x00
#define HSMCI_MR	0x04
#define HSMCI_DTOR	0x08
#define HSMCI_SDCR	0x0c
#define HSMCI_ARGR	0x10
#define HSMCI_CMDR	0x14
#define HSMCI_BLKR	0x18
#define HSMCI_CSTOR	0x1c
#define HSMCI_RSPR	0x20
#define HSMCI_RSPR2	0x24
#define HSMCI_RSPR3	0x28
#define HSMCI_RSPR4	0x2c
#define HSMCI_RDR	0x30
#define HSMCI_TDR	0x34
#define HSMCI_SR	0x40
#define HSMCI_IER	0x44
#define HSMCI_IDR	0x48
#define HSMCI_IMR	0x4c
#define HSMCI_DMA	0x50
#define HSMCI_CFG	0x54
#define HSMCI_WPMR	0xe4
#define HSMCI_WPSR	0xe8

/*
 * HSMCI Control Register
 */
#define HSMCI_CR_MCIEN		(1<<0)
#define HSMCI_CR_MCIDIS		(1<<1)
#define HSMCI_CR_PWSEN		(1<<2)
#define HSMCI_CR_PWSDIS		(1<<3)
#define HSMCI_CR_SWRST		(1<<7)

/*
 * HSMCI Mode Register
 */
#define HSMCI_MR_RDPROOF	(1<<11)
#define HSMCI_MR_WRPROOF	(1<<12)
#define HSMCI_MR_FBYTE		(1<<13)
#define HSMCI_MR_PADV		(1<<14)
#define HSMCI_MR_CLKODD		(1<<16)

/*
 * HSMCI Data Timeout Register
 */
#define HSMCI_DTOR_DTOCYC_SHIFT		0
#define HSMCI_DTOR_DTOCYC_WIDTH		4
#define HSMCI_DTOR_DTOMUL_SHIFT		4
#define HSMCI_DTOR_DTOMUL_WIDTH		3

/*
 * HSMCI SDCard/SDIO Register
 */
#define HSMIC_SDCR_SDCSEL_SHIFT		0
#define HSMIC_SDCR_SDCSEL_WIDTH		2
#define HSMCI_SDCR_SDCSEL_SLOTA		(0<<HSMIC_SDCR_SDCSEL_SHIFT)
#define HSMCI_SDCR_SDCSEL_SLOTB		(1<<HSMIC_SDCR_SDCSEL_SHIFT)
#define HSMCI_SDCR_SDCSEL_SLOTC		(2<<HSMIC_SDCR_SDCSEL_SHIFT)
#define HSMCI_SDCR_SDCSEL_SLOTD		(3<<HSMIC_SDCR_SDCSEL_SHIFT)
#define HSMIC_SDCR_SDCBUS_SHIFT		6
#define HSMIC_SDCR_SDCBUS_WIDTH		2
#define HSMCI_SDCR_SDCBUS_1BIT		(0<<HSMIC_SDCR_SDCBUS_SHIFT)
#define HSMCI_SDCR_SDCBUS_4BIT		(2<<HSMIC_SDCR_SDCBUS_SHIFT)
#define HSMCI_SDCR_SDCBUS_8BIT		(3<<HSMIC_SDCR_SDCBUS_SHIFT)

/*
 * HSMCI Command Register
 */
#define HSMCI_CMDR_CMDNB_SHIFT		0
#define HSMCI_CMDR_CMDNB_WIDTH		6
#define HSMCI_CMDR_CMDNB_MASK		((1<<HSMCI_CMDR_CMDNB_WIDTH)-1)
#define HSMCI_CMDR_RSPTYP_SHIFT		6
#define HSMCI_CMDR_RSPTYP_WIDTH		2
#define HSMCI_CMDR_RSPTYP_NORESP	0
#define HSMCI_CMDR_RSPTYP_48BIT		1
#define HSMCI_CMDR_RSPTYP_136BIT	2
#define HSMCI_CMDR_RSPTYP_R1B		3
#define HSMCI_CMDR_RSPTYP_MASK		((1<<HSMCI_CMDR_RSPTYP_WIDTH)-1)
#define HSMCI_CMDR_SPCMD_SHIFT		8
#define HSMCI_CMDR_SPCMD_WIDTH		3
#define HSMCI_CMDR_SPCMD_STD		0
#define HSMCI_CMDR_SPCMD_INIT		1
#define HSMCI_CMDR_SPCMD_SYNC		2
#define HSMCI_CMDR_SPCMD_CE_ATA		3
#define HSMCI_CMDR_SPCMD_IT_CMD		4
#define HSMCI_CMDR_SPCMD_IT_RESP	5
#define HSMCI_CMDR_SPCMD_BOR		6
#define HSMCI_CMDR_SPCMD_EBO		7
#define HSMCI_CMDR_SPCMD_MASK		((1<<HSMCI_CMDR_SPCMD_WIDTH)-1)
#define HSMCI_CMDR_OPDCMD_SHIFT		11
#define HSMCI_CMDR_OPDCMD_WIDTH		1
#define HSMCI_CMDR_OPDCMD_PUSHPULL	0
#define HSMCI_CMDR_OPDCMD_OPENDRAIN	1
#define HSMCI_CMDR_OPDCMD_MASK		((1<<HSMCI_CMDR_OPCMD_WIDTH)-1)
#define HSMCI_CMDR_MAXLAT_SHIFT		12
#define HSMCI_CMDR_MAXLAT_WIDTH		1
#define HSMCI_CMDR_MAXLAT_5CYCLE	0
#define HSMCI_CMDR_MAXLAT_64CYCLE	1
#define HSMCI_CMDR_TRCMD_SHIFT		16
#define HSMCI_CMDR_TRCMD_WIDTH		2
#define HSMCI_CMDR_TRCMD_NO_DATA	0
#define HSMCI_CMDR_TRCMD_START_DATA	1
#define HSMCI_CMDR_TRCMD_STOP_DATA	2
#define HSMCI_CMDR_TRDIR_SHIFT		18
#define HSMCI_CMDR_TRDIR_WIDTH		1
#define HSMCI_CMDR_TRDIR_WRITE		0
#define HSMCI_CMDR_TRDIR_READ		1
#define HSMCI_CMDR_TRDIR_MASK		((1<<HSMCI_CMDR_TRDIR_WIDTH)-1)
#define HSMCI_CMDR_TRTYP_SHIFT		19
#define HSMCI_CMDR_TRTYP_WIDTH		3
#define HSMCI_CMDR_TRTYP_SINGLE		0
#define HSMCI_CMDR_TRTYP_MULTIPLE	1
#define HSMCI_CMDR_TRTYP_STREAM		2
#define HSMCI_CMDR_TRTYP_BYTE		4
#define HSMCI_CMDR_TRTYP_BLOCK		5
#define HSMCI_CMDR_IOSPCMD_SHIFT	24
#define HSMCI_CMDR_IOSPCMD_WIDTH	3
#define HSMCI_CMDR_IOSPCMD_STD		0
#define HSMCI_CMDR_IOSPCMD_SUSPEND	1
#define HSMCI_CMDR_IOSPCMD_RESUME	2
#define HSMCI_CMDR_ATACS_SHIFT		26
#define HSMCI_CMDR_ATACS_WIDTH		1
#define HSMCI_CMDR_BOOT_ACK_SHIFT	27
#define HSMCI_CMDR_BOOT_ACK_WIDTH	1

/*
 * HSMCI Block Register
 */
#define HSMCI_BLKR_BCNT_SHIFT		0
#define HSMCI_BLKR_BCNT_WIDTH		16
#define HSMCI_BLKR_BCNT_MASK		((1<<HSMCI_BLKR_BCNT_WIDTH) - 1)
#define HSMCI_BLKR_BLKLEN_SHIFT		16
#define HSMCI_BLKR_BLKLEN_WIDTH		16
#define HSMCI_BLKR_BLKLEN_MASK		((1<<HSMCI_BLKR_BLKLEN_WIDTH) - 1)

/*
 * HSMCI Completion Signal Timeout Register
 */
#define HSMCI_CSTOR_CSTOCYC_SHIFT	0
#define HSMCI_CSTOR_CSTOCYC_WIDTH	4
#define HSMCI_CSTOR_CSTOMUL_SHIFT	4
#define HSMCI_CSTOR_CSTOMUL_WIDTH	3

/*
 * HSMCI Status Register
 */
#define HSMCI_EVENT_CMDRDY		(1<<0)
#define HSMCI_EVENT_RXRDY		(1<<1)
#define HSMCI_EVENT_TXRDY		(1<<2)
#define HSMCI_EVENT_BLKE		(1<<3)
#define HSMCI_EVENT_DTIP		(1<<4)
#define HSMCI_EVENT_NOTBUSY		(1<<5)
#define HSMCI_EVENT_SDIOIRQA		(1<<8)
#define HSMCI_EVENT_SDIOWAIT		(1<<12)
#define HSMCI_EVENT_CSRCV		(1<<13)
#define HSMCI_EVENT_RINDE		(1<<16)
#define HSMCI_EVENT_RDIRE		(1<<17)
#define HSMCI_EVENT_RCRCE		(1<<18)
#define HSMCI_EVENT_RENDE		(1<<19)
#define HSMCI_EVENT_RTOE		(1<<20)
#define HSMCI_EVENT_DCRCE		(1<<21)
#define HSMCI_EVENT_DTOE		(1<<22)
#define HSMCI_EVENT_CSTOE		(1<<23)
#define HSMCI_EVENT_BLKOVRE		(1<<24)
#define HSMCI_EVENT_DMADONE		(1<<25)
#define HSMCI_EVENT_FIFOEMPTY		(1<<26)
#define HSMCI_EVENT_XFRDONE		(1<<27)
#define HSMCI_EVENT_ACKRCV		(1<<28)
#define HSMCI_EVENT_ACKRCVE		(1<<29)
#define HSMCI_EVENT_OVRE		(1<<30)
#define HSMCI_EVENT_UNRE		(1<<31)


/*
 * HSMCI DMA Configuration Register
 */
#define HSMCI_DMA_OFFSET_SHIFT		0
#define HSMCI_DMA_OFFSET_WIDTH		2
#define HSMCI_DMA_CHKSIZE_SHIFT		4
#define HSMCI_DMA_CHKSIZE_WIDTH		2
#define HSMCI_DMA_DMAEN_SHIFT		8
#define HSMCI_DMA_DMAEN_WIDTH		1
#define HSMCI_DMA_ROPT_SHIFT		12
#define HSMCI_DMA_ROPT_WIDTH		1


/*
 * HSMCI Configuration Register
 */
#define HSMCI_CFG_FIFOMODE		(1<<0)
#define HSMCI_CFG_FERRCTRL		(1<<1)
#define HSMCI_CFG_HSMODE		(1<<8)
#define HSMCI_CFG_LSYNC			(1<<12)

/*
 * HSMCI Write Protection Mode Register
 */
#define HSMCI_WPMR_WPEN_SHIFT		0
#define HSMCI_WPMR_WPEN_WIDTH		1
#define HSMCI_WPMR_WPKEY_SHIFT		8
#define HSMCI_WPMR_WPKEY_WIDTH		24

/*
 * HSMCI Write Protection Status Register
 */
#define HSMCI_WPSR_WPVS_SHIFT		0
#define HSMCI_WPSR_WPVS_WIDTH		1
#define HSMCI_WPSR_WPVSRC_SHIFT		8
#define HSMCI_WPSR_WPVSRC_WIDTH		16


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


static void hsmci_update_irq(At91sam9HsmciState *hs)
{
	uint32_t irq;

	irq = hs->reg_sr & hs->reg_imr;

	qemu_set_irq(hs->irq,!!irq);

}
#if 0
static void hsmci_set_status(At91sam9HsmciState *hs ,uint32_t st)
{
	hs->reg_sr |= st;

	hsmci_update_irq(hs);
}
#endif

static int hsmci_sdbus_send_command(At91sam9HsmciState *hs,SDRequest *req)
{
	int len;
	uint32_t rsptype;

	memset(hs->reg_rspr,0,sizeof(hs->reg_rspr));

	len = sdbus_do_command(&hs->sdbus, req, (uint8_t*)hs->reg_rspr);

	if(len < 0){

	}

	rsptype = __FIELD_VALUE_GET(HSMCI_CMDR,RSPTYP,hs->reg_cmdr);

	if(rsptype == HSMCI_CMDR_RSPTYP_NORESP){
		hs->reg_sr |= HSMCI_EVENT_CMDRDY;
		return 0;
	}


	if(len == 4){
		hs->reg_rspr[0] = ntohl(hs->reg_rspr[0]);
	} else if(len == 16){
		hs->reg_rspr[0] = ntohl(hs->reg_rspr[0]);
		hs->reg_rspr[1] = ntohl(hs->reg_rspr[1]);
		hs->reg_rspr[2] = ntohl(hs->reg_rspr[2]);
		hs->reg_rspr[3] = ntohl(hs->reg_rspr[3]);
	}

	hs->reg_sr |= HSMCI_EVENT_CMDRDY;

	return 0;
}

static void hsmci_sdbus_send_data(At91sam9HsmciState *hs,uint32_t value)
{
	int len = 4;
	int i;
	uint8_t * data = (uint8_t*)&value;

	if(hs->xfer_size == 0){
		return ;
	}


	if(hs->xfer_size < len){
		len = hs->xfer_size;
	}

	i = 0;
	while(i < len){
		sdbus_write_data(&hs->sdbus,data[i]);
		hs->xfer_size--;
		i++;
	}

	if(!hs->xfer_size){
		hs->reg_sr &= ~HSMCI_EVENT_TXRDY;
	}
}

static uint32_t hsmci_sdbus_receive_data(At91sam9HsmciState *hs)
{
	int len = 4;
	int i;
	uint32_t value;
	uint8_t data;

	if(hs->xfer_size == 0){
		return 0;
	}

	if(hs->xfer_size < len){
		len = hs->xfer_size;
	}

	i = 0;
	value = 0;
	while(i < len){
		data = sdbus_read_data(&hs->sdbus);
		value |= data << i*8;
		hs->xfer_size--;
		i++;
	}


	if(!hs->xfer_size){
		hs->reg_sr &= ~HSMCI_EVENT_RXRDY;

	}

	return value;

}


static uint64_t hsmci_read(void *opaque, hwaddr offset, unsigned size)
{
	At91sam9HsmciState * hs = AT91SAM9_HSMCI(opaque);

	switch(offset){
	case HSMCI_RSPR:
		return hs->reg_rspr[0];
	case HSMCI_RSPR + 0x04:
		return hs->reg_rspr[1];
	case HSMCI_RSPR + 0x08:
		return hs->reg_rspr[2];
	case HSMCI_RSPR + 0x0c:
		return hs->reg_rspr[3];
	case HSMCI_IMR:
		return hs->reg_imr;
	case HSMCI_SR:
		return hs->reg_sr;
	case HSMCI_RDR:
		hs->reg_rdr = hsmci_sdbus_receive_data(hs);
		hsmci_update_irq(hs);
		return hs->reg_rdr;

	}
	return 0;
}

static void hsmci_write(void *opaque, hwaddr offset, uint64_t value,
	unsigned size)
{

	At91sam9HsmciState * hs = AT91SAM9_HSMCI(opaque);

	switch(offset){
	case HSMCI_CMDR:{
		SDRequest request;
		hs->reg_cmdr = value;
		request.cmd = __FIELD_VALUE_GET(HSMCI_CMDR,CMDNB,hs->reg_cmdr);
		request.arg = hs->reg_argr;
		hsmci_sdbus_send_command(hs,&request);

		if(value & __FIELD_VALUE_FIXED(HSMCI_CMDR,TRCMD,START_DATA)){
			hs->xfer_size = __FIELD_VALUE_GET(HSMCI_BLKR,BCNT,hs->reg_blkr) *
					__FIELD_VALUE_GET(HSMCI_BLKR,BLKLEN,hs->reg_blkr);
			if(__FIELD_VALUE_GET(HSMCI_CMDR,TRDIR,value) == HSMCI_CMDR_TRDIR_READ){
				hs->reg_sr |= HSMCI_EVENT_RXRDY;
			}
			if(__FIELD_VALUE_GET(HSMCI_CMDR,TRDIR,value) == HSMCI_CMDR_TRDIR_WRITE){
				hs->reg_sr |= HSMCI_EVENT_TXRDY;
			}
		}
		hsmci_update_irq(hs);
		break;
	}
	case HSMCI_ARGR:
		hs->reg_argr = value;
		break;
	case HSMCI_IER:
		hs->reg_imr |= value;
		hsmci_update_irq(hs);
		break;
	case HSMCI_IDR:
		hs->reg_imr &= ~value;
		break;
	case HSMCI_TDR:
		hsmci_sdbus_send_data(hs,value);
		hsmci_update_irq(hs);
		break;
	case HSMCI_BLKR:
		hs->reg_blkr = value;
		break;
	}

}

static const MemoryRegionOps hsmci_ops = {
	.read = hsmci_read,
	.write = hsmci_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static Property hsmci_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9HsmciState,addr,0),
	DEFINE_PROP_END_OF_LIST(),
};

static void hsmci_reset(DeviceState *opaque)
{

}

static void hsmci_init(Object *obj) 
{
	At91sam9HsmciState * hs = AT91SAM9_HSMCI(obj);

	qbus_create_inplace(&hs->sdbus,sizeof(hs->sdbus),
		TYPE_AT91SAM9_HSMCI_SDBUS,DEVICE(hs),"sd-bus");

	sysbus_init_irq(SYS_BUS_DEVICE(hs),&(hs->irq));

	memory_region_init_io(&hs->iomem,OBJECT(hs),&hsmci_ops,hs,
		TYPE_AT91SAM9_HSMCI,HSMCI_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(hs),&hs->iomem);


}

static void hsmci_realize(DeviceState *dev, Error **errp)
{
	At91sam9HsmciState * hs = AT91SAM9_HSMCI(dev);


	sysbus_mmio_map(SYS_BUS_DEVICE(hs), 0, hs->addr);

}

static void hsmci_class_init(ObjectClass *c ,void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);
	
	dc->realize = hsmci_realize;
	dc->reset = hsmci_reset;
	dc->props = hsmci_properties;
}

static const TypeInfo hsmci_type_info = {
	.name = TYPE_AT91SAM9_HSMCI,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(At91sam9HsmciState),
	.instance_init = hsmci_init,
	.class_init = hsmci_class_init,
};

static const TypeInfo hsmci_sdbus_info = {
	.name = TYPE_AT91SAM9_HSMCI_SDBUS,
	.parent = TYPE_SD_BUS,
	.instance_size = sizeof(SDBus),
};

static void at91sam9_hsmci_register_types(void)
{
	type_register_static(&hsmci_type_info);
	type_register_static(&hsmci_sdbus_info);
}

type_init(at91sam9_hsmci_register_types)
