/*
 * at91sam9_soc.c
 *
 *  Created on: Apr 12, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "sysemu/sysemu.h"
#include "hw/hw.h"
#include "hw/qdev.h"
#include "hw/arm/at91sam9_soc.h"

#define AIC_ADDRESS 		0xFFFFF000
#define DMAC_ADDRESS(i) 	(0xFFFFEC00 + (i)*0x200)
#define DBGU_ADDRESS 		0xFFFFF200
#define PMC_ADDRESS 		0xFFFFFC00
#define PIT_ADDRESS 		0xFFFFFE30
#define SCKC_ADDRESS 		0xFFFFFE50
#define EMAC_ADDRESS 		0xF802C000
#define HSMCI_ADDRESS(i) 	(0xF0008000 + i * 0x4000)
#define PIO_ADDRESS(i) 		(0xFFFFF400 + i * 0x200)

static void at91sam9_soc_init(Object *obj)
{
	At91sam9SocState *soc = AT91SAM9_SOC(obj);
	int i;

	/*
	 * cpu
	 */
	object_initialize(&soc->cpu, sizeof(soc->cpu),
			  ARM_CPU_TYPE_NAME("arm926"));
	object_property_add_child(obj, "cpu", OBJECT(&soc->cpu), NULL);

	/*
	 * sckc
	 */
	object_initialize(&soc->sckc, sizeof(soc->sckc), TYPE_AT91SAM9_SCKC);
	qdev_set_parent_bus(DEVICE(&soc->sckc), sysbus_get_default());

	/*
	 * pmc
	 */
	object_initialize(&soc->pmc, sizeof(soc->pmc), TYPE_AT91SAM9_PMC);
	qdev_set_parent_bus(DEVICE(&soc->pmc), sysbus_get_default());

	/*
	 * pio
	 */
	for (i = 0; i < sizeof(soc->pio) / sizeof(soc->pio[0]); i++) {
		object_initialize(&soc->pio[i], sizeof(soc->pio[i]),
				  TYPE_AT91SAM9_PIO);
		qdev_set_parent_bus(DEVICE(&soc->pio[i]), sysbus_get_default());
	}

	/*
	 * aic
	 */
	object_initialize(&soc->aic, sizeof(soc->aic), TYPE_AT91SAM9_AIC);
	qdev_set_parent_bus(DEVICE(&soc->aic), sysbus_get_default());


	/*
	 * dmac
	 */
	object_initialize(&soc->dmac, sizeof(soc->dmac), TYPE_AT91SAM9_DMAC);
	qdev_set_parent_bus(DEVICE(&soc->dmac), sysbus_get_default());


	/*
	 * pit
	 */
	object_initialize(&soc->pit, sizeof(soc->pit), TYPE_AT91SAM9_PIT);
	qdev_set_parent_bus(DEVICE(&soc->pit), sysbus_get_default());

	/*
	 * dbgu
	 */
	object_initialize(&soc->dbgu, sizeof(soc->dbgu), TYPE_AT91SAM9_UART);
	qdev_set_parent_bus(DEVICE(&soc->dbgu), sysbus_get_default());

	/*
	 * emac
	 */
	object_initialize(&soc->emac, sizeof(soc->emac), TYPE_AT91SAM9_EMAC);
	qdev_set_parent_bus(DEVICE(&soc->emac), sysbus_get_default());

	/*
	 * hsmci
	 */
	object_initialize(&soc->hsmci, sizeof(soc->hsmci), TYPE_AT91SAM9_HSMCI);
	qdev_set_parent_bus(DEVICE(&soc->hsmci), sysbus_get_default());
}

