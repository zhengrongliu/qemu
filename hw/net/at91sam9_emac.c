/*
 * at91sam9_emac.c
 *
 *  Created on: Apr 20, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "net/net.h"
#include "hw/sysbus.h"
#include "hw/net/at91sam9_emac.h"

#define EMAC_REGION_SIZE 0x100

#define EMAC_NCR 0x0
#define EMAC_NCFGR 0x4
#define EMAC_NSR 0x8
#define EMAC_TSR 0x14
#define EMAC_RBQP 0x18
#define EMAC_TBQP 0x1c
#define EMAC_RSR 0x20
#define EMAC_ISR 0x24
#define EMAC_IER 0x28
#define EMAC_IDR 0x2c
#define EMAC_IMR 0x30
#define EMAC_MAN 0x34
#define EMAC_PTR 0x38
#define EMAC_PFR 0x3c
#define EMAC_FTO 0x40
#define EMAC_SCF 0x44
#define EMAC_MCF 0x48
#define EMAC_FRO 0x4c
#define EMAC_FCSE 0x50
#define EMAC_ALE 0x54
#define EMAC_DTF 0x58
#define EMAC_LCOL 0x5c
#define EMAC_ECOL 0x60
#define EMAC_TUND 0x64
#define EMAC_CSE 0x68
#define EMAC_RRE 0x6c
#define EMAC_ROV 0x70
#define EMAC_RSE 0x74
#define EMAC_ELE 0x78
#define EMAC_RJA 0x7c
#define EMAC_USF 0x80
#define EMAC_STE 0x84
#define EMAC_RLE 0x88
#define EMAC_HRB 0x90
#define EMAC_HRT 0x94
#define EMAC_SA1B 0x98
#define EMAC_SA1T 0x9c
#define EMAC_SA2B 0xa0
#define EMAC_SA2T 0xa4
#define EMAC_SA3B 0xa8
#define EMAC_SA3T 0xac
#define EMAC_SA4B 0xb0
#define EMAC_SA4T 0xb4
#define EMAC_TID 0xb8
#define EMAC_USRIO 0xc0

/*
 * EMAC_NCR Network Control Register
 */
#define EMAC_NCR_LB (1 << 0)
#define EMAC_NCR_LLB (1 << 1)
#define EMAC_NCR_RE (1 << 2)
#define EMAC_NCR_TE (1 << 3)
#define EMAC_NCR_MPE (1 << 4)
#define EMAC_NCR_CLRSTAT (1 << 5)
#define EMAC_NCR_INCSTAT (1 << 6)
#define EMAC_NCR_WESTAT (1 << 7)
#define EMAC_NCR_BP (1 << 8)
#define EMAC_NCR_TSTART (1 << 9)
#define EMAC_NCR_THALT (1 << 10)

/*
 * EMAC_NCFGR Network Configuration Register
 */
#define EMAC_NCFGR_SPD_SHIFT 0
#define EMAC_NCFGR_FD_SHIFT 1
#define EMAC_NCFGR_JFRAME_SHIFT 2
#define EMAC_NCFGR_CAF_SHIFT 3
#define EMAC_NCFGR_NBC_SHIFT 4
#define EMAC_NCFGR_MTI_SHIFT 5
#define EMAC_NCFGR_UNI_SHIFT 6
#define EMAC_NCFGR_BIG_SHIFT 7
#define EMAC_NCFGR_CLK_SHIFT 10
#define EMAC_NCFGR_CLK_MASK 0x3
#define EMAC_NCFGR_RTY_SHIFT 12
#define EMAC_NCFGR_PAE_SHIFT 13
#define EMAC_NCFGR_RBOF_SHIFT 14
#define EMAC_NCFGR_RBOF_MASK 0x3
#define EMAC_NCFGR_RLCE_SHIFT 16
#define EMAC_NCFGR_DRFCS_SHIFT 17
#define EMAC_NCFGR_EFRHD_SHIFT 18
#define EMAC_NCFGR_IRXFCS_SHIFT 19

