/*
 * Minos vmbox console shell driver
 *
 * Copyright (c) 2019 Le Min
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <shell/shell.h>
#include <minos/vmbox.h>

#define LOG_MODULE_NAME shell_vmbox
LOG_MODULE_REGISTER(shell_vmbox);

#define VMBOX_CONSOLE_ID	0x3420

#define BUF_0_SIZE	4096
#define BUF_1_SIZE	2048

struct shell_vmbox {
	struct device *dev;
	struct vm_ring *tx;
	struct vm_ring *rx;
	void *context;
	shell_transport_handler_t evt_handler;
};

static void vmbox_vc_vring_isr(void *data)
{
	struct shell_vmbox *sv = (struct shell_vmbox *)data;

	if (sv->evt_handler)
		sv->evt_handler(SHELL_TRANSPORT_EVT_RX_RDY, sv->context);
}

static int vmbox_shell_init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_vmbox *sh_vmbox = (struct shell_vmbox *)transport->ctx;
	struct vmbox_dev_data *dd;
	unsigned long base;

	sh_vmbox->dev = (struct device *)config;
	sh_vmbox->context = context;
	sh_vmbox->evt_handler = evt_handler;
	dd = VMBOX_DEV_DATA(sh_vmbox->dev);
	base = (unsigned long)dd->data_base;

	/* init the vm_ring tx and rx, use interrupt to handle
	 * the read and write */
	if (dd->backend) {
		sh_vmbox->tx = (struct vm_ring *)base;
		sh_vmbox->rx = (struct vm_ring *)(base +
				sizeof(struct vm_ring) + BUF_0_SIZE);
	} else {
		sh_vmbox->rx = (struct vm_ring *)base;
		sh_vmbox->tx = (struct vm_ring *)(base +
				sizeof(struct vm_ring) + BUF_0_SIZE);
	}

	if (irq_connect_dynamic(dd->vring_irq, 0,
				vmbox_vc_vring_isr, sh_vmbox, 0))
		return -EIO;

	irq_enable(dd->vring_irq);
	vmbox_device_state_change(dd, VMBOX_DEV_STAT_OPENED);

	return 0;
}

static int vmbox_shell_write(const struct shell_transport *transport,
		const void *data, size_t length, size_t *cnt)
{
	int len = length, send = 0;
	u32_t ridx, widx;
	struct shell_vmbox *sh_vmbox = (struct shell_vmbox *)transport->ctx;
	const u8_t *data8 = (const u8_t *)data;
	struct vm_ring *ring = sh_vmbox->tx;
	struct vmbox_dev_data *dd = VMBOX_DEV_DATA(sh_vmbox->dev);

	while (length) {
again:
		ridx = ring->ridx;
		widx = ring->widx;
		vm_mb();

		if ((widx - ridx) == ring->size) {
			if (dd->ipc_in->state != VMBOX_DEV_STAT_OPENED) {
				ridx += length;
				ring->ridx = ridx;
				vm_wmb();
			} else {
				vmbox_device_vring_event(dd);
				goto again;
			}
		}

		while ((send < length) && ((widx - ridx) < ring->size)) {
			ring->buf[VM_RING_IDX(widx++, ring->size)] =
				data8[send++];
		}

		ring->widx = widx;
		vm_mb();

		length -= send;
		data8 += send;

		if (send && dd->ipc_in->state == VMBOX_DEV_STAT_OPENED)
			vmbox_device_vring_event(dd);

		send = 0;
	}

	*cnt = len;

	return 0;
}

static int vmbox_shell_read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_vmbox *sh_vmbox = (struct shell_vmbox *)transport->ctx;
	struct vm_ring *ring = sh_vmbox->rx;
	u32_t ridx, widx, recv = 0;
	u8_t *buf = (u8_t *)data;

	ridx = ring->ridx;
	widx = ring->widx;
	vm_mb();

	if ((widx - ridx) > ring->size) {
		printk("vmbox_shell_read overflow happend\n");
		k_fatal_halt(0);
	}

	while ((ridx != widx) && (recv < length))
		buf[recv++] = ring->buf[VM_RING_IDX(ridx++, ring->size)];

	ring->ridx = ridx;
	vm_mb();

	*cnt = recv;

	return 0;
}

static int vmbox_shell_uinit(const struct shell_transport *transport)
{
	return 0;
}

static int vmbox_shell_enable(const struct shell_transport *transport, bool bt)
{
	return 0;
}

const struct shell_transport_api vmbox_console_transport_api = {
	.init = vmbox_shell_init,
	.uninit = vmbox_shell_uinit,
	.enable = vmbox_shell_enable,
	.write = vmbox_shell_write,
	.read = vmbox_shell_read,
};

#define SHELL_VMBOX_DEFINE(_name)					\
	static struct shell_vmbox _name##_shell_vmbox;			\
	struct shell_transport _name = {				\
		.api = &vmbox_console_transport_api,			\
		.ctx = (struct shell_vmbox *)&_name##_shell_vmbox, 	\
	}

SHELL_VMBOX_DEFINE(shell_transport_vmbox);
SHELL_DEFINE(shell_vmbox, CONFIG_SHELL_PROMPT_VMBOX, &shell_transport_vmbox,
		CONFIG_SHELL_VMBOX_LOG_MESSAGE_QUEUE_SIZE,
		CONFIG_SHELL_VMBOX_LOG_MESSAGE_QUEUE_TIMEOUT,
		SHELL_FLAG_OLF_CRLF);

#define VMBOX_CONSOLE_NAME	"vmbox_console_0"

static int vmbox_console_init(struct device *dev)
{
	int ret;
	struct vmbox_dev_data *dev_data = VMBOX_DEV_DATA(dev);

	ret = vmbox_get_device_info(dev_data,
			VMBOX_CONSOLE_ID, VMBOX_ANY_VENDOR_ID);
	if (ret)
		return ret;

	vmbox_device_setup(dev_data, dev);
	vmbox_device_online(dev_data);

	return 0;
}

static int vc_otherside_state_change(struct device *dev)
{
	return 0;
}

static int vc_event_handler(struct device *dev, int event)
{
	return 0;
}

static struct vmbox_dev_data vc_dev_data0 = {
	.class_id = 0,
	.otherside_state_change = vc_otherside_state_change,
	.event_handler = vc_event_handler,
};

DEVICE_AND_API_INIT(vmbox_console_0,
		VMBOX_CONSOLE_NAME,
		&vmbox_console_init,
		&vc_dev_data0,
		NULL, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);

static int enable_shell_vmbox_console(struct device *arg)
{
	ARG_UNUSED(arg);
	struct device *dev = &__device_vmbox_console_0;
	bool log_backend = CONFIG_SHELL_VMBOX_LOG_LEVEL > 0;
	u32_t level =
		(CONFIG_SHELL_VMBOX_LOG_LEVEL > LOG_LEVEL_DBG) ?
		CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_VMBOX_LOG_LEVEL;

	if (!dev)
		return -ENODEV;

	return shell_init(&shell_vmbox, dev, true, log_backend, level);
}
SYS_INIT(enable_shell_vmbox_console, POST_KERNEL, 0);
