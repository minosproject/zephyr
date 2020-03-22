/*
 * Minos vmbox bus driver
 *
 * Copyright (c) 2019 Le Min
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <device.h>
#include <soc.h>
#include <minos/vmbox.h>

#define VMBOX_CON_BASE	DT_INST_0_MINOS_VMBOX_CONTROLLER_BASE_ADDRESS

#define VMBOX_CON_REG()	\
	((volatile struct vmbox_con_reg *)(VMBOX_CON_BASE))

#define VMBOX_DEV_REG(id) \
	((volatile struct vmbox_dev_reg *)((unsigned long)VMBOX_CON_BASE + \
		VMBOX_CON_DEV_BASE + id * VMBOX_CON_DEV_SIZE))

static int vmbox_init;
static u32_t dev_stat;

static void vmbox_con_init(void)
{
	/* tell the hypervisor we are online */
	VMBOX_CON_REG()->con_online = 1;
	dev_stat = VMBOX_CON_REG()->con_dev_stat;
}

static void vmbox_ipc_isr(void *data)
{
	struct device *dev = data;
	struct vmbox_dev_data *info = VMBOX_DEV_DATA(dev);
	int event, bit;

	event = info->iomem->ipc_type;

	while (event != 0) {
		bit = find_lsb_set(event) - 1;
		if (bit == VMBOX_IPC_STATE_CHANGE) {
			if (info->otherside_state_change)
				info->otherside_state_change(dev);
		} else {
			if (info->event_handler)
				info->event_handler(dev, bit);
		}

		event &= ~(1 << bit);
	}
}

int vmbox_device_setup(struct vmbox_dev_data *info,
		void *data)
{
	if (info->ipc_irq) {
		if (irq_connect_dynamic(info->ipc_irq, 0,
					vmbox_ipc_isr, data, 0)) {
			printk("register ipc irq failed\n");
			info->ipc_irq = 0;
			return -EINVAL;
		}

		irq_enable(info->ipc_irq);
	}

	return 0;
}

static void __vmbox_get_device_info(struct vmbox_dev_data *info,
		volatile struct vmbox_dev_reg *dev_reg, u32_t did)
{
	info->iomem = dev_reg;
	info->mem_size = dev_reg->mem_size;
	info->vring_base = (unsigned long)dev_reg->base_low;
	info->nr_vqs = dev_reg->vqs;
	info->vring_num = dev_reg->vring_num;
	info->vring_size = dev_reg->vring_size;
	info->vring_irq = dev_reg->vring_irq;
	info->ipc_irq = dev_reg->ipc_irq;

	info->backend = !(did % 2);
	info->data_base = info->vring_base + VMBOX_IPC_ALL_ENTRY_SIZE;

	if (info->backend) {
		info->ipc_out = (struct vmbox_ipc_entry *)info->vring_base;
		info->ipc_in = (struct vmbox_ipc_entry*)(info->vring_base +
				VMBOX_IPC_PER_ENTRY_SZIE);
	} else {
		info->ipc_in = (struct vmbox_ipc_entry *)info->vring_base;
		info->ipc_out = (struct vmbox_ipc_entry*)(info->vring_base +
				VMBOX_IPC_PER_ENTRY_SZIE);
	}
}

int vmbox_get_device_info(struct vmbox_dev_data *info,
		u32_t devid, u32_t vendor_id)
{
	u32_t state;
	u8_t bit;
	volatile struct vmbox_dev_reg *dev_reg;

	if (!vmbox_init) {
		vmbox_con_init();
		vmbox_init = 1;
	}

	state = dev_stat;

	while (state != 0) {
		bit = find_lsb_set(state) - 1;
		dev_reg = VMBOX_DEV_REG(bit);

		/*
		 * once one device is detected, mask it device status
		 * to 0
		 */
		if ((devid == dev_reg->device_id) &&
				((vendor_id == VMBOX_ANY_VENDOR_ID) ||
				 (vendor_id == dev_reg->vendor_id))) {
			__vmbox_get_device_info(info, dev_reg, devid);
			dev_stat &= (1 << bit);

			return 0;
		}

		state &= ~(1 << bit);
	}

	return -ENODEV;
}
