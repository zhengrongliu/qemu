/*
 * fish2416.c
 *
 *  Created on: Dec 5, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "qemu/error-report.h"
#include "cpu.h"
#include "sysemu/sysemu.h"
#include "sysemu/block-backend.h"
#include "hw/sysbus.h"
#include "hw/arm/arm.h"
#include "hw/devices.h"
#include "hw/boards.h"
#include "hw/block/nand_flash.h"
#include "hw/arm/s3c2416_soc.h"
#include "exec/address-spaces.h"


#define FISH2416_SDRAM_BASE 0x30000000

typedef struct Fish2416BoardState {
	/*< private >*/
	MachineState parent_obj;
	/*< public >*/
	S3C2416SocState soc;
	MemoryRegion sdram;
	NandFlashState nand_flash;
} Fish2416BoardState;

static struct arm_boot_info fish2416_binfo;

static void fish2416_init(MachineState *machine)
{

	Fish2416BoardState *bmc = g_new0(Fish2416BoardState, 1);
	BlockBackend *blk;

	/*
	 * initialize
	 */
	object_initialize(&bmc->soc, (sizeof(bmc->soc)), TYPE_S3C2416_SOC);

	object_property_add_child(OBJECT(machine), "soc", OBJECT(&bmc->soc),
				  &error_abort);

	/*
	 * nand flash
	 */
	object_initialize(&bmc->nand_flash, (sizeof(bmc->nand_flash)),
			  TYPE_NAND_FLASH);

	blk = blk_by_name("mtd0");
	qdev_prop_set_drive(DEVICE(&bmc->nand_flash), "drive", blk, NULL);
	qdev_prop_set_uint8(DEVICE(&bmc->nand_flash), "chip_id", 0xDA);
	qdev_prop_set_uint8(DEVICE(&bmc->nand_flash), "manufacturer_id", 0xEC);

	bmc->soc.nand.flash = &bmc->nand_flash;
	qdev_prop_set_drive(DEVICE(&bmc->nand_flash), "drive", blk, NULL);

	/*
	 * realized
	 */
	object_property_set_bool(OBJECT(&bmc->nand_flash), true, "realized",
				 &error_abort);

	memory_region_allocate_system_memory(
	    &bmc->sdram, NULL, "fish2416.sdram", machine->ram_size);
	memory_region_add_subregion(get_system_memory(), FISH2416_SDRAM_BASE,
				    &bmc->sdram);

	object_property_set_bool(OBJECT(&bmc->soc), true, "realized",
				 &error_abort);

	fish2416_binfo.ram_size = machine->ram_size;
	fish2416_binfo.kernel_filename = machine->kernel_filename;
	fish2416_binfo.kernel_cmdline = machine->kernel_cmdline;
	fish2416_binfo.initrd_filename = machine->initrd_filename;

	arm_load_kernel(ARM_CPU(first_cpu), &fish2416_binfo);
}

static void fish2416_class_init(ObjectClass *oc, void *data)
{
	MachineClass *mc = MACHINE_CLASS(oc);

	mc->desc = "Samsung FISH2416 (ARM926EJ-S)";
	mc->init = fish2416_init;
	mc->max_cpus = 1;

	mc->no_sdcard = 1;
	mc->no_floppy = 1;
	mc->no_cdrom = 1;
	mc->no_parallel = 1;
}

static const TypeInfo fish2416_type = {
	.name = MACHINE_TYPE_NAME("fish2416"),
	.parent = TYPE_MACHINE,
	.class_init = fish2416_class_init,
};

static void fish2416_machine_init(void)
{
	type_register_static(&fish2416_type);
}

type_init(fish2416_machine_init)


