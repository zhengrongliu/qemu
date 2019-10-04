/*
 * s3c2416_soc.c
 *
 *  Created on: Dec 5, 2016
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/arm/arm.h"
#include "hw/sysbus.h"
#include "exec/address-spaces.h"
#include "qapi/error.h"
#include "sysemu/sysemu.h"
#include "cpu.h"
#include "hw/arm/s3c2416_soc.h"


#define SRAM_BASE_ADDRESS (0x40000000)
#define SRAM_SIZE (64 * 1024)

#define SYSCON_BASE_ADDRESS 0x4c000000
#define NAND_BASE_ADDRESS 0x4E000000
#define VIC_BASE_ADDRESS 0x4a000000
#define TIMER_BASE_ADDRESS 0x51000000
#define UART_BASE_ADDRESS(x) (0x50000000 + 0x4000 * (x))
#define LCD_BASE_ADDRESS 0x4c800000

static const int timer_irqs[] = {10, 11, 12, 13, 14};

static const int uart_irqs[][3] = {
    {HWIRQ_N_UART0_RXD, HWIRQ_N_UART0_TXD, HWIRQ_N_UART0_ERR},
    {HWIRQ_N_UART1_RXD, HWIRQ_N_UART1_TXD, HWIRQ_N_UART1_ERR},
    {HWIRQ_N_UART2_RXD, HWIRQ_N_UART2_TXD, HWIRQ_N_UART2_ERR},
    {HWIRQ_N_UART3_RXD, HWIRQ_N_UART3_TXD, HWIRQ_N_UART3_ERR},
};

static void s3c2416_soc_init(Object *obj)
{
	S3C2416SocState *state = S3C2416_SOC(obj);
	int i;

	/*
	 * cpu
	 */
	object_initialize(&state->cpu, sizeof(state->cpu),
			  ARM_CPU_TYPE_NAME("arm926"));
	object_property_add_child(obj, "cpu", OBJECT(&state->cpu), NULL);

	/*
	 * vic
	 */
	object_initialize(&state->vic, sizeof(state->vic), TYPE_S3C2416_VIC);
	qdev_set_parent_bus(DEVICE(&state->vic), sysbus_get_default());

	/*
	 * pwm timer
	 */
	object_initialize(&state->timer, sizeof(state->timer),
			  TYPE_S3C2416_TIMER);
	qdev_set_parent_bus(DEVICE(&state->timer), sysbus_get_default());

	/*
	 * uart
	 */
	for (i = 0; i < S3C2416_NR_UARTS; i++) {
		object_initialize(&state->uarts[i], sizeof(state->uarts[i]),
				  TYPE_S3C2416_UART);
		qdev_set_parent_bus(DEVICE(&state->uarts[i]),
				    sysbus_get_default());
	}

	/*
	 * lcd controller
	 */
	object_initialize(&state->lcd, sizeof(state->lcd), TYPE_S3C2416_LCD);
	qdev_set_parent_bus(DEVICE(&state->lcd), sysbus_get_default());

	/*
	 * nand controller
	 */
	object_initialize(&state->nand, sizeof(state->nand), TYPE_S3C2416_NAND);
	qdev_set_parent_bus(DEVICE(&state->nand), sysbus_get_default());

	/*
	 * syscon
	 */
	object_initialize(&state->syscon, sizeof(state->syscon),
			  TYPE_S3C2416_SYSCON);
	qdev_set_parent_bus(DEVICE(&state->syscon), sysbus_get_default());
}

