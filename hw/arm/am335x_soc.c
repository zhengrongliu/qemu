/*
 * am335x_soc.c
 *
 *  Created on: Feb 5, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "hw/hw.h"
#include "hw/qdev.h"
#include "hw/sysbus.h"
#include "hw/arm/am335x_soc.h"
#include "cpu.h"

/*
 * device address
 */
#define INTC_ADDRESS		(0x48200000)
#define PRCM_ADDRESS		(0x44e00000)
#define CM_ADDRESS		(0x44e10000)
#define EDMA_ADDRESS(x)		(edma_io_addr[x])
#define UART_ADDRESS(x) 	(uarts_io_addr[x])
#define TIMER_ADDRESS(x)	(timers_io_addr[x])
#define MCASP_ADDRESS(x)	(mcasps_io_addr[x])
#define I2C_ADDRESS(x)		(i2cs_io_addr[x])
//#define VHCI_ADDRESS		(0x48000000)
#define VPCI_HOST_MMCFG_ADDRESS	(0xc0000000)
#define VPCI_HOST_MMCFG_SIZE	(0x01000000)
#define VPCI_HOST_MMIO_ADDRESS	(0xe0000000)
#define VPCI_HOST_MMIO_SIZE	(0x10000000)

/*
 * device irq
 */
#define TIMER_IRQ(x)		(timers_irq_nr[x])
#define UART_IRQ(x)		(uarts_irq_nr[x])
#define MCASP_IRQ(x,y)		(mcasps_irq_nr[2*(x)+y])
#define I2C_IRQ(x)		(i2cs_irq_nr[x])
#define EDMA_IRQ(x,y)		(edma_irq_nr[3*(x)+y])
//#define VHCI_IRQ		23
#define VPCI_HOST_IRQ(x)	(vpci_host_irq_nr[x])

/*
 * device dma event
 */
#define MCASP_DMA_EVENT(x,y)	(mcasps_dma_event[2*(x)+y])
static hwaddr timers_io_addr[]={
	0x48040000, /* DMTimer2 */
};
static uint32_t timers_irq_nr[] = {
	68, /* TINT2 */
};

static hwaddr uarts_io_addr[]={
	0x44e09000, /* uart0 */
};
static uint32_t uarts_irq_nr[] = {
	72, /* UART0 */
};

static hwaddr mcasps_io_addr[] = {
	0x48038000, /* MCASP 0 */
	0x4803c000, /* MCASP 1 */
};
static uint32_t mcasps_irq_nr[] = {
	80, /* MCATXINT0 */
	81, /* MCARXINT0 */
	82, /* MCATXINT1 */
	83, /* MCARXINT1 */
};
static hwaddr edma_io_addr[] = {
	0x49000000,
};
static uint32_t edma_irq_nr[] = {
	12, /* EDMA CCINT */
	13, /* EDMA MPINT */
	14, /* EDMA ERRINT */
};
#if 0
static uint32_t mcasps_dma_event[] = {
	8,
	9,
	10,
	11,
};
#endif

static uint32_t vpci_host_irq_nr[] = {
	15,16,17,18
};


static hwaddr i2cs_io_addr[] = {
	0x44e0b000,
};

static uint32_t i2cs_irq_nr[] = {
	70,
};


static DeviceState *sysbus_create_device(const char *name,
                                   hwaddr *addrs,int naddr,DeviceState *condev,uint32_t * irqs,int nirq)
{
	DeviceState *dev;
	SysBusDevice *s;
	int n;

	dev = qdev_create(NULL, name);
	s = SYS_BUS_DEVICE(dev);

	n = 0;
	while( n < naddr){
		sysbus_mmio_map(s, n, addrs[n]);
		n++;
	}

	n = 0;
	while( n < nirq){
		sysbus_connect_irq(s, n, qdev_get_gpio_in(condev,irqs[n]));
		n++;
	}

	return dev;
}

static void am335x_soc_init(Object *obj)
{
	AM335XSocState *soc = AM335X_SOC(obj);
	Error *err = NULL;
	/*
	 * cpu
	 */
	object_initialize(&soc->cpu, sizeof(soc->cpu),
                          ARM_CPU_TYPE_NAME("cortex-a8"));

	object_property_add_child(OBJECT(obj), "cpu", OBJECT(&soc->cpu), &err);

}

