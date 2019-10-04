/*
 * nand_flash.c
 *
 *  Created on: Feb 3, 2017
 *      Author: Zhengrong Liu<towering@126.com>
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "sysemu/block-backend.h"
#include "hw/qdev.h"
#include "hw/block/nand_flash.h"

#define NAND_CMD_READ 0x00
#define NAND_CMD_READ_EXC 0x30
#define NAND_CMD_READID 0x90
#define NAND_CMD_RESET 0xff
#define NAND_CMD_PROGRAM 0x80
#define NAND_CMD_PROGRAM_EXC 0x10
#define NAND_CMD_ERASE 0x60
#define NAND_CMD_ERASE_EXC 0xd0
#define NAND_CMD_READSTATUS 0x70
#define NAND_CMD_READEDC 0x7B
#define NAND_CMD_RANDOM_IN 0x85
#define NAND_CMD_RANDOM_OUT 0x05
#define NAND_CMD_RANDOM_OUT_EXC 0xe0

#define NAND_STATUS_PASS (0 << 0)
#define NAND_STATUS_FAIL (1 << 0)
#define NAND_STATUS_READY (1 << 6)
#define NAND_STATUS_BUSY (0 << 6)
#define NAND_STATUS_PROTECTED (0 << 7)
#define NAND_STATUS_UNPROTECTED (1 << 7)

enum nand_flash_status {
	st_standby = 0,
	st_read_addr,
	st_read_loaded,
	st_read_executed,
	st_read_random_addr,
	st_read_random_loaded,
	st_program_addr,
	st_program_loaded,
	st_program_data,
	st_program_executed,
	st_erase_addr,
	st_erase_loaded,
	st_erase_executed,
	st_readstatus_executed,
	st_readid_addr,
};

enum nand_flash_fact {
	fact_cmd_read,
	fact_cmd_read_exc,
	fact_cmd_read_random,
	fact_cmd_read_random_exc,
	fact_cmd_program,
	fact_cmd_program_exc,
	fact_cmd_program_random,
	fact_cmd_erase,
	fact_cmd_erase_exc,
	fact_cmd_readid,
	fact_cmd_reset,
	fact_cmd_readstatus,
	fact_addr,
	fact_addr_ready,
	fact_data,
	fact_data_end,
	fact_end,
};

enum nand_flash_op {
	op_read,
	op_page_program,
	op_erase,
};

static uint8_t state_machine[][fact_end] = {
	[st_standby] =
	    {
		    [fact_cmd_read] = st_read_addr,
		    [fact_cmd_program] = st_program_addr,
		    [fact_cmd_erase] = st_erase_addr,
		    [fact_cmd_readid] = st_readid_addr,
		    [fact_cmd_readstatus] = st_readstatus_executed,
	    },
	[st_read_addr] =
	    {
		    [fact_addr] = st_read_addr,
		    [fact_addr_ready] = st_read_loaded,
	    },
	[st_read_loaded] =
	    {
		    [fact_cmd_read_exc] = st_read_executed,
	    },
	[st_read_executed] =
	    {
		    [fact_cmd_read_random] = st_read_random_addr,
		    [fact_data] = st_read_executed,
		    [fact_cmd_read] = st_read_addr,
		    [fact_cmd_program] = st_program_addr,
		    [fact_cmd_erase] = st_erase_addr,
		    [fact_cmd_readid] = st_readid_addr,
		    [fact_cmd_readstatus] = st_readstatus_executed,
	    },
	[st_read_random_addr] =
	    {
		    [fact_addr] = st_read_random_addr,
		    [fact_addr_ready] = st_read_random_loaded,
	    },
	[st_read_random_loaded] =
	    {
		    [fact_cmd_read_random_exc] = st_read_executed,
	    },
	[st_program_addr] =
	    {
		    [fact_addr] = st_program_addr,
		    [fact_addr_ready] = st_program_loaded,
	    },
	[st_program_loaded] =
	    {
		    [fact_data] = st_program_data,
	    },
	[st_program_data] =
	    {
		    [fact_data] = st_program_data,
		    [fact_cmd_program_exc] = st_program_executed,
		    [fact_cmd_program_random] = st_program_addr,
	    },
	[st_program_executed] =
	    {
		    [fact_cmd_read] = st_read_addr,
		    [fact_cmd_program] = st_program_addr,
		    [fact_cmd_erase] = st_erase_addr,
		    [fact_cmd_readid] = st_readid_addr,
		    [fact_cmd_readstatus] = st_readstatus_executed,
	    },
	[st_erase_addr] =
	    {
		    [fact_addr] = st_erase_addr,
		    [fact_addr_ready] = st_erase_loaded,
	    },
	[st_erase_loaded] =
	    {
		    [fact_cmd_erase_exc] = st_erase_executed,
	    },
	[st_erase_executed] =
	    {
		    [fact_cmd_read] = st_read_addr,
		    [fact_cmd_program] = st_program_addr,
		    [fact_cmd_erase] = st_erase_addr,
		    [fact_cmd_readid] = st_readid_addr,
		    [fact_cmd_readstatus] = st_readstatus_executed,
	    },
	[st_readid_addr] =
	    {
		    [fact_addr_ready] = st_read_executed,
	    },
	[st_readstatus_executed] =
	    {
		    [fact_cmd_read] = st_read_addr,
		    [fact_cmd_program] = st_program_addr,
		    [fact_cmd_erase] = st_erase_addr,
		    [fact_cmd_readid] = st_readid_addr,
		    [fact_cmd_readstatus] = st_readstatus_executed,
	    },
};

static void state_update(NandFlashState *nand, enum nand_flash_fact fact)
{
	int64_t offset;
	int i = 0;

	nand->status = state_machine[nand->status][fact];

	switch (nand->status) {
	case st_standby:
		nand->caddr_index = nand->caddr_bytes;
		nand->raddr_index = nand->raddr_bytes;
		nand->caddr = 0;
		nand->raddr = 0;
		break;
	case st_program_loaded:
	case st_read_loaded:
		offset = nand->raddr * nand->page_oob_size;
		blk_pread(nand->blk, offset, nand->data_buf,
			  nand->page_oob_size);
		break;
	case st_program_executed:
		offset = nand->raddr * nand->page_oob_size;
		blk_pwrite(nand->blk, offset, nand->data_buf,
			   nand->page_oob_size, 0);
		break;
	case st_erase_executed:
		offset = nand->raddr / nand->pages_per_block;
		offset *= nand->pages_per_block * nand->page_oob_size;
		for (i = 0; i < nand->pages_per_block; i++) {
			blk_pread(nand->blk, offset, nand->data_buf,
				  nand->page_oob_size);
			memset(nand->data_buf, 0xff, nand->page_oob_size);
			blk_pwrite(nand->blk, offset, nand->data_buf,
				   nand->page_oob_size, 0);
			offset += nand->page_oob_size;
		}
		break;
	case st_readid_addr:
		nand->caddr = nand->page_oob_size - 5;
		nand->data_buf[nand->caddr + 0] = nand->manf_id;
		nand->data_buf[nand->caddr + 1] = nand->chip_id;
		nand->data_buf[nand->caddr + 2] = 0x10;
		nand->data_buf[nand->caddr + 3] = 0x95;
		nand->data_buf[nand->caddr + 4] = 0x44;
		break;
	case st_readstatus_executed:
		nand->caddr = nand->page_oob_size - 1;
		nand->data_buf[nand->caddr + 0] = nand->reg_status;
		break;
	}
}

void nand_flash_set_command(NandFlashState *nand, uint8_t cmd)
{
	enum nand_flash_fact fact = fact_cmd_reset;

	switch (cmd) {
	case NAND_CMD_READ:
		nand->caddr_index = 0;
		nand->raddr_index = 0;
		nand->caddr = 0;
		nand->raddr = 0;

		fact = fact_cmd_read;
		break;
	case NAND_CMD_READ_EXC:
		fact = fact_cmd_read_exc;
		break;
	case NAND_CMD_RANDOM_OUT:
		nand->caddr_index = 0;
		nand->caddr = 0;
		fact = fact_cmd_read_random;
		break;
	case NAND_CMD_RANDOM_OUT_EXC:
		fact = fact_cmd_read_random_exc;
		break;
	case NAND_CMD_PROGRAM:
		nand->caddr_index = 0;
		nand->raddr_index = 0;
		nand->caddr = 0;
		nand->raddr = 0;

		fact = fact_cmd_program;
		break;
	case NAND_CMD_RANDOM_IN:
		nand->caddr_index = 0;
		nand->caddr = 0;
		fact = fact_cmd_program_random;
		break;
	case NAND_CMD_PROGRAM_EXC:
		fact = fact_cmd_program_exc;
		break;
	case NAND_CMD_ERASE:
		nand->caddr_index = nand->caddr_bytes;
		nand->raddr_index = 0;
		nand->raddr = 0;

		fact = fact_cmd_erase;
		break;
	case NAND_CMD_ERASE_EXC:
		fact = fact_cmd_erase_exc;
		break;
	case NAND_CMD_READID:
		fact = fact_cmd_readid;
		break;
	case NAND_CMD_RESET:
		fact = fact_cmd_reset;
		break;
	case NAND_CMD_READSTATUS:
		fact = fact_cmd_readstatus;
		break;
	default:
		fprintf(stderr, "nand flash unsupported command %x\n", cmd);
		break;
	}

	state_update(nand, fact);
}

void nand_flash_set_address(NandFlashState *nand, uint8_t addr)
{
	enum nand_flash_fact fact = fact_addr;

	switch (nand->status) {
	case st_program_addr:
	case st_erase_addr:
	case st_read_addr:
	case st_read_random_addr:
		if (nand->caddr_index < nand->caddr_bytes) {
			nand->caddr |= addr << (nand->caddr_index * 8);
			nand->caddr_index++;
		} else if (nand->raddr_index < nand->raddr_bytes) {
			nand->raddr |= addr << (nand->raddr_index * 8);
			nand->raddr_index++;
		}
		if (nand->caddr_index == nand->caddr_bytes &&
		    nand->raddr_index == nand->raddr_bytes) {
			fact = fact_addr_ready;
		}
		break;
	case st_readid_addr:
		if (addr == 0x00) {
			fact = fact_addr_ready;
		}
		break;
	}

	state_update(nand, fact);
}

void nand_flash_set_data(NandFlashState *nand, uint8_t data)
{

	switch (nand->status) {
	case st_program_loaded:
	case st_program_data:
		nand->data_buf[nand->caddr++] &= data;
		break;
	}
	state_update(nand, fact_data);
}

int nand_flash_get_data(NandFlashState *nand, uint8_t *pdata)
{
	int ret = 0;
	enum nand_flash_fact fact = fact_data;

	switch (nand->status) {
	case st_readstatus_executed:
	case st_read_executed:
		if (nand->caddr < nand->page_oob_size) {
			*pdata = nand->data_buf[nand->caddr++];
		} else {
			*pdata = 0x00;
			fact = fact_data_end;
		}
		break;
	default:
		ret = -1;
		break;
	}

	state_update(nand, fact);
	return ret;
}

static void nand_flash_init(Object *obj)
{
	NandFlashState *nand = NAND_FLASH(obj);
	nand->status = st_standby;

	nand->reg_status =
	    NAND_STATUS_PASS | NAND_STATUS_READY | NAND_STATUS_UNPROTECTED;

	nand->caddr_bytes = 2;
	nand->raddr_bytes = 3;
	nand->caddr_index = nand->caddr_bytes;
	nand->raddr_index = nand->raddr_bytes;

	nand->page_oob_size = 2048 + 64;
	nand->pages_per_block = 64;

	nand->data_buf = g_malloc(nand->page_oob_size);
}

static void nand_flash_realize(DeviceState *dev_soc, Error **errp)
{
	/*
	    NandFlashState *nand = NAND_FLASH(dev_soc);

	    memset(nand->data_buf,0x0,nand->page_oob_size);
	    blk_pwrite(nand->blk,0x0,nand->data_buf,nand->page_oob_size,0);
	*/
}

static void nand_flash_reset(DeviceState *dev) {}

static Property nand_flash_properties[] = {
    DEFINE_PROP_UINT8("manufacturer_id", NandFlashState, manf_id, 0),
    DEFINE_PROP_UINT8("chip_id", NandFlashState, chip_id, 0),
    DEFINE_PROP_DRIVE("drive", NandFlashState, blk), DEFINE_PROP_END_OF_LIST(),
};

static void nand_flash_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = nand_flash_realize;
	dc->reset = nand_flash_reset;

	dc->props = nand_flash_properties;
}

static const TypeInfo nand_flash_info = {
    .name = TYPE_NAND_FLASH,
    .parent = TYPE_DEVICE,
    .instance_init = nand_flash_init,
    .instance_size = sizeof(NandFlashState),
    .class_init = nand_flash_class_init,
};

static void nand_flash_register_types(void)
{
	type_register_static(&nand_flash_info);
}

type_init(nand_flash_register_types)
