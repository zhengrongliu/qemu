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
#define EDMA_ADDRESS		(0x49000000)
#define UART_ADDRESS(x) 	(uarts_io_addr[x])
#define TIMER_ADDRESS(x)	(timers_io_addr[x])
#define MCASP_ADDRESS(x)	(mcasps_io_addr[x])
#define I2C_ADDRESS(x)		(i2cs_io_addr[x])
//#define VHCI_ADDRESS		(0x48000000)
#define VHOST_PCI_MMCFG_ADDRESS	(0xc0000000)
#define VHOST_PCI_MMCFG_SIZE	(0x01000000)
#define VHOST_PCI_MMIO_ADDRESS	(0xe0000000)
#define VHOST_PCI_MMIO_SIZE	(0x10000000)

/*
 * device irq
 */
#define TIMER_IRQ(x)		(timers_irq_nr[x])
#define UART_IRQ(x)		(uarts_irq_nr[x])
#define MCASP_IRQ(x,y)		(mcasps_irq_nr[2*(x)+y])
#define I2C_IRQ(x)		(i2cs_irq_nr[x])
#define EDMA_IRQ(x,y)		(edma_irq_nr[3*(x)+y])
//#define VHCI_IRQ		23
#define VHOST_IRQ(x)		(vhost_irq_nr[x])

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

static uint32_t vhost_irq_nr[] = {
	15,16,17,18
};
#endif


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

#if 0
        /*
         * intc
         */
        object_initialize(&soc->intc, sizeof(soc->intc), TYPE_AM335X_INTC);
        qdev_set_parent_bus(DEVICE(&soc->intc), sysbus_get_default());


        /*
         * prcm
         */
        object_initialize(&soc->prcm, sizeof(soc->prcm), TYPE_AM335X_PRCM);
        qdev_set_parent_bus(DEVICE(&soc->prcm), sysbus_get_default());

        /*
         * cm
         */
        object_initialize(&soc->cm, sizeof(soc->cm), TYPE_AM335X_CM);
        qdev_set_parent_bus(DEVICE(&soc->cm), sysbus_get_default());

        /*
         * edma
         */
        object_initialize(&soc->edma, sizeof(soc->edma), TYPE_AM335X_EDMA);
        qdev_set_parent_bus(DEVICE(&soc->edma), sysbus_get_default());

        /*
         * vhost
         */
        object_initialize(&soc->gpex, sizeof(soc->gpex), TYPE_GPEX_HOST);
        qdev_set_parent_bus(DEVICE(&soc->gpex), sysbus_get_default());



        /*
         * timers
         */
        for(i = 0;i < ARRAY_SIZE(soc->timers);i++){
        	object_initialize(&soc->timers[i], sizeof(soc->timers[i]), TYPE_AM335X_TIMER);
        	qdev_set_parent_bus(DEVICE(&soc->timers[i]), sysbus_get_default());
        }

	/*
	 * uarts
	 */
        for(i = 0;i < ARRAY_SIZE(soc->uarts);i++){
        	object_initialize(&soc->uarts[i], sizeof(soc->uarts[i]), TYPE_AM335X_UART);
        	qdev_set_parent_bus(DEVICE(&soc->uarts[i]), sysbus_get_default());
        }

       	/*
	 * mcasps
	 */
        for(i = 0;i < ARRAY_SIZE(soc->mcasps);i++){
        	object_initialize(&soc->mcasps[i], sizeof(soc->mcasps[i]), TYPE_AM335X_MCASP);
        	qdev_set_parent_bus(DEVICE(&soc->mcasps[i]), sysbus_get_default());
        }

        /*
         * i2cs
         */
        for(i = 0;i < ARRAY_SIZE(soc->i2cs);i++){
        	object_initialize(&soc->i2cs[i], sizeof(soc->i2cs[i]), TYPE_AM335X_I2C);
        	qdev_set_parent_bus(DEVICE(&soc->i2cs[i]), sysbus_get_default());
        }
#endif
}

