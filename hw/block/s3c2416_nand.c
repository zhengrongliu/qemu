/*
 * s3c2416_nand.c
 *
 *  Created on: Feb 5, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "hw/misc/s3c2416_nand.h"

#define NAND_REGION_SIZE 0x100

#define REG_IND(x) ((x) / 4)

#define NFCONF 0x00
#define NFCONT 0x04
#define NFCMMD 0x08
#define NFADDR 0x0c
#define NFDATA 0x10
#define NFMECCD0 0x14
#define NFMECCD1 0x18
#define NFSECCD 0x1c
#define NFSBLK 0x20
#define NFEBLK 0x24
#define NFSTAT 0x28
#define NFECCERR0 0x2c
#define NFECCERR1 0x30
#define NFMECC0 0x34
#define NFMECC1 0x38
#define NFSECC 0x3c
#define NFMLCBITPT 0x40
#define NF8ECCERR0 0x44
#define NF8ECCERR1 0x48
#define NF8ECCERR2 0x4c
#define NFM8ECC0 0x50
#define NFM8ECC1 0x54
#define NFM8ECC2 0x58
#define NFM8ECC3 0x5c
#define NFMLC8BITPT0 0x60
#define NFMLC8BITPT1 0x64

/*
 * NFCONF
 */
#define NCONF_BUS_WIDTH_SHIFT 0x00
#define NCONF_ADDR_CYCLE_SHIFT 0x01
#define NCONF_PAGE_SIZE_EXT_SHIFT 0x02
#define NCONF_PAGE_SIZE_SHIFT 0x03
#define NCONF_TWRPH1_SHIFT 0x04
#define NCONF_TWRPH1_MSK 0x07
#define NCONF_TWRPH0_SHIFT 0x08
#define NCONF_TWRPH0_MSK 0x07
#define NCONF_TACLS_SHIFT 0x0c
#define NCONF_TACLS_MSK 0x07
#define NCONF_ECC_TYPE_SHIFT 0x17
#define NCONF_ECC_TYPE_MSK 0x03
#define NCONF_MSG_LENGTH_SHIFT 0x19

/*
 * NCONT
 */
#define NCONT_MODE_SHIFT 0x00
#define NCONT_REG_nCE0_SHIFT 0x01
#define NCONT_REG_nCE1_SHIFT 0x02
#define NCONT_INIT_SECC_SHIFT 0x04
#define NCONT_INIT_MECC_SHIFT 0x05
#define NCONT_SPARE_ECCLOCK_SHIFT 0x06
#define NCONT_MAIN_ECCLOCK_SHIFT 0x07
#define NCONT_RnB_TRANSMODE_SHIFT 0x08
#define NCONT_RnB_INT_SHIFT 0x09
#define NCONT_ILLEGAL_ACCL_INT_SHIFT 0x0a
#define NCONT_8BIT_STOP_SHIFT 0x0b
#define NCONT_ECC_DECODE_INT_SHIFT 0x0c
#define NCONT_SOFT_LOCK_SHIFT 0x10
#define NCONT_LOCK_TIGHT_SHIFT 0x11
#define NCONT_ECC_DIRECT_SHIFT 0x12

/*
 * Nand flash status register
 */
#define NFSTAT_READY_SHIFT 0x0

static uint64_t s3c2416_nand_read(void *opaque, hwaddr addr, unsigned size)
{
	S3C2416NandState *ns = S3C2416_NAND(opaque);
	uint8_t data;

	switch (addr) {
	case NFDATA:
		nand_flash_get_data(ns->flash, &data);
		return data;
	case NFSTAT:
		return ns->regs[REG_IND(addr)] | (1 << NFSTAT_READY_SHIFT);
	}

	return ns->regs[REG_IND(addr)];
}

static void s3c2416_nand_write(void *opaque, hwaddr addr, uint64_t data,
			       unsigned size)
{
	S3C2416NandState *ns = S3C2416_NAND(opaque);

	switch (addr) {
	case NFCONF:
		break;
	case NFCONT:
		break;
	case NFCMMD:
		nand_flash_set_command(ns->flash, data);
		break;
	case NFADDR:
		nand_flash_set_address(ns->flash, data);
		break;
	case NFDATA:
		nand_flash_set_data(ns->flash, data);
		break;
	}
	ns->regs[REG_IND(addr)] = data;
}

static const MemoryRegionOps s3c2416_nand_ops = {
    .read = s3c2416_nand_read,
    .write = s3c2416_nand_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void s3c2416_nand_init(Object *obj) {}

static void s3c2416_nand_realize(DeviceState *dev, Error **errp)
{
	S3C2416NandState *ns = S3C2416_NAND(dev);

	memory_region_init_io(&ns->iomem, OBJECT(ns), &s3c2416_nand_ops, ns,
			      TYPE_S3C2416_NAND, NAND_REGION_SIZE);
	sysbus_init_mmio(SYS_BUS_DEVICE(ns), &ns->iomem);
}

static void s3c2416_nand_reset(DeviceState *dev)
{
	S3C2416NandState *ns = S3C2416_NAND(dev);

	ns->regs[REG_IND(NFCONF)] = 0x00001000;
	ns->regs[REG_IND(NFCONT)] = 0x000100c6;
	ns->regs[REG_IND(NFMECCD0)] = 0x0;
	ns->regs[REG_IND(NFMECCD1)] = 0x0;
	ns->regs[REG_IND(NFSECCD)] = 0x0;
	ns->regs[REG_IND(NFSBLK)] = 0x0;
	ns->regs[REG_IND(NFEBLK)] = 0x0;
	ns->regs[REG_IND(NFSTAT)] = 0x0080001d;
	ns->regs[REG_IND(NFECCERR1)] = 0x0;
	ns->regs[REG_IND(NFMLCBITPT)] = 0x0;
	ns->regs[REG_IND(NF8ECCERR0)] = 0x40000000;
	ns->regs[REG_IND(NF8ECCERR1)] = 0x0;
	ns->regs[REG_IND(NF8ECCERR2)] = 0x0;
	ns->regs[REG_IND(NFMLC8BITPT0)] = 0x0;
	ns->regs[REG_IND(NFMLC8BITPT1)] = 0x0;
}

static Property s3c2416_nand_properties[] = {
    //	DEFINE_PROP_PTR("flash-chip",S3C2416NandState,flash_chip),
    DEFINE_PROP_END_OF_LIST(),
};

static void s3c2416_nand_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = s3c2416_nand_realize;
	dc->props = s3c2416_nand_properties;
	dc->reset = s3c2416_nand_reset;
}

static const TypeInfo s3c2416_nand_info = {
    .name = TYPE_S3C2416_NAND,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = s3c2416_nand_init,
    .instance_size = sizeof(S3C2416NandState),
    .class_init = s3c2416_nand_class_init,
};

static void s3c2416_nand_register_types(void)
{
	type_register_static(&s3c2416_nand_info);
}

type_init(s3c2416_nand_register_types)
