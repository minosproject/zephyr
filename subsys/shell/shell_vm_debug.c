/*
 * Minos VM debug console shell driver
 *
 * Copyright (c) 2019 Le Min
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <shell/shell.h>
#include <minos/minos.h>

#define BUF_TX_SIZE	4096
#define BUF_RX_SIZE	2048

#define LOG_MODULE_NAME	shell_vm
LOG_MODULE_REGISTER(shell_vm);

struct shell_vm {
	uint32_t irq;
	struct vm_ring *tx;
	struct vm_ring *rx;
	void *context;
	shell_transport_handler_t evt_handler;
};

static void vm_shell_isr(void *data)
{
	struct shell_vm *sh = data;

	if (sh->evt_handler)
		sh->evt_handler(SHELL_TRANSPORT_EVT_RX_RDY, sh->context);
}

static int vm_shell_init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	/*
	 * register the irq handler for read operation
	 */
	struct shell_vm *sh = transport->ctx;

	sh->evt_handler = evt_handler;
	sh->context = context;

	if (irq_connect_dynamic(sh->irq, 0, vm_shell_isr, sh, 0))
		return -EIO;

	irq_enable(sh->irq);
	minos_hvc0(HVC_DC_OPEN);

	return 0;
}

static int vm_shell_write(const struct shell_transport *transport,
		const void *data, size_t length, size_t *cnt)
{
	int send = 0, len = length;
	struct shell_vm *sh = transport->ctx;
	struct vm_ring *tx = sh->tx;
	uint32_t ridx, widx;
	u8_t *data8 = (u8_t *)data;

	while (length > 0) {
		ridx = tx->ridx;
		widx = tx->widx;
		vm_mb();

		if ((widx - ridx) >= tx->size)
			k_fatal_halt(0);

		while ((send < length) && (widx - ridx) < tx->size)
			tx->buf[VM_RING_IDX(widx++, tx->size)] = data8[send++];

		tx->widx = widx;
		vm_mb();

		length -= send;
		data8 += send;

		minos_hvc0(HVC_DC_WRITE);
		send = 0;
	}

	*cnt = len;

	return 0;
}

static int vm_shell_read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_vm *sh = transport->ctx;
	struct vm_ring *rx = sh->rx;
	uint32_t ridx, widx;
	u8_t *buf = (u8_t *)data;
	int recv = 0;

	ridx = rx->ridx;
	widx = rx->widx;
	vm_mb();

	if (widx - ridx >= rx->size)
		k_fatal_halt(0);

	while ((ridx != widx) && (recv < length))
		buf[recv++] = rx->buf[VM_RING_IDX(ridx++, rx->size)];

	rx->ridx = ridx;
	vm_mb();

	*cnt = recv;

	return 0;
}

static int vm_shell_uninit(const struct shell_transport *transport)
{
	struct shell_vm *sh = transport->ctx;

	irq_disable(sh->irq);
	minos_hvc0(HVC_DC_CLOSE);

	return 0;
}

static int vm_shell_enable(const struct shell_transport *transport, bool on)
{
	return 0;
}

const struct shell_transport_api vm_shell_api = {
	.init = vm_shell_init,
	.uninit = vm_shell_uninit,
	.enable = vm_shell_enable,
	.write = vm_shell_write,
	.read = vm_shell_read,
};

#define SHELL_VM_DEFINE(_name)	\
	static struct shell_vm _name##_shell_vm;		\
	struct shell_transport _name = {			\
		.api = &vm_shell_api,				\
		.ctx = (struct shell_vm *)&_name##_shell_vm,	\
	}

SHELL_VM_DEFINE(shell_transport_vm);
SHELL_DEFINE(shell_vm, CONFIG_SHELL_PROMPT_VM, &shell_transport_vm,
		CONFIG_SHELL_VM_LOG_MESSAGE_QUEUE_SIZE,
		CONFIG_SHELL_VM_LOG_MESSAGE_QUEUE_TIMEOUT,
		SHELL_FLAG_OLF_CRLF);

static int enable_shell_debug_console(struct device *arg)
{
	ARG_UNUSED(arg);
	bool log_backend = CONFIG_SHELL_VM_LOG_LEVEL > 0;
	u32_t level =
		(CONFIG_SHELL_VM_LOG_LEVEL > LOG_LEVEL_DBG) ?
		CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_VM_LOG_LEVEL;
	uint32_t id;
	struct shell_vm *sh_vm = &shell_transport_vm_shell_vm;
	unsigned long base;

	id = minos_hvc0(HVC_DC_GET_STAT);
	if ((id & 0xffff0000) != 0xabcd0000)
		return -ENOENT;

	sh_vm->irq = minos_hvc0(HVC_DC_GET_IRQ);
	base = minos_hvc0(HVC_DC_GET_RING);

	sh_vm->rx = (struct vm_ring *)base;
	sh_vm->tx = (struct vm_ring *)(base +
		sizeof(struct vm_ring) + BUF_RX_SIZE);

	return shell_init(&shell_vm, NULL, true, log_backend, level);
}
SYS_INIT(enable_shell_debug_console, POST_KERNEL, 0);