#define EMAC_NCFGR_SPD_10M (0 << EMAC_NCFGR_SPD_SHIFT)
#define EMAC_NCFGR_SPD_100M (1 << EMAC_NCFGR_SPD_SHIFT)
#define EMAC_NCFGR_FD_FULL (1 << EMAC_NCFGR_FD_SHIFT)
#define EMAC_NCFGR_FD_HALT (0 << EMAC_NCFGR_FD_SHIFT)
#define EMAC_NCFGR_JFRAME_ENABLE (1 << EMAC_NCFGR_JFRAME_SHIFT)
#define EMAC_NCFGR_JFRAME_DISABLE (0 << EMAC_NCFGR_JFRAME_SHIFT)
#define EMAC_NCFGR_CAF_ENABLE (1 << EMAC_NCFGR_CAF_SHIFT)
#define EMAC_NCFGR_CAF_DISABLE (0 << EMAC_NCFGR_CAF_SHIFT)
#define EMAC_NCFGR_NBC_ENABLE (1 << EMAC_NCFGR_NBC_SHIFT)
#define EMAC_NCFGR_NBC_DISABLE (0 << EMAC_NCFGR_NBC_SHIFT)
#define EMAC_NCFGR_MTI_ENABLE (1 << EMAC_NCFGR_MTI_SHIFT)
#define EMAC_NCFGR_MTI_DISABLE (0 << EMAC_NCFGR_MTI_SHIFT)
#define EMAC_NCFGR_UNI_ENABLE (1 << EMAC_NCFGR_UNI_SHIFT)
#define EMAC_NCFGR_UNI_DISABLE (0 << EMAC_NCFGR_UNI_SHIFT)
#define EMAC_NCFGR_BIG_ENABLE (1 << EMAC_NCFGR_BIG_SHIFT)
#define EMAC_NCFGR_BIG_DISABLE (0 << EMAC_NCFGR_BIG_SHIFT)
#define EMAC_NCFGR_CLK(x) ((x) << EMAC_NCFGR_CLK_SHIFT)
#define EMAC_NCFGR_RTY_ENABLE (1 << EMAC_NCFGR_RTY_SHIFT)
#define EMAC_NCFGR_RTY_DISABLE (0 << EMAC_NCFGR_RTY_SHIFT)
#define EMAC_NCFGR_PAE_ENABLE (1 << EMAC_NCFGR_PAE_SHIFT)
#define EMAC_NCFGR_PAE_DISABLE (0 << EMAC_NCFGR_PAE_SHIFT)
#define EMAC_NCFGR_RBOF(x) ((x) << EMAC_NCFGR_RBOF_SHIFT)
#define EMAC_NCFGR_RLCE_ENABLE (1 << EMAC_NCFGR_RLCE_SHIFT)
#define EMAC_NCFGR_RLCE_DISABLE (0 << EMAC_NCFGR_RLCE_SHIFT)
#define EMAC_NCFGR_DRFCS_ENABLE (1 << EMAC_NCFGR_DRFCS_SHIFT)
#define EMAC_NCFGR_DRFCS_DISABLE (0 << EMAC_NCFGR_DRFCS_SHIFT)
#define EMAC_NCFGR_EFRHD_ENABLE (1 << EMAC_NCFGR_EFRHD_SHIFT)
#define EMAC_NCFGR_EFRHD_DISABLE (0 << EMAC_NCFGR_EFRHD_SHIFT)
#define EMAC_NCFGR_IRXFCS_ENABLE (1 << EMAC_NCFGR_IRXFCS_SHIFT)
#define EMAC_NCFGR_IRXFCS_DISABLE (0 << EMAC_NCFGR_IRXFCS_SHIFT)

/*
 * EMAC_NSR Network Status Register
 */
#define EMAC_NSR_MDIO (1 << 1)
#define EMAC_NSR_IDLE (1 << 2)

/*
 * EMAC_TSR Transmit Status Register
 */
#define EMAC_TSR_UBR (1 << 0)
#define EMAC_TSR_COL (1 << 1)
#define EMAC_TSR_RLES (1 << 2)
#define EMAC_TSR_TGO (1 << 3)
#define EMAC_TSR_BEX (1 << 4)
#define EMAC_TSR_COMP (1 << 5)
#define EMAC_TSR_UND (1 << 6)

