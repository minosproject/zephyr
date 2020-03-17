#ifndef __ZEPHYR_MINOS_H__
#define __ZEPHYR_MINOS_H__

#include <zephyr/types.h>

/* below defination is for HVC call */
#define HVC_TYPE_VM0			(0x8)
#define HVC_TYPE_MISC			(0x9)
#define HVC_TYPE_VMBOX			(0xa)
#define HVC_TYPE_DEBUG_CONSOLE		(0xb)

#define HVC_CALL_BASE			(0xc0000000)

#define HVC_CALL_NUM(t, n)		(HVC_CALL_BASE + (t << 24) + n)

#define HVC_VM0_FN(n)			HVC_CALL_NUM(HVC_TYPE_VM0, n)
#define HVC_MISC_FN(n)			HVC_CALL_NUM(HVC_TYPE_MISC, n)
#define HVC_VMBOX_FN(n)			HVC_CALL_NUM(HVC_TYPE_VMBOX, n)
#define HVC_VMBOX_DEBUG_CONSOLE(n)	HVC_CALL_NUM(HVC_TYPE_DEBUG_CONSOLE, n)

/* hypercall for vm releated operation */
#define	HVC_VM_CREATE			HVC_VM0_FN(0)
#define HVC_VM_DESTORY			HVC_VM0_FN(1)
#define HVC_VM_RESTART			HVC_VM0_FN(2)
#define HVC_VM_POWER_UP			HVC_VM0_FN(3)
#define HVC_VM_POWER_DOWN		HVC_VM0_FN(4)
#define HVC_VM_MMAP			HVC_VM0_FN(5)
#define HVC_VM_UNMMAP			HVC_VM0_FN(6)
#define HVC_VM_SEND_VIRQ		HVC_VM0_FN(7)
#define HVC_VM_CREATE_VMCS		HVC_VM0_FN(8)
#define HVC_VM_CREATE_VMCS_IRQ		HVC_VM0_FN(9)
#define HVC_VM_REQUEST_VIRQ		HVC_VM0_FN(10)
#define HVC_VM_VIRTIO_MMIO_INIT		HVC_VM0_FN(11)
#define HVC_VM_VIRTIO_MMIO_DEINIT	HVC_VM0_FN(12)
#define HVC_VM_CREATE_RESOURCE		HVC_VM0_FN(13)
#define HVC_CHANGE_LOG_LEVEL		HVC_VM0_FN(14)

#define HVC_GET_VMID			HVC_MISC_FN(0)
#define HVC_SCHED_OUT			HVC_MISC_FN(1)

#define HVC_DC_GET_STAT			HVC_VMBOX_DEBUG_CONSOLE(0)
#define HVC_DC_GET_RING			HVC_VMBOX_DEBUG_CONSOLE(1)
#define HVC_DC_GET_IRQ			HVC_VMBOX_DEBUG_CONSOLE(2)
#define HVC_DC_WRITE			HVC_VMBOX_DEBUG_CONSOLE(3)
#define HVC_DC_OPEN			HVC_VMBOX_DEBUG_CONSOLE(4)
#define HVC_DC_CLOSE			HVC_VMBOX_DEBUG_CONSOLE(5)

struct arm_smccc_res {
        unsigned long a0;
        unsigned long a1;
        unsigned long a2;
        unsigned long a3;
};

struct arm_smccc_quirk {
	int	id;
	union {
		unsigned long a6;
	} state;
};

/**
 * __arm_smccc_smc() - make SMC calls
 * @a0-a7: arguments passed in registers 0 to 7
 * @res: result values from registers 0 to 3
 * @quirk: points to an arm_smccc_quirk, or NULL when no quirks are required.
 *
 * This function is used to make SMC calls following SMC Calling Convention.
 * The content of the supplied param are copied to registers 0 to 7 prior
 * to the SMC instruction. The return values are updated with the content
 * from register 0 to 3 on return from the SMC instruction.  An optional
 * quirk structure provides vendor specific behavior.
 */
void __arm_smccc_smc(unsigned long a0, unsigned long a1,
			unsigned long a2, unsigned long a3, unsigned long a4,
			unsigned long a5, unsigned long a6, unsigned long a7,
			struct arm_smccc_res *res, struct arm_smccc_quirk *quirk);

/**
 * __arm_smccc_hvc() - make HVC calls
 * @a0-a7: arguments passed in registers 0 to 7
 * @res: result values from registers 0 to 3
 * @quirk: points to an arm_smccc_quirk, or NULL when no quirks are required.
 *
 * This function is used to make HVC calls following SMC Calling
 * Convention.  The content of the supplied param are copied to registers 0
 * to 7 prior to the HVC instruction. The return values are updated with
 * the content from register 0 to 3 on return from the HVC instruction.  An
 * optional quirk structure provides vendor specific behavior.
 */
void __arm_smccc_hvc(unsigned long a0, unsigned long a1,
			unsigned long a2, unsigned long a3, unsigned long a4,
			unsigned long a5, unsigned long a6, unsigned long a7,
			struct arm_smccc_res *res, struct arm_smccc_quirk *quirk);

#define arm_smccc_smc(...) __arm_smccc_smc(__VA_ARGS__, NULL)
#define arm_smccc_smc_quirk(...) __arm_smccc_smc(__VA_ARGS__)
#define arm_smccc_hvc(...) __arm_smccc_hvc(__VA_ARGS__, NULL)
#define arm_smccc_hvc_quirk(...) __arm_smccc_hvc(__VA_ARGS__)

static inline unsigned long __minos_hvc(uint32_t id, unsigned long a0,
       unsigned long a1, unsigned long a2, unsigned long a3,
       unsigned long a4, unsigned long a5, unsigned long a6)
{
   struct arm_smccc_res res;

   arm_smccc_hvc(id, a0, a1, a2, a3, a4, a5, a6, &res);
   return res.a0;
}

#define minos_hvc(id, a, b, c, d, e, f, g) \
	__minos_hvc(id, (unsigned long)(a), (unsigned long)(b), \
		    (unsigned long)(c), (unsigned long)(d), \
		    (unsigned long)(e), (unsigned long)(f), \
		    (unsigned long)(g))

#define minos_hvc0(id) 				minos_hvc(id, 0, 0, 0, 0, 0, 0, 0)
#define minos_hvc1(id, a)			minos_hvc(id, a, 0, 0, 0, 0, 0, 0)
#define minos_hvc2(id, a, b)			minos_hvc(id, a, b, 0, 0, 0, 0, 0)
#define minos_hvc3(id, a, b, c) 		minos_hvc(id, a, b, c, 0, 0, 0, 0)
#define minos_hvc4(id, a, b, c, d)		minos_hvc(id, a, b, c, d, 0, 0, 0)
#define minos_hvc5(id, a, b, c, d, e)		minos_hvc(id, a, b, c, d, e, 0, 0)
#define minos_hvc6(id, a, b, c, d, e, f)	minos_hvc(id, a, b, c, d, e, f, 0)
#define minos_hvc7(id, a, b, c, d, e, f, g)	minos_hvc(id, a, b, c, d, e, f, g)

struct vm_ring {
	volatile u32_t ridx;
	volatile u32_t widx;
	u32_t size;
	char buf[0];
};

#define VM_RING_IDX(idx, size)	(idx & (size - 1))

static inline void vm_mb(void)
{
	__ISB();
}

static inline void vm_wmb(void)
{
	__DMB();
}

#endif
