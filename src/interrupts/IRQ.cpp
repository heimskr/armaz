// Credit: https://github.com/rsta2/circle

#include "assert.h"
#include "Config.h"
#include "aarch64/MMIO.h"
#include "aarch64/Synchronize.h"
#include "board/BCM2711.h"
#include "board/BCM2711int.h"
#include "interrupts/IRQ.h"
#include "lib/printf.h"

#define AARCH64_OPCODE_BRANCH(distance) (0x14000000 | (distance))
#define AARCH64_DISTANCE(from, to) ((uint32_t *) &(to) - (uint32_t *) &(from))

#define GICD_CTLR		(ARM_GICD_BASE + 0x000)
	#define GICD_CTLR_DISABLE	(0 << 0)
	#define GICD_CTLR_ENABLE	(1 << 0)
	// secure access
	#define GICD_CTLR_ENABLE_GROUP0	(1 << 0)
	#define GICD_CTLR_ENABLE_GROUP1	(1 << 1)
#define GICD_IGROUPR0		(ARM_GICD_BASE + 0x080)		// secure access for group 0
#define GICD_ISENABLER0		(ARM_GICD_BASE + 0x100)
#define GICD_ICENABLER0		(ARM_GICD_BASE + 0x180)
#define GICD_ISPENDR0		(ARM_GICD_BASE + 0x200)
#define GICD_ICPENDR0		(ARM_GICD_BASE + 0x280)
#define GICD_ISACTIVER0		(ARM_GICD_BASE + 0x300)
#define GICD_ICACTIVER0		(ARM_GICD_BASE + 0x380)
#define GICD_IPRIORITYR0	(ARM_GICD_BASE + 0x400)
	#define GICD_IPRIORITYR_DEFAULT	0xA0
	#define GICD_IPRIORITYR_FIQ	0x40
#define GICD_ITARGETSR0		(ARM_GICD_BASE + 0x800)
	#define GICD_ITARGETSR_CORE0	(1 << 0)
#define GICD_ICFGR0		(ARM_GICD_BASE + 0xC00)
	#define GICD_ICFGR_LEVEL_SENSITIVE	(0 << 1)
	#define GICD_ICFGR_EDGE_TRIGGERED	(1 << 1)
#define GICD_SGIR		(ARM_GICD_BASE + 0xF00)
	#define GICD_SGIR_SGIINTID__MASK		0x0F
	#define GICD_SGIR_CPU_TARGET_LIST__SHIFT	16
	#define GICD_SGIR_TARGET_LIST_FILTER__SHIFT	24

// GIC CPU interface registers
#define GICC_CTLR		(ARM_GICC_BASE + 0x000)
	#define GICC_CTLR_DISABLE	(0 << 0)
	#define GICC_CTLR_ENABLE	(1 << 0)
	// secure access
	#define GICC_CTLR_ENABLE_GROUP0	(1 << 0)
	#define GICC_CTLR_ENABLE_GROUP1	(1 << 1)
	#define GICC_CTLR_FIQ_ENABLE	(1 << 3)
#define GICC_PMR		(ARM_GICC_BASE + 0x004)
	#define GICC_PMR_PRIORITY	(0xF0 << 0)
#define GICC_IAR		(ARM_GICC_BASE + 0x00C)
	#define GICC_IAR_INTERRUPT_ID__MASK	0x3FF
	#define GICC_IAR_CPUID__SHIFT		10
	#define GICC_IAR_CPUID__MASK		(3 << 10)
#define GICC_EOIR		(ARM_GICC_BASE + 0x010)
	#define GICC_EOIR_EOIINTID__MASK	0x3FF
	#define GICC_EOIR_CPUID__SHIFT		10
	#define GICC_EOIR_CPUID__MASK		(3 << 10)

namespace Armaz::Interrupts {
	Handler handlers[72];
	void *params[72];

