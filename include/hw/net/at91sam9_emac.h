/*
 * at91sam9_mac.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#ifndef INCLUDE_HW_NET_AT91SAM9_EMAC_H_
#define INCLUDE_HW_NET_AT91SAM9_EMAC_H_

#include "net/net.h"
#include "hw/irq.h"

#define TYPE_AT91SAM9_EMAC "at91sam9-emac"
#define AT91SAM9_EMAC(obj) \
    OBJECT_CHECK(At91sam9EmacState, (obj), TYPE_AT91SAM9_EMAC)

typedef struct At91sam9EmacState{
    /* <private> */
    SysBusDevice parent_obj;
    /* <public> */
    MemoryRegion iomem;
    uint32_t     addr;
    NICConf      nconf;
    NICState*    nic;
    qemu_irq     irq;
    hwaddr       rbqp_base;
    hwaddr       rbqp_cur;
    hwaddr       tbqp_base;
    hwaddr       tbqp_cur;

    int          rx_enabled;
    int          tx_enabled;

    int          rx_buf_spaces;


    uint32_t     reg_emac_ncr;
    uint32_t     reg_emac_ncfgr;
    uint32_t     reg_emac_imr;
    uint32_t     reg_emac_tsr;
    uint32_t     reg_emac_rsr;
    uint32_t     reg_emac_isr;
    uint32_t     reg_emac_sa1b;
    uint32_t     reg_emac_sa1t;

}At91sam9EmacState;




#endif /* INCLUDE_HW_NET_AT91SAM9_EMAC_H_ */