/*
 * EMAC_RSR Receive Status Register
 */
#define EMAC_RSR_BNA (1 << 0)
#define EMAC_RSR_REC (1 << 1)
#define EMAC_RSR_OVR (1 << 2)

/*
 * EMAC Interrupt event
 */
#define EMAC_INT_EV_MFD (1 << 0)
#define EMAC_INT_EV_RCOMP (1 << 1)
#define EMAC_INT_EV_RXUBR (1 << 2)
#define EMAC_INT_EV_TXUBR (1 << 3)
#define EMAC_INT_EV_TUND (1 << 4)
#define EMAC_INT_EV_RLEX (1 << 5)
#define EMAC_INT_EV_TXERR (1 << 6)
#define EMAC_INT_EV_TCOMP (1 << 7)
#define EMAC_INT_EV_ROVR (1 << 10)
#define EMAC_INT_EV_HRESP (1 << 11)
#define EMAC_INT_EV_PFRE (1 << 12)
#define EMAC_INT_EV_PTZ (1 << 13)

/*
 * EMAC_MAN PHY Maintenance Register
 */
#define EMAC_MAN_DATA_SHIFT 0
#define EMAC_MAN_DATA_MASK 0xffff
#define EMAC_MAN_CODE_SHIFT 16
#define EMAC_MAN_CODE_MASK 0x3
#define EMAC_MAN_REGA_SHIFT 18
#define EMAC_MAN_REGA_MASK 0x1f
#define EMAC_MAN_PHYA_SHIFT 23
#define EMAC_MAN_PHYA_MASK 0x1f
#define EMAC_MAN_RW_SHIFT 28
#define EMAC_MAN_RW_MASK 0x3
#define EMAC_MAN_SOF_SHIFT 30
#define EMAC_MAN_SOF_MASK 0x3

/*
 * EMAC_PTR Pause Time Register
 */
#define EMAC_PTR_PTIME_SHIFT 0
#define EMAC_PTR_PTIME_MASK 0xffff

/*
 * EMAC_TID Type ID Checking Register
 */
#define EMAC_TID_SHIFT 0
#define EMAC_TID_MASK 0xffff

/*
 * EMAC_USRIO User Input/Output Register
 */
#define EMAC_USRIO_RMII (1 << 0)
#define EMAC_USRIO_CLKEN (1 << 1)

/*
 * transmit buffer descriptor
 */
#define EMAC_TX_DESC_LENGTH_MASK 0x7ff
#define EMAC_TX_DESC_LENGTH_SHIFT 0
#define EMAC_TX_DESC_LENGTH(x)                                                 \
	((EMAC_TX_DESC_LENGTH_MASK & (x)) << EMAC_TX_DESC_LENGTH_SHIFT)
#define EMAC_TX_DESC_LAST_BUFFER (1 << 15)
#define EMAC_TX_DESC_NO_CRC (1 << 16)
#define EMAC_TX_DESC_BUF_EXHST (1 << 27)
#define EMAC_TX_DESC_UNDERRUN (1 << 28)
#define EMAC_TX_DESC_ERROR (1 << 29)
#define EMAC_TX_DESC_WRAP (1 << 30)
#define EMAC_TX_DESC_USED (1 << 31)

/*
 * receive buffer descriptor
 */
#define EMAC_RX_DESC_LENGTH_MASK 0xfff
#define EMAC_RX_DESC_LENGTH_SHIFT 0
#define EMAC_RX_DESC_LENGTH(x)                                                 \
	((EMAC_RX_DESC_LENGTH_MASK & (x)) << EMAC_RX_DESC_LENGTH_SHIFT)
