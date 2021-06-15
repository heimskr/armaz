#include "BCM2711.h"
#include "BCM2711int.h"
#include "IRQ.h"
#include "MMIO.h"
#include "printf.h"

// Credit: https://github.com/rsta2/circle

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
	void init() {
		for (int i = 0; i < 16; ++i)
			vectors.entries[i].branch = AARCH64_OPCODE_BRANCH(AARCH64_DISTANCE(vectors.entries[i].branch,
				i == 8? SMCStub : UnexpectedStub));

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

	void SecureMonitorHandler(uint32_t function, uint32_t param) {
		printf("SecureMonitorHandler(%u, %u)\n", function, param);
	}

	void ExceptionHandler(uint64_t exception, AbortFrame *frame) {
		printf("ExceptionHandler(%llu, 0x%llx)\n", exception, frame);
	}

	void InterruptHandler() {
		printf("InterruptHandler()\n");
	}
}
