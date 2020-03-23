/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/timer/arm_arch_timer.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <arch/cpu.h>

#define CYC_PER_TICK	((u32_t)((u64_t)sys_clock_hw_cycles_per_sec() \
			       / (u64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

#define MAX_TICKS	((0xffffffffu - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY	(1000)

static struct k_spinlock lock;
static volatile u64_t last_cycle;

static void arm_arch_timer_compare_isr(void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	u64_t curr_cycle = arm_arch_timer_count();
	u32_t delta_ticks = (u32_t)((curr_cycle - last_cycle) / CYC_PER_TICK);

	last_cycle += delta_ticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) ||
	     IS_ENABLED(CONFIG_QEMU_TICKLESS_WORKAROUND)) {
		u64_t next_cycle = last_cycle + CYC_PER_TICK;

		if ((s64_t)(next_cycle - curr_cycle) < MIN_DELAY) {
			next_cycle += CYC_PER_TICK;
		}
		arm_arch_timer_set_compare(next_cycle);
	}

	k_spin_unlock(&lock, key);

	z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks : 1);
}

int z_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	IRQ_CONNECT(ARM_ARCH_TIMER_IRQ, 0, arm_arch_timer_compare_isr, 0,
		    ARM_TIMER_FLAGS);
	arm_arch_timer_set_compare(arm_arch_timer_count() + CYC_PER_TICK);
	arm_arch_timer_toggle(true);
	irq_enable(ARM_ARCH_TIMER_IRQ);

	return 0;
}

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL) && !defined(CONFIG_QEMU_TICKLESS_WORKAROUND)

	if (idle) {
		return;
	}

	ticks = (ticks == K_FOREVER) ? MAX_TICKS : ticks;
	ticks = MAX(MIN(ticks - 1, (s32_t)MAX_TICKS), 0);

	/*
	 * change req_cycle from u32_t to u64_t when
	 * req_cycle += CYC_PER_TICK may overflow then
	 * it will cacuse timer irq storm
	 */
	k_spinlock_key_t key = k_spin_lock(&lock);
	u64_t curr_cycle = arm_arch_timer_count();
	u64_t req_cycle = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary */
	req_cycle += (u32_t)(curr_cycle - last_cycle) + (CYC_PER_TICK - 1);
	req_cycle = (req_cycle / CYC_PER_TICK) * CYC_PER_TICK;

	if ((s32_t)(req_cycle + last_cycle - curr_cycle) < MIN_DELAY) {
		req_cycle += CYC_PER_TICK;
	}

	arm_arch_timer_set_compare(req_cycle + last_cycle);
	arm_arch_timer_enable_event(1);
	k_spin_unlock(&lock, key);

#endif
}

u32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = ((u32_t)arm_arch_timer_count() - (u32_t)last_cycle)
		    / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

u32_t z_timer_cycle_get_32(void)
{
	return (u32_t)arm_arch_timer_count();
}