static void am335x_soc_realize(DeviceState *soc_dev, Error **errp)
{
	AM335XSocState *soc = AM335X_SOC(soc_dev);
	DeviceState *intc,*edma;
	DeviceState *dev;
	int i;

	/*
	 * cpu
	 */

	qdev_init_nofail(DEVICE(&soc->cpu));

	/*
	 * intc
	 */
	intc = sysbus_create_varargs("am335x-intc",INTC_ADDRESS,
				qdev_get_gpio_in(DEVICE(&soc->cpu),ARM_CPU_IRQ),
				qdev_get_gpio_in(DEVICE(&soc->cpu),ARM_CPU_FIQ),NULL);

	qdev_set_parent_bus(intc, sysbus_get_default());


	/*
	 * prcm
	 */
	dev = sysbus_create_simple("am335x-prcm",PRCM_ADDRESS,NULL);
	qdev_set_parent_bus(dev, sysbus_get_default());


	/*
	 * cm
	 */
	dev = sysbus_create_simple("am335x-cm",CM_ADDRESS,NULL);
	qdev_set_parent_bus(dev, sysbus_get_default());


	/*
	 * edma
	 */
	edma = sysbus_create_device("am335x-edma",&EDMA_ADDRESS(0),1,
			intc,&EDMA_IRQ(0,0),3);

	qdev_set_parent_bus(edma, sysbus_get_default());
	qdev_init_nofail(edma);

	/*
	 * timer
	 */
	for(i = 0;i < ARRAY_SIZE(timers_io_addr);i++){
		dev = sysbus_create_device("am335x-timer",&TIMER_ADDRESS(i),1,
				intc,&TIMER_IRQ(i),1);

		qdev_set_parent_bus(dev, sysbus_get_default());
		qdev_init_nofail(dev);

	}

	/*
	 * uart
	 */
	for(i = 0;i < ARRAY_SIZE(uarts_io_addr);i++){
		dev = sysbus_create_device("am335x-uart",&UART_ADDRESS(i),1,
				intc,&UART_IRQ(i),1);

		qdev_set_parent_bus(dev, sysbus_get_default());

		qdev_prop_set_chr(dev, "chardev", serial_hds[i]);

		qdev_init_nofail(dev);

	}


	/*
	 * mcasps
	 */
	for(i = 0;i < ARRAY_SIZE(mcasps_io_addr);i++){
		dev = sysbus_create_device("am335x-mcasp",&MCASP_ADDRESS(i),1,
				intc,&MCASP_IRQ(i,0),2);

		qdev_set_parent_bus(dev, sysbus_get_default());

		qdev_init_nofail(dev);

	}

	/*
	 * i2c
	 */
	for(i = 0;i < ARRAY_SIZE(i2cs_io_addr);i++){
		dev = sysbus_create_device("am335x-i2c",&I2C_ADDRESS(i),1,
				intc,&I2C_IRQ(i),1);

		qdev_set_parent_bus(dev, sysbus_get_default());

		qdev_init_nofail(dev);

	}


	MemoryRegion *mmio_reg;
	MemoryRegion *mmio_alias;
	MemoryRegion *ecam_reg;
	MemoryRegion *ecam_alias;

	dev = qdev_create(NULL, "gpex-pcihost");
	qdev_init_nofail(dev);


	mmio_alias = g_new0(MemoryRegion,1);
	mmio_reg = sysbus_mmio_get_region(SYS_BUS_DEVICE(dev),1);
	memory_region_init_alias(mmio_alias,OBJECT(dev),"pcie-mmio",
			mmio_reg,VPCI_HOST_MMIO_ADDRESS,VPCI_HOST_MMIO_SIZE);
	memory_region_add_subregion(get_system_memory(),
			VPCI_HOST_MMIO_ADDRESS,mmio_alias);

	ecam_alias = g_new0(MemoryRegion,1);
	ecam_reg = sysbus_mmio_get_region(SYS_BUS_DEVICE(dev),0);
	memory_region_init_alias(ecam_alias,OBJECT(dev),"pcie-ecam",
			ecam_reg,0,VPCI_HOST_MMCFG_SIZE);
	memory_region_add_subregion(get_system_memory(),
			VPCI_HOST_MMCFG_ADDRESS,ecam_alias);


	sysbus_mmio_map(SYS_BUS_DEVICE(dev), 2, 0xd0000000);
	sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0,
			   qdev_get_gpio_in(DEVICE(intc), VPCI_HOST_IRQ(0)));
	sysbus_connect_irq(SYS_BUS_DEVICE(dev), 1,
			   qdev_get_gpio_in(DEVICE(intc), VPCI_HOST_IRQ(1)));
	sysbus_connect_irq(SYS_BUS_DEVICE(dev), 2,
			   qdev_get_gpio_in(DEVICE(intc), VPCI_HOST_IRQ(2)));
	sysbus_connect_irq(SYS_BUS_DEVICE(dev), 3,
			   qdev_get_gpio_in(DEVICE(intc), VPCI_HOST_IRQ(3)));