static void s3c2416_soc_realize(DeviceState *dev_soc, Error **errp)
{

	MemoryRegion *system_memory = get_system_memory();
	MemoryRegion *sram = g_new(MemoryRegion, 1);
	S3C2416SocState *s = S3C2416_SOC(dev_soc);
	int i = 0, j = 0;

	memory_region_init_ram(sram, NULL, "s3c2416-soc.sram", SRAM_SIZE,
			       &error_fatal);
	memory_region_add_subregion(system_memory, SRAM_BASE_ADDRESS, sram);

	/*
	 * cpu
	 */
	object_property_set_bool(OBJECT(&s->cpu), true, "realized", &error_abort);


	/*
	 * vic
	 */
	S3C2416VicState *vs = &s->vic;
	object_property_set_bool(OBJECT(vs), true, "realized", &error_abort);
	sysbus_mmio_map(SYS_BUS_DEVICE(vs), 0, VIC_BASE_ADDRESS);

	/*
	 * connect irq to arm core
	 */
	sysbus_connect_irq(SYS_BUS_DEVICE(vs), 0,
			   qdev_get_gpio_in(DEVICE(&s->cpu), ARM_CPU_IRQ));
	/*
	 * connect fiq to arm core
	 */
	sysbus_connect_irq(SYS_BUS_DEVICE(vs), 1,
			   qdev_get_gpio_in(DEVICE(&s->cpu), ARM_CPU_FIQ));

	/*
	 * timer
	 */
	S3C2416TimerState *ts = &s->timer;
	object_property_set_int(OBJECT(ts), 66000000, "clock-freq",
				&error_abort);
	object_property_set_bool(OBJECT(ts), true, "realized", &error_abort);

	sysbus_mmio_map(SYS_BUS_DEVICE(ts), 0, TIMER_BASE_ADDRESS);

	for (i = 0; i < ts->nr_irqs; i++) {
		sysbus_connect_irq(SYS_BUS_DEVICE(ts), i,
				   qdev_get_gpio_in(DEVICE(vs), timer_irqs[i]));
	}

	/*
	 * uart
	 */
	S3C2416UartState *us = NULL;
	for (i = 0; i < S3C2416_NR_UARTS; i++) {
		us = &s->uarts[i];
		qdev_prop_set_uint32(DEVICE(us), "rx_fifo_size", 64);
		qdev_prop_set_chr(DEVICE(us), "chardev",
				  i < MAX_SERIAL_PORTS ? serial_hds[i] : NULL);

		object_property_set_bool(OBJECT(us), true, "realized",
					 &error_abort);

		sysbus_mmio_map(SYS_BUS_DEVICE(us), 0, UART_BASE_ADDRESS(i));

		for (j = 0; j < 3; j++) {
			sysbus_connect_irq(
			    SYS_BUS_DEVICE(us), j,
			    qdev_get_gpio_in(DEVICE(vs), uart_irqs[i][j]));
		}
	}

	/*
	 * nand controller
	 */
	S3C2416NandState *ns = &s->nand;
	object_property_set_bool(OBJECT(ns), true, "realized", &error_abort);

	sysbus_mmio_map(SYS_BUS_DEVICE(ns), 0, NAND_BASE_ADDRESS);

	/*
	 * lcd controller
	 */
	S3C2416LCDState *ls = &s->lcd;
	object_property_set_bool(OBJECT(ls), true, "realized", &error_abort);

	sysbus_mmio_map(SYS_BUS_DEVICE(ls), 0, LCD_BASE_ADDRESS);

	/*
	 * syscon
	 */
	S3C2416SysconState *ss = &s->syscon;

	qdev_prop_set_uint64(DEVICE(ss), "crystal-freq", 12000000);
	qdev_prop_set_uint64(DEVICE(ss), "osc-freq", 12000000);
	qdev_prop_set_ptr(DEVICE(ss), "timer-device", ts);

	object_property_set_bool(OBJECT(ss), true, "realized", &error_abort);

	sysbus_mmio_map(SYS_BUS_DEVICE(ss), 0, SYSCON_BASE_ADDRESS);
}

static Property s3c2416_soc_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};

static void s3c2416_soc_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = s3c2416_soc_realize;
	dc->props = s3c2416_soc_properties;
}

static const TypeInfo s3c2416_soc_info = {
	.name = TYPE_S3C2416_SOC,
	.parent = TYPE_DEVICE,
	.instance_init = s3c2416_soc_init,
	.instance_size = sizeof(S3C2416SocState),
	.class_init = s3c2416_soc_class_init,
};

static void s3c2416_soc_register_types(void)
{
	type_register_static(&s3c2416_soc_info);
}

type_init(s3c2416_soc_register_types)


