/*
 * tlv320aic31xx.c
 *
 *  Created on: Apr 3, 2018
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/i2c/i2c.h"

#define TYPE_TLV320AIC "tlv320aic31xx"
#define TLV320AIC31XX(obj)                                                     \
	OBJECT_CHECK(Tlv320aicState, (obj), TYPE_TLV320AIC31XX)

typedef struct Tlv320aicState {
	I2CSlave parent_obj;

}Tlv320aicState;

static void tlv320aic_class_init(ObjectClass *c, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(c);

}

static const TypeInfo tlv320aic_type_info = {
	.name = TYPE_TLV320AIC,
	.parent = TYPE_I2C_SLAVE,
	.instance_size = sizeof(Tlv320aicState),
	.class_init = tlv320aic_class_init,
};

static void tlv320aic_register_types(void)
{
	type_register_static(&tlv320aic_type_info);
}

type_init(tlv320aic_register_types)