	void init() {
		VectorTable *vecs = (VectorTable *) 0x70000;
		for (int i = 0; i < 16; ++i)
			vecs->entries[i].branch = AARCH64_OPCODE_BRANCH(AARCH64_DISTANCE(vecs->entries[i].branch,
				i == 8? SMCStub : UnexpectedStub));

		syncDataAndInstructionCache();

		// Initialize distributor

		write32(GICD_CTLR, GICD_CTLR_DISABLE);

		// Disable, acknowledge and deactivate all interrupts
		for (int n = 0; n < IRQ_LINES / 32; ++n) {
			write32(GICD_ICENABLER0 + 4 * n, ~0);
			write32(GICD_ICPENDR0   + 4 * n, ~0);
			write32(GICD_ICACTIVER0 + 4 * n, ~0);
		}

		// Direct all interrupts to core 0 with default priority
		for (int n = 0; n < IRQ_LINES / 4; ++n) {
			write32(GICD_IPRIORITYR0 + 4 * n, GICD_IPRIORITYR_DEFAULT
				| GICD_IPRIORITYR_DEFAULT << 8 | GICD_IPRIORITYR_DEFAULT << 16 | GICD_IPRIORITYR_DEFAULT << 24);
			write32(GICD_ITARGETSR0 + 4 * n, GICD_ITARGETSR_CORE0
				| GICD_ITARGETSR_CORE0 << 8 | GICD_ITARGETSR_CORE0 << 16 | GICD_ITARGETSR_CORE0 << 24);
		}

		// Set all interrupts to level triggered
		for (int n = 0; n < IRQ_LINES / 16; ++n)
			write32(GICD_ICFGR0 + 4 * n, 0);

		write32(GICD_CTLR, GICD_CTLR_ENABLE);

		// Initialize core 0 CPU interface
		write32(GICC_PMR, GICC_PMR_PRIORITY);
		write32(GICC_CTLR, GICC_CTLR_ENABLE);

		enableIRQs();
	}

	void connect(unsigned irq, Handler handler, void *param) {
		assert(irq < IRQ_LINES);
		assert(!handlers[irq]);
		handlers[irq] = handler;
		params[irq] = param;
		enable(irq);
	}

	void enable(unsigned irq) {
		assert(irq < IRQ_LINES);
		write32(GICD_ISENABLER0 + 4 * (irq / 32), 1 << (irq % 32));
	}

	void disconnect(unsigned irq) {
		assert(irq < IRQ_LINES);
		assert(handlers[irq]);
		disable(irq);
		handlers[irq] = nullptr;
		params[irq] = nullptr;
	}

	void disable(unsigned irq) {
		assert(irq < IRQ_LINES);
		write32(GICD_ICENABLER0 + 4 * (irq / 32), 1 << (irq % 32));
	}

	bool callIRQHandler(unsigned irq) {
		assert(irq < IRQ_LINES);
		if (handlers[irq]) {
			(*handlers[irq])(params[irq]);
			return true;
		} else {
			disconnect(irq);
		}

		return false;
	}

	void SecureMonitorHandler(uint32_t function, uint32_t param) {
		printf("SecureMonitorHandler(%u, %u)\n", function, param);
	}

	static uint64_t eh_regs[32];