static void am335x_soc_realize(DeviceState *soc_dev, Error **errp)
{
	AM335XSocState *soc = AM335X_SOC(soc_dev);
	Error *err = NULL;
	DeviceState *intc,*edma;
	DeviceState *dev;
	int i;

	/*
	 * cpu
	 */
	object_initialize(&soc->cpu, sizeof(soc->cpu),
                          ARM_CPU_TYPE_NAME("cortex-a8"));

	object_property_add_child(OBJECT(soc_dev), "cpu", OBJECT(&soc->cpu), &err);
	object_property_set_bool(OBJECT(&soc->cpu), true, "realized", &err);
    if (err) {
    	error_propagate(errp, err);
    	return;
    }

    /*
     * intc
     */
    intc = sysbus_create_varargs("am335x-intc",INTC_ADDRESS,
				qdev_get_gpio_in(DEVICE(&soc->cpu),ARM_CPU_IRQ),
				qdev_get_gpio_in(DEVICE(&soc->cpu),ARM_CPU_FIQ),NULL);

	qdev_set_parent_bus(intc, sysbus_get_default());

	object_property_set_bool(OBJECT(intc), true, "realized", &err);
    if (err) {
    	error_propagate(errp, err);
    	return;
    }

	/*
	 * prcm
	 */
	dev = sysbus_create_simple("am335x-prcm",PRCM_ADDRESS,NULL);
	qdev_set_parent_bus(dev, sysbus_get_default());

	object_property_set_bool(OBJECT(dev), true, "realized", &err);
    if (err) {
    	error_propagate(errp, err);
    	return;
    }

	/*
	 * cm
	 */
	dev = sysbus_create_simple("am335x-cm",CM_ADDRESS,NULL);
	qdev_set_parent_bus(dev, sysbus_get_default());

	object_property_set_bool(OBJECT(dev), true, "realized", &err);
    if (err) {
    	error_propagate(errp, err);
    	return;
    }
	/*
	 * edma
	 */
	edma = sysbus_create_varargs("am335x-edma",EDMA_ADDRESS,
			qdev_get_gpio_in(intc,EDMA_IRQ(0,0)),
			qdev_get_gpio_in(intc,EDMA_IRQ(0,1)),
			qdev_get_gpio_in(intc,EDMA_IRQ(0,2)),NULL);

	qdev_set_parent_bus(edma, sysbus_get_default());

	object_property_set_bool(OBJECT(edma), true, "realized", &err);
    if (err) {
    	error_propagate(errp, err);
    	return;
    }
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


#if 0
	/*
	 * intc
	 */
	Am335xIntcState *intc = &soc->intc;
	object_property_set_uint(OBJECT(intc), INTC_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(intc), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}
	sysbus_connect_irq(SYS_BUS_DEVICE(intc), 0,
			   qdev_get_gpio_in(DEVICE(&soc->cpu), ARM_CPU_IRQ));
	sysbus_connect_irq(SYS_BUS_DEVICE(intc), 1,
			   qdev_get_gpio_in(DEVICE(&soc->cpu), ARM_CPU_FIQ));

	/*
	 * prcm
	 */
	object_property_set_uint(OBJECT(&soc->prcm), PRCM_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(&soc->prcm), true, "realized", &err);

	/*
	 * cm
	 */
	object_property_set_uint(OBJECT(&soc->cm), CM_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(&soc->cm), true, "realized", &err);

	/*
	 * edma
	 */
	Am335xEdmaState *edma = &soc->edma;
	object_property_set_uint(OBJECT(edma), EDMA_ADDRESS, "addr", &err);
	object_property_set_bool(OBJECT(edma), true, "realized", &err);
	if (err) {
		error_propagate(errp, err);
		return;
	}

	sysbus_connect_irq(SYS_BUS_DEVICE(edma), 0,
			   qdev_get_gpio_in(DEVICE(intc), EDMA_IRQ(0,0)));
	sysbus_connect_irq(SYS_BUS_DEVICE(edma), 1,
			   qdev_get_gpio_in(DEVICE(intc), EDMA_IRQ(0,1)));
	sysbus_connect_irq(SYS_BUS_DEVICE(edma), 2,
			   qdev_get_gpio_in(DEVICE(intc), EDMA_IRQ(0,2)));

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
