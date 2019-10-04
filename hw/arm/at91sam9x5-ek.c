/*
 * at91sam9x5-ek.c
 *
 *  Created on: Apr 12, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "exec/address-spaces.h"
#include "hw/hw.h"
#include "hw/boards.h"
#include "hw/arm/at91sam9_soc.h"


#define RAM_BASE 0x20000000
#define RAM_SIZE 0x04000000

typedef struct At91sam9x5EKBoardState {
	/*< private >*/
	MachineState parent_obj;
	/*< public >*/
	At91sam9SocState soc;
	MemoryRegion ram;
} At91sam9x5EKBoardState;

static struct arm_boot_info binfo;

static void at91sam9x5_ek_init(MachineState *machine)
{
	At91sam9x5EKBoardState *board = g_new0(At91sam9x5EKBoardState, 1);

	/*
	 * initialize
	 */
	/*
	 * soc init
	 */
	object_initialize(&board->soc, (sizeof(board->soc)), TYPE_AT91SAM9_SOC);

	object_property_add_child(OBJECT(machine), "SOC", OBJECT(&board->soc),
				  &error_abort);

	/*
	 * sdram
	 */
	memory_region_allocate_system_memory(&board->ram, NULL,
					     "at91sam9.sdram", RAM_SIZE);
	memory_region_add_subregion(get_system_memory(), RAM_BASE, &board->ram);

	/*
	 * realized
	 */
	object_property_set_bool(OBJECT(&board->soc), true, "realized",
				 &error_abort);

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

static void at91sam9x5_ek_class_init(ObjectClass *oc, void *data)
{
	MachineClass *mc = MACHINE_CLASS(oc);

	mc->desc = "AT91SAM9X5-EK Boards AT91SAM9(ARM926EJ-S)";
	mc->init = at91sam9x5_ek_init;
	mc->max_cpus = 1;

	mc->no_sdcard = 1;
	mc->no_floppy = 1;
	mc->no_cdrom = 1;
	mc->no_parallel = 1;
}

static const TypeInfo at91sam9x5_ek_type = {
	.name = MACHINE_TYPE_NAME("at91sam9x5-ek"),
	.parent = TYPE_MACHINE,
	.class_init = at91sam9x5_ek_class_init,
};

static void at91sam9x5_ek_machine_init(void)
{
	type_register_static(&at91sam9x5_ek_type);
}

type_init(at91sam9x5_ek_machine_init)


