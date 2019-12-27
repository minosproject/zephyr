/*
 * Minos vmbox bus driver
 *
 * Copyright (c) 2019 Le Min
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_VMBOX_H__
#define __ZEPHYR_VMBOX_H__

#include <kernel.h>
#include <device.h>
#include <sys/sys_io.h>

#define VMBOX_ANY_DEV_ID	0xffff
#define VMBOX_ANY_VENDOR_ID	0xffff

enum {
	VMBOX_DEV_STAT_OFFLINE = 0,
	VMBOX_DEV_STAT_ONLINE,
	VMBOX_DEV_STAT_OPENED,
	VMBOX_DEV_STAT_CLOSED,
};

/*
 * below are the defination of the vmbox controller
 * and device register map, each controller will have
 * a 4K IO memory space, 0x0-0xff is for controller
 * itself, and 0x100 - 0xfff is for the vmbox devices
 */
#define VMBOX_DEVICE_MAGIC		0xabcdef00

#define VMBOX_CON_DEV_STAT		0x00	/* RO state of each device */
#define VMBOX_CON_ONLINE		0x04	/* WO to inform the controller is online */
#define VMBOX_CON_INT_STATUS		0x08	/* RO virq will send by hypervisor */

#define VMBOX_CON_INT_TYPE_DEV_ONLINE	(1 << 0)

#define VMBOX_CON_DEV_BASE 		0x100
#define VMBOX_CON_DEV_SIZE 		0x40

#define VMBOX_DEV_ID			0x00	/* RO */
#define VMBOX_DEV_VQS			0x04	/* RO */
#define VMBOX_DEV_VRING_NUM 		0X08	/* RO */
#define VMBOX_DEV_VRING_SIZE 		0x0c	/* RO */
#define VMBOX_DEV_VRING_BASE_HI		0x10	/* RO */
#define VMBOX_DEV_VRING_BASE_LOW	0x14	/* RO */
#define VMBOX_DEV_MEM_SIZE		0x18
#define VMBOX_DEV_DEVICE_ID		0x1c	/* RO */
#define VMBOX_DEV_VENDOR_ID		0x20	/* RO */
#define VMBOX_DEV_VRING_IRQ		0x24	/* RO */
#define VMBOX_DEV_IPC_IRQ		0x28	/* RO */
#define VMBOX_DEV_VRING_EVENT		0x2c	/* WO trigger a vring event */
#define VMBOX_DEV_IPC_EVENT		0x30	/* WO trigger a config event */
#define VMBOX_DEV_VDEV_ONLINE		0x34	/* only for client device */
#define VMBOX_DEV_IPC_TYPE		0x38	/* the event type, read and clear */

#define VMBOX_DEV_IPC_COUNT		32

#define VMBOX_IPC_STATE_CHANGE		0x0
#define VMBOX_IPC_USER_BASE		0x8
#define VMBOX_IPC_USER_EVENT(n)		(n + VMBOX_IPC_USER_BASE)

#define VMBOX_IPC_PER_ENTRY_SZIE	0x80
#define VMBOX_IPC_ALL_ENTRY_SIZE	0x100		// 0x80 * 2

#define VMBOX_F_NO_VIRTQ		(1 << 0)
#define VMBOX_F_DEV_BACKEND		(1 << 1)
#define VMBOX_F_VRING_IRQ_MANUAL	(1 << 2)

struct vmbox_con_reg {
	u32_t con_dev_stat;
	u32_t con_online;
	u32_t con_int_stat;
};

struct vmbox_dev_reg {
	u32_t id;
	u32_t vqs;
	u32_t vring_num;
	u32_t vring_size;
	u32_t base_hi;
	u32_t base_low;
	u32_t mem_size;
	u32_t device_id;
	u32_t vendor_id;
	u32_t vring_irq;
	u32_t ipc_irq;
	u32_t vring_event;
	u32_t ipc_event;
	u32_t vdev_online;
	u32_t ipc_type;
};

struct vmbox_ipc_entry {
	volatile u32_t state;
	char buf[0];
};

struct vmbox_dev_data {
	volatile struct vmbox_dev_reg *iomem;
	unsigned long vring_base;
	unsigned long data_base;
	int backend;
	int class_id;
	u32_t mem_size;
	u32_t nr_vqs;
	u32_t vring_num;
	u32_t vring_size;
	u32_t vring_irq;
	u32_t ipc_irq;
	struct vmbox_ipc_entry *ipc_out;
	struct vmbox_ipc_entry *ipc_in;
	int (*otherside_state_change)(struct device *dev);
	int (*event_handler)(struct device *dev, int event);
};

#define VMBOX_DEV_DATA(dev) \
	(struct vmbox_dev_data *)dev->driver_data;

static inline void vmbox_mb(void)
{
	__ISB();
}

static inline void vmbox_wmb(void)
{
	__DMB();
}

static inline int
vmbox_device_ipc_event(struct vmbox_dev_data *info, int event)
{
	if (event >= VMBOX_DEV_IPC_COUNT)
		return -EINVAL;

	info->iomem->ipc_event = event;
	return 0;
}

static inline int
vmbox_device_vring_event(struct vmbox_dev_data *info)
{
	if (info->ipc_in->state != VMBOX_DEV_STAT_OPENED)
		return -EINVAL;

	info->iomem->vring_event = 1;
	return 0;
}

static inline void
vmbox_device_state_change(struct vmbox_dev_data *info, u32_t state)
{
	info->ipc_out->state = state;
	vmbox_device_ipc_event(info, VMBOX_IPC_STATE_CHANGE);
}

static inline void
vmbox_device_online(struct vmbox_dev_data *info)
{
	info->ipc_out->state = VMBOX_DEV_STAT_ONLINE;
	info->iomem->vdev_online = 1;
}

int vmbox_device_setup(struct vmbox_dev_data *info,
		void *data);
int vmbox_get_device_info(struct vmbox_dev_data *info,
		u32_t devid, u32_t vendor_id);

#endif