	void ExceptionHandler(uint64_t exception, AbortFrame *frame) {
		asm volatile("mov %0, x0" : "=r"(eh_regs[0]));
		asm volatile("mov %0, x1" : "=r"(eh_regs[1]));
		asm volatile("mov %0, x2" : "=r"(eh_regs[2]));
		asm volatile("mov %0, x3" : "=r"(eh_regs[3]));
		asm volatile("mov %0, x4" : "=r"(eh_regs[4]));
		asm volatile("mov %0, x5" : "=r"(eh_regs[5]));
		asm volatile("mov %0, x6" : "=r"(eh_regs[6]));
		asm volatile("mov %0, x7" : "=r"(eh_regs[7]));
		asm volatile("mov %0, x8" : "=r"(eh_regs[8]));
		asm volatile("mov %0, x9" : "=r"(eh_regs[9]));
		asm volatile("mov %0, x10" : "=r"(eh_regs[10]));
		asm volatile("mov %0, x11" : "=r"(eh_regs[11]));
		asm volatile("mov %0, x12" : "=r"(eh_regs[12]));
		asm volatile("mov %0, x13" : "=r"(eh_regs[13]));
		asm volatile("mov %0, x14" : "=r"(eh_regs[14]));
		asm volatile("mov %0, x15" : "=r"(eh_regs[15]));
		asm volatile("mov %0, x16" : "=r"(eh_regs[16]));
		asm volatile("mov %0, x17" : "=r"(eh_regs[17]));
		asm volatile("mov %0, x18" : "=r"(eh_regs[18]));
		asm volatile("mov %0, x19" : "=r"(eh_regs[19]));
		asm volatile("mov %0, x20" : "=r"(eh_regs[20]));
		asm volatile("mov %0, x21" : "=r"(eh_regs[21]));
		asm volatile("mov %0, x22" : "=r"(eh_regs[22]));
		asm volatile("mov %0, x23" : "=r"(eh_regs[23]));
		asm volatile("mov %0, x24" : "=r"(eh_regs[24]));
		asm volatile("mov %0, x25" : "=r"(eh_regs[25]));
		asm volatile("mov %0, x26" : "=r"(eh_regs[26]));
		asm volatile("mov %0, x27" : "=r"(eh_regs[27]));
		asm volatile("mov %0, x28" : "=r"(eh_regs[28]));
		asm volatile("mov %0, x29" : "=r"(eh_regs[29]));
		asm volatile("mov %0, x30" : "=r"(eh_regs[30]));
		asm volatile("mov %0, x31" : "=r"(eh_regs[31]));
		printf("ExceptionHandler(%llu, 0x%llx)\n", exception, frame);
		printf("Reason: 0b%b\n", (frame->esr_el1 >> 26) & 0b111111);
		printf("[esr_el1  0x%llx]\n", frame->esr_el1);
		printf("[spsr_el1 0x%llx]\n", frame->spsr_el1);
		printf("[lr       0x%llx]\n", frame->x30);
		printf("[elr_el1  0x%llx]\n", frame->elr_el1);
		printf("[sp_el0   0x%llx]\n", frame->sp_el0);
		printf("[sp_el1   0x%llx]\n", frame->sp_el1);
		printf("[far_el1  0x%llx]\n", frame->far_el1);

		for (unsigned i = 0; i < 32; ++i)
			printf("[x%-2u 0x%llx]\n", i, eh_regs[i]);

		uint64_t lr = eh_regs[29];
		for (int i = 0; i < 10; ++i) {
			printf("[0 0x%llx]\n", *(volatile uint64_t *) lr);
			printf("[1 0x%llx]\n", *((volatile uint64_t *) lr + 1));
			if (*((volatile uint64_t *) lr + 1) == 0x50)
				break;
			lr = *(volatile uint64_t *) lr;
		}

		printf("\n");

		// for (int i = -3; i < 4; ++i)
		// 	printf("[%-2d 0x%llx]\n", i, *((volatile uint64_t *) eh_regs[29] + i));

		for (;;) asm volatile("wfi");
	}

	void InterruptHandler() {
		unsigned iar = read32(GICC_IAR);
		unsigned irq = iar & GICC_IAR_INTERRUPT_ID__MASK;
		if (irq < IRQ_LINES) {
			if (15 < irq) {
				callIRQHandler(irq);
			}
#ifdef ARM_ALLOW_MULTI_CORE
			else {
				// TODO
			}
#endif
			write32(GICC_EOIR, iar);
		} else {
			assert(1020 <= irq);
		}
	}
}
