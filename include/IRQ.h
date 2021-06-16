#pragma once

// Credit: https://github.com/rsta2/circle

#include <stdint.h>

namespace Armaz {
	struct IRQs {
		volatile uint32_t irq0Pending0;
		volatile uint32_t irq0Pending1;
		volatile uint32_t irq0Pending2;
		volatile uint32_t res0;
		volatile uint32_t irq0Enable0;
		volatile uint32_t irq0Enable1;
		volatile uint32_t irq0Enable2;
		volatile uint32_t res1;
		volatile uint32_t irq0Disable0;
		volatile uint32_t irq0Disable1;
		volatile uint32_t irq0Disable2;
	};

	struct VectorTable {
		struct Entry {
			volatile uint32_t branch;
			volatile uint32_t padding[31];
		};

		volatile Entry entries[16];
	};

	struct AbortFrame {
		uint64_t esr_el1;
		uint64_t spsr_el1;
		uint64_t x30; // lr
		uint64_t elr_el1;
		uint64_t sp_el0;
		uint64_t sp_el1;
		uint64_t far_el1;
		uint64_t unused;
	} __attribute__((packed));

	namespace Interrupts {
		void SMCStub();
		void IRQStub();
		void FIQStub();
		void UnexpectedStub();
		void SynchronousStub();
		void SErrorStub();
		void SecureMonitorHandler(uint32_t function, uint32_t param);
		void ExceptionHandler(uint64_t exception, AbortFrame *frame);
		void InterruptHandler();

		using Handler = void (*)(void *);
		extern Handler handlers[72];
		extern void *params[72];

		void init();
		void connect(unsigned irq, Handler, void *);
		void enable(unsigned irq);

		inline void enableIRQs() { asm volatile("msr DAIFClr, #2"); }
		inline void enableFIQs() { asm volatile("msr DAIFClr, #1"); }
		inline void disableIRQs() { asm volatile("msr DAIFSet, #2"); }
		inline void disableFIQs() { asm volatile("msr DAIFSet, #1"); }
	}
}

extern Armaz::VectorTable vectors;

extern uintptr_t IRQReturnAddress;