#define EMAC_RX_DESC_RX_OFFSET(x) ((0x3 & (x)) << 12)
#define EMAC_RX_DESC_SOF (1 << 14)
#define EMAC_RX_DESC_EOF (1 << 15)
#define EMAC_RX_DESC_CFI (1 << 16)
#define EMAC_RX_DESC_VLAN_PRIOR(x) ((0x7 & (x)) << 17)
#define EMAC_RX_DESC_PRIOR_DETECT (1 << 20)
#define EMAC_RX_DESC_VLAN_DETECT (1 << 21)
#define EMAC_RX_DESC_TYPE_ID_MATCH (1 << 22)
#define EMAC_RX_DESC_SA4_MATCH (1 << 23)
#define EMAC_RX_DESC_SA3_MATCH (1 << 24)
#define EMAC_RX_DESC_SA2_MATCH (1 << 25)
#define EMAC_RX_DESC_SA1_MATCH (1 << 26)
#define EMAC_RX_DESC_EXT_ADDR_MATCH (1 << 28)
#define EMAC_RX_DESC_UNI_HASH_MATCH (1 << 29)
#define EMAC_RX_DESC_MUL_HASH_MATCH (1 << 30)
#define EMAC_RX_DESC_BROADCAST_MATCH (1 << 31)

#define EMAC_RX_DESC_OWNER_SOFT (1 << 0)
#define EMAC_RX_DESC_OWNER_HARD (0 << 0)
#define EMAC_RX_DESC_OWNER_MASK (1 << 0)
#define EMAC_RX_DESC_WRAP (1 << 1)
#define EMAC_RX_DESC_ADDR_MASK 0xfffffffc

#define EMAC_RX_BUF_SIZE 128

struct emac_buffer_descriptor {
	uint32_t addr;
	uint32_t ctrl;
};

#define EMAC_BUF_SIZE 2048
static uint8_t buffer[EMAC_BUF_SIZE];

static int at91sam9_emac_can_receive(NetClientState *nc);

static void at91sam9_emac_start_tx_xfer(At91sam9EmacState *es)
{
	NetClientState *nc = qemu_get_queue(es->nic);
	struct emac_buffer_descriptor desc;
	uint8_t *buf = buffer;
	int len = 0;
	int total_len = 0;

	if (!es->tx_enabled) {
		return;
	}

	//fprintf(stderr,"start send\n");
	cpu_physical_memory_read(es->tbqp_cur, &desc, sizeof(desc));

	while (!(desc.ctrl & EMAC_TX_DESC_USED)) {
		len = (desc.ctrl >> EMAC_TX_DESC_LENGTH_SHIFT) &
		      EMAC_TX_DESC_LENGTH_MASK;

		cpu_physical_memory_read(desc.addr, buf + total_len, len);

		total_len += len;


		if (desc.ctrl & EMAC_TX_DESC_LAST_BUFFER) {
			qemu_send_packet(nc, buf, total_len);
			total_len = 0;
		}
		desc.ctrl |= EMAC_TX_DESC_USED;

		cpu_physical_memory_write(es->tbqp_cur, &desc, sizeof(desc));

		if ((desc.ctrl & EMAC_TX_DESC_WRAP) ||
				(es->tbqp_cur - es->tbqp_base)/sizeof(desc) == 1024) {
			es->tbqp_cur = es->tbqp_base;
		} else {
			es->tbqp_cur += sizeof(desc);
		}

		cpu_physical_memory_read(es->tbqp_cur, &desc, sizeof(desc));
	}


	es->reg_emac_tsr |= EMAC_TSR_COMP;
	es->reg_emac_isr |= EMAC_INT_EV_TCOMP;

	qemu_irq_raise(es->irq);
}

static void at91sam9_EMAC_NCR_write(At91sam9EmacState *es, uint32_t value)
{

	NetClientState *nc;


	if (value & EMAC_NCR_TSTART) {
		at91sam9_emac_start_tx_xfer(es);
	}

	if (value & EMAC_NCR_TE) {
		es->tx_enabled = true;
	} else {
		es->tx_enabled = false;
	}

	if (value & EMAC_NCR_RE) {
		es->rx_enabled = true;
		nc = qemu_get_queue(es->nic);
		if (at91sam9_emac_can_receive(nc)) {
			qemu_flush_queued_packets(nc);
		}
	} else {
		es->rx_enabled = false;
	}
}

static void at91sam9_EMAC_NCFGR_write(At91sam9EmacState *es, uint32_t value) {}