static void at91sam9_soc_realize(DeviceState *obj, Error **errp)
{
	At91sam9SocState *soc = AT91SAM9_SOC(obj);
	int i = 0;
	Error *err = NULL;

	/*
	 * cpu
	 */
	object_property_set_bool(OBJECT(&soc->cpu), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}

	/*
	 * sckc
	 */
	At91sam9SckcState *ss = &soc->sckc;
	object_property_set_uint(OBJECT(ss), SCKC_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(ss), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}

	/*
	 * pmc
	 */
	At91sam9PmcState *ps = &soc->pmc;
	object_property_set_uint(OBJECT(ps), PMC_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(ps), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}


	/*
	 * aic
	 */
	At91sam9AicState *aic = &soc->aic;
	object_property_set_uint(OBJECT(aic), AIC_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(aic), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}


	sysbus_mmio_map(SYS_BUS_DEVICE(aic), 0, AIC_ADDRESS);

	sysbus_connect_irq(SYS_BUS_DEVICE(aic), 0,
			   qdev_get_gpio_in(DEVICE(&soc->cpu), ARM_CPU_IRQ));
	sysbus_connect_irq(SYS_BUS_DEVICE(aic), 1,
			   qdev_get_gpio_in(DEVICE(&soc->cpu), ARM_CPU_FIQ));

	/*
	 * dmac
	 */
	At91sam9DmacState *dmac = &soc->dmac;
	object_property_set_uint(OBJECT(dmac), DMAC_ADDRESS(0), "addr", &err);
	object_property_set_bool(OBJECT(dmac), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}

	sysbus_connect_irq(SYS_BUS_DEVICE(dmac), 0,
			   qdev_get_gpio_in(DEVICE(aic), 20));

	/*
	 * pio
	 */
	for (i = 0; i < sizeof(soc->pio) / sizeof(soc->pio[0]); i++) {
		At91sam9PioState *pio = &soc->pio[i];
		object_property_set_uint(OBJECT(pio), PIO_ADDRESS(i), "addr", &err);
		object_property_set_bool(OBJECT(pio), true, "realized",
					 &error_abort);

	}

	/*
	 * pit
	 */
	At91sam9PitState *pit = &soc->pit;
	object_property_set_uint(OBJECT(pit), PIT_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(pit), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}


	sysbus_connect_irq(SYS_BUS_DEVICE(pit), 0,
			   qdev_get_gpio_in(DEVICE(aic), 1));

	/*
	 * dbgu
	 */
	At91sam9UartState *dbgu = &soc->dbgu;
	qdev_prop_set_chr(DEVICE(dbgu), "chardev", serial_hds[0]);

	object_property_set_uint(OBJECT(dbgu), DBGU_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(dbgu), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}


	sysbus_connect_irq(SYS_BUS_DEVICE(dbgu), 0,
			   qdev_get_gpio_in(DEVICE(aic), 1));

	/*
	 * emac
	 */
	At91sam9EmacState *emac = &soc->emac;

	if (nd_table[0].used) {
		qemu_check_nic_model(&nd_table[0], TYPE_AT91SAM9_EMAC);
		qdev_set_nic_properties(DEVICE(emac), &nd_table[0]);
	}

	object_property_set_uint(OBJECT(emac), EMAC_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(emac), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}

	sysbus_connect_irq(SYS_BUS_DEVICE(emac), 0,
			   qdev_get_gpio_in(DEVICE(aic), 24));

	/*
	 * hsmci
	 */
	At91sam9HsmciState *hsmci = &soc->hsmci;


	object_property_set_uint(OBJECT(hsmci), HSMCI_ADDRESS(0), "addr", &err);
	object_property_set_bool(OBJECT(hsmci), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}

	sysbus_connect_irq(SYS_BUS_DEVICE(hsmci), 0,
			   qdev_get_gpio_in(DEVICE(aic), 12));

}

static Property at91sam9_soc_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};

static void at91sam9_soc_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = at91sam9_soc_realize;
	dc->props = at91sam9_soc_properties;
}

static const TypeInfo at91sam9_soc_info = {
	.name = TYPE_AT91SAM9_SOC,
	.parent = TYPE_DEVICE,
	.instance_init = at91sam9_soc_init,
	.instance_size = sizeof(At91sam9SocState),
	.class_init = at91sam9_soc_class_init,
};

static void at91sam9_soc_register_types(void)
{
	type_register_static(&at91sam9_soc_info);
}

type_init(at91sam9_soc_register_types)


