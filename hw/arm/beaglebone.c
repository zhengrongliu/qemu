/*
 * beaglebone.c
 *
 *  Created on: Feb 5, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "exec/address-spaces.h"
#include "hw/hw.h"
#include "hw/boards.h"
#include "hw/arm/am335x_soc.h"

#define RAM_BASE 0x80000000
#define RAM_SIZE 0x40000000

typedef struct BeagleBoneState {
	/*< private >*/
	MachineState parent_obj;
	/*< public >*/
	AM335XSocState soc;
	MemoryRegion ram;
} BeagleBoneState;

static struct arm_boot_info binfo;

static void beaglebone_init(MachineState *machine)
{
	BeagleBoneState *board = g_new0(BeagleBoneState, 1);

	/*
	 * initialize
	 */
	object_initialize(&board->soc, (sizeof(board->soc)), TYPE_AM335X_SOC);

	object_property_add_child(OBJECT(machine), "soc", OBJECT(&board->soc),
				  &error_abort);
	/*
	 * realized
	 */
	object_property_set_bool(OBJECT(&board->soc), true, "realized",
				 &error_abort);


	memory_region_allocate_system_memory(
	    &board->ram, NULL, "beaglebone.sdram", RAM_SIZE);
	memory_region_add_subregion(get_system_memory(), RAM_BASE, &board->ram);

	/*
	 * firmware
	 */
	binfo.ram_size = machine->ram_size;
	binfo.kernel_filename = machine->kernel_filename;
	binfo.kernel_cmdline = machine->kernel_cmdline;
	binfo.initrd_filename = machine->initrd_filename;
#if 0
    arm_load_kernel(ARM_CPU(first_cpu), &binfo);
#endif
}

static void beaglebone_machine_init(MachineClass *mc)
{

	mc->desc = "TI BeagleBone AM335X(Cortex-A8)";
	mc->init = beaglebone_init;
	mc->max_cpus = 1;

	mc->no_sdcard = 1;
	mc->no_floppy = 1;
	mc->no_cdrom = 1;
	mc->no_parallel = 1;
}

DEFINE_MACHINE("beaglebone", beaglebone_machine_init)