static uint64_t at91sam9_emac_read(void *opaque, hwaddr addr, unsigned size)
{
	At91sam9EmacState *es = AT91SAM9_EMAC(opaque);
	uint64_t data = 0;

	switch (addr) {
	case EMAC_NCR:
		data = es->reg_emac_ncr;
		break;
	case EMAC_NCFGR:
		data = es->reg_emac_ncfgr;
		break;
	case EMAC_SA1B:
		data = *((uint32_t *)&(es->nconf.macaddr.a[0]));
		break;
	case EMAC_SA1T:
		data = *((uint16_t *)&(es->nconf.macaddr.a[4]));
		break;
	case EMAC_RBQP:
		data = es->rbqp_cur;
		break;
	case EMAC_TBQP:
		data = es->tbqp_cur;
		break;
	case EMAC_IMR:
		data = es->reg_emac_imr;
		break;
	case EMAC_ISR:
		data = es->reg_emac_isr;
		es->reg_emac_isr = 0;
		break;
	case EMAC_TSR:
		data = es->reg_emac_tsr;
		break;
	case EMAC_RSR:
		data = es->reg_emac_rsr;
		break;
	}

	return data;
}

static void at91sam9_emac_write(void *opaque, hwaddr addr, uint64_t data,
				unsigned size)
{
	At91sam9EmacState *es = AT91SAM9_EMAC(opaque);

	switch (addr) {
	case EMAC_NCR:
		at91sam9_EMAC_NCR_write(es, data);
		es->reg_emac_ncr = data;
		break;
	case EMAC_NCFGR:
		at91sam9_EMAC_NCFGR_write(es, data);
		es->reg_emac_ncfgr = data;
		break;
	case EMAC_SA1B:
		*((uint32_t *)&(es->nconf.macaddr.a[0])) = data;
		break;
	case EMAC_SA1T:
		*((uint16_t *)&(es->nconf.macaddr.a[4])) = data;
		break;
	case EMAC_RBQP:
		es->rbqp_base = data;
		es->rbqp_cur = data;
		break;
	case EMAC_TBQP:
		es->tbqp_base = data;
		es->tbqp_cur = data;
		break;
	case EMAC_IDR:
		es->reg_emac_imr &= ~data;
		break;
	case EMAC_IER:
		es->reg_emac_imr |= data;
		break;
	case EMAC_TSR:
		es->reg_emac_tsr &= ~data | EMAC_TSR_TGO;
		break;
	}
}

static int at91sam9_emac_can_receive(NetClientState *nc)
{
	int total_len = 0;
	At91sam9EmacState *es = qemu_get_nic_opaque(nc);
	struct emac_buffer_descriptor desc;
	hwaddr rbqp = es->rbqp_cur;

	if (!es->rx_enabled) {
		return 0;
	}

	cpu_physical_memory_read(rbqp, &desc, sizeof(desc));

	while ((desc.addr & EMAC_RX_DESC_OWNER_MASK) ==
	       EMAC_RX_DESC_OWNER_HARD) {
		total_len += EMAC_RX_BUF_SIZE;

		if ((desc.addr & EMAC_RX_DESC_WRAP) ||
		    (rbqp - es->rbqp_base) / sizeof(desc) == 1024) {
			rbqp = es->rbqp_base;
		} else {
			rbqp += sizeof(desc);
		}

		if (rbqp == es->rbqp_cur) {
			break;
		}

		cpu_physical_memory_read(rbqp, &desc, sizeof(desc));
	}

	return total_len;
}