#if 0
	/*
	 * vhost TODO:delete
	 */
	GPEXHost *gpex = &soc->gpex;
	object_property_set_bool(OBJECT(gpex), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}

	sysbus_mmio_map(SYS_BUS_DEVICE(gpex), 0, VHOST_PCI_MMCFG_ADDRESS);
	sysbus_mmio_map(SYS_BUS_DEVICE(gpex), 1, VHOST_PCI_MMIO_ADDRESS);

	MemoryRegion *mmio_reg;
	MemoryRegion *mmio_alias;

	mmio_alias = g_new0(MemoryRegion, 1);

	mmio_reg = sysbus_mmio_get_region(SYS_BUS_DEVICE(gpex), 1);
	memory_region_init_alias(mmio_alias, OBJECT(gpex), "pcie-mmio",
                             mmio_reg, VHOST_PCI_MMIO_ADDRESS, VHOST_PCI_MMIO_SIZE);
	memory_region_add_subregion(get_system_memory(), VHOST_PCI_MMIO_ADDRESS, mmio_alias);



	sysbus_connect_irq(SYS_BUS_DEVICE(gpex), 0,
			   qdev_get_gpio_in(DEVICE(intc), VHOST_IRQ(0)));
	sysbus_connect_irq(SYS_BUS_DEVICE(gpex), 1,
			   qdev_get_gpio_in(DEVICE(intc), VHOST_IRQ(1)));
	sysbus_connect_irq(SYS_BUS_DEVICE(gpex), 2,
			   qdev_get_gpio_in(DEVICE(intc), VHOST_IRQ(2)));
	sysbus_connect_irq(SYS_BUS_DEVICE(gpex), 3,
			   qdev_get_gpio_in(DEVICE(intc), VHOST_IRQ(3)));

	/*
	 * timers
	 */
	for(i = 0;i < ARRAY_SIZE(soc->timers);i++){
		object_property_set_uint(OBJECT(&soc->timers[i]), TIMER_ADDRESS(i), "addr", &err);
		object_property_set_bool(OBJECT(&soc->timers[i]), true, "realized", &err);
		if (err) {
			error_propagate(errp, err);
			return;
		}
		sysbus_connect_irq(SYS_BUS_DEVICE(&soc->timers[i]), 0,
				qdev_get_gpio_in(DEVICE(intc), TIMER_IRQ(i)));
	}


        /*
         * uarts
         */
	for(i = 0;i < ARRAY_SIZE(soc->uarts);i++){
		qdev_prop_set_chr(DEVICE(&soc->uarts[i]), "chardev", serial_hds[i]);

		object_property_set_uint(OBJECT(&soc->uarts[i]), UART_ADDRESS(i), "addr", &err);
		object_property_set_bool(OBJECT(&soc->uarts[i]), true, "realized", &err);
		if (err) {
			error_propagate(errp, err);
			return;
		}
		sysbus_connect_irq(SYS_BUS_DEVICE(&soc->uarts[i]), 0,
				qdev_get_gpio_in(DEVICE(intc), UART_IRQ(i)));
	}

	/*
         * mcasps
         */
	for(i = 0;i < ARRAY_SIZE(soc->mcasps);i++){
		object_property_set_uint(OBJECT(&soc->mcasps[i]), MCASP_ADDRESS(i), "addr", &err);
		object_property_set_bool(OBJECT(&soc->mcasps[i]), true, "realized", &err);
		if (err) {
			error_propagate(errp, err);
			return;
		}

		sysbus_connect_irq(SYS_BUS_DEVICE(&soc->mcasps[i]), 0,
				qdev_get_gpio_in(DEVICE(intc), MCASP_IRQ(i,0)));
		sysbus_connect_irq(SYS_BUS_DEVICE(&soc->mcasps[i]), 1,
				qdev_get_gpio_in(DEVICE(intc), MCASP_IRQ(i,1)));

		sysbus_connect_irq(SYS_BUS_DEVICE(&soc->mcasps[i]), 2,
				qdev_get_gpio_in(DEVICE(edma), MCASP_DMA_EVENT(i,0)));
		sysbus_connect_irq(SYS_BUS_DEVICE(&soc->mcasps[i]), 3,
				qdev_get_gpio_in(DEVICE(edma), MCASP_DMA_EVENT(i,1)));
	}

	/*
	 * i2cs
	 */
	for(i = 0;i < ARRAY_SIZE(soc->i2cs);i++){
		object_property_set_uint(OBJECT(&soc->i2cs[i]), I2C_ADDRESS(i), "addr", &err);
		object_property_set_bool(OBJECT(&soc->i2cs[i]), true, "realized", &err);
		if (err) {
			error_propagate(errp, err);
			return;
		}
		sysbus_connect_irq(SYS_BUS_DEVICE(&soc->i2cs[i]), 0,
				qdev_get_gpio_in(DEVICE(intc), I2C_IRQ(i)));
	}
#endif

}

static Property am335x_soc_properties[] = {
	DEFINE_PROP_END_OF_LIST(),
};

static void am335x_soc_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = am335x_soc_realize;
	dc->desc = "AM335X SOC";
	dc->props = am335x_soc_properties;

	dc->user_creatable = false;
}

static const TypeInfo am335x_soc_info = {
	.name = TYPE_AM335X_SOC,
	.parent = TYPE_DEVICE,
	.instance_init = am335x_soc_init,
	.instance_size = sizeof(AM335XSocState),
	.class_init = am335x_soc_class_init,
};

static void am335x_soc_register_types(void)
{
	type_register_static(&am335x_soc_info);
}

type_init(am335x_soc_register_types)