static ssize_t at91sam9_emac_receive(NetClientState *nc, const uint8_t *data,
				     size_t size)
{
	At91sam9EmacState *es = qemu_get_nic_opaque(nc);
	struct emac_buffer_descriptor desc;
	int len = EMAC_RX_BUF_SIZE;
	int total_len = size;
	uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	if (!es->rx_enabled) {
		return 0;
	}

	if (memcmp(es->nconf.macaddr.a, data, 6) != 0 &&
	    memcmp(data, broadcast, 6) != 0) {
		return size;
	}

	//fprintf(stderr,"emac recv %ld\n",size);

	cpu_physical_memory_read(es->rbqp_cur, &desc, sizeof(desc));
	desc.ctrl = EMAC_RX_DESC_SOF;
	while (total_len > 0) {
		if (total_len < len) {
			len = total_len;
		}

		cpu_physical_memory_write(desc.addr & EMAC_RX_DESC_ADDR_MASK,
					  data, len);

		data += len;
		total_len -= len;

		desc.addr |= EMAC_RX_DESC_OWNER_SOFT;
		if (total_len == 0) {
			desc.ctrl |=
			    EMAC_RX_DESC_LENGTH(size) | EMAC_RX_DESC_EOF;
		}
		cpu_physical_memory_write(es->rbqp_cur, &desc, sizeof(desc));

		if ((desc.addr & EMAC_RX_DESC_WRAP) ||
		    (es->rbqp_cur - es->rbqp_base) / sizeof(desc) == 1024) {
			es->rbqp_cur = es->rbqp_base;
		} else {
			es->rbqp_cur += sizeof(desc);
		}

		cpu_physical_memory_read(es->rbqp_cur, &desc, sizeof(desc));
		desc.ctrl = 0;
	}

	es->reg_emac_rsr |= EMAC_RSR_REC;
	es->reg_emac_isr |= EMAC_INT_EV_RCOMP;
	qemu_irq_raise(es->irq);

	return size;
}

static void at91sam9_emac_set_link(NetClientState *nc) {}

static const MemoryRegionOps at91sam9_emac_ops = {
	.read = at91sam9_emac_read,
	.write = at91sam9_emac_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

NetClientInfo net_at91sam9_emac_info = {
	.type = NET_CLIENT_DRIVER_NIC,
	.size = sizeof(NICState),
	.can_receive = at91sam9_emac_can_receive,
	.receive = at91sam9_emac_receive,
	.link_status_changed = at91sam9_emac_set_link,
};

static void at91sam9_emac_init(Object *obj)
{
	At91sam9EmacState *es = AT91SAM9_EMAC(obj);

	memory_region_init_io(&es->iomem, OBJECT(es), &at91sam9_emac_ops, es,
			      TYPE_AT91SAM9_EMAC, EMAC_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(es), &es->iomem);

	sysbus_init_irq(SYS_BUS_DEVICE(es), &(es->irq));

	es->rx_enabled = false;
	es->tx_enabled = false;
}

static void at91sam9_emac_realize(DeviceState *dev, Error **errp)
{
	At91sam9EmacState *es = AT91SAM9_EMAC(dev);
	// NetClientState *ncs;

	sysbus_mmio_map(SYS_BUS_DEVICE(es), 0, es->addr);

#if 1
	es->nic = qemu_new_nic(&net_at91sam9_emac_info, &es->nconf,
			       object_get_typename(OBJECT(dev)), dev->id, es);
	qemu_format_nic_info_str(qemu_get_queue(es->nic), es->nconf.macaddr.a);
#else

	ncs = qemu_find_netdev("eth0");

	es->nconf.peers.ncs[0] = ncs;
	es->nconf.peers.queues = 1;

	es->nic = qemu_new_nic(&net_at91sam9_emac_info, &es->nconf,
			       object_get_typename(OBJECT(dev)), dev->id, es);
#endif
}

static void at91sam9_emac_reset(DeviceState *dev) {}

static Property at91sam9_emac_properties[] = {
	DEFINE_PROP_UINT32("addr",At91sam9EmacState,addr,0),
	DEFINE_NIC_PROPERTIES(At91sam9EmacState, nconf), DEFINE_PROP_END_OF_LIST(),
};

static void at91sam9_emac_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = at91sam9_emac_realize;
	dc->props = at91sam9_emac_properties;
	dc->reset = at91sam9_emac_reset;
}

static const TypeInfo at91sam9_emac_info = {
	.name = TYPE_AT91SAM9_EMAC,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_init = at91sam9_emac_init,
	.instance_size = sizeof(At91sam9EmacState),
	.class_init = at91sam9_emac_class_init,
};

static void at91sam9_emac_register_types(void)
{
	type_register_static(&at91sam9_emac_info);
}

type_init(at91sam9_emac_register_types)
