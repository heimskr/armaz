#pragma once

// Credit: https://github.com/rsta2/circle

#include "board/BCM2711int.h"
#include "interrupts/IRQ.h"

#include <stddef.h>
#include <stdint.h>

namespace Armaz::GPIO {
	enum class Mode {
		Input,
		Output,
		InputPullUp,
		InputPullDown,
		AlternateFunction0,
		AlternateFunction1,
		AlternateFunction2,
		AlternateFunction3,
		AlternateFunction4,
		AlternateFunction5,
		Unknown
	};

	enum class PullMode {Off, Down, Up, Unknown};

	enum class Interrupt {
		RisingEdge,
		FallingEdge,
		HighLevel,
		LowLevel,
		AsyncRisingEdge,
		AsyncFallingEdge,
		Unknown
	};

	constexpr uint32_t PINS = 54;
	constexpr uint32_t IRQ = ARM_IRQ_GPIO3;

	using InterruptHandler = void (*)(void *);

	class Manager;

	class Pin {
		public:
			Pin();
			Pin(unsigned pin_, Mode, Manager &);
			virtual ~Pin();

			void assign(unsigned);
			void setMode(Mode, bool init = true);
			void setMode(PullMode);
			void write(bool);
			bool read() const;
			void invert();
			void connect(InterruptHandler, void *, bool auto_ack = true);
			void disconnect();
			void enable(Interrupt);
			void disableInterrupt();
			void enable2(Interrupt);
			void disableInterrupt2();
			void acknowledgeInterrupt();

			static void writeAll(unsigned, uint32_t mask);
			static uint32_t readAll();

		private:
			void setAlternateFunction(unsigned);
			void interruptHandler();
			static void disableAllInterrupts(unsigned pin);
			friend class Manager;

		protected:
			volatile unsigned pin = PINS;
			unsigned registerOffset;
			uint32_t registerMask;
			Mode mode = Mode::Unknown;
			bool value;

			Manager *manager;
			InterruptHandler handler = nullptr;
			void *param = nullptr;
			bool autoAck = false;
			Interrupt interrupt  = Interrupt::Unknown;
			Interrupt interrupt2 = Interrupt::Unknown;
	};

	class Manager {
		public:
			Manager();
			~Manager();

			bool init();

		private:
			void connectInterrupt(Pin *);
			void disconnectInterrupt(Pin *);
			friend class Pin;

			void interruptHandler();
			static void interruptStub(void *);

		private:
			// CInterruptSystem *m_pInterrupt;
			bool irqConnected = false;

			Pin *pins[PINS];
	};

	constexpr uint32_t MAX_PIN       = 53;
	constexpr uint32_t FUNCTION_OUT  =  1;
	constexpr uint32_t FUNCTION_ALT5 =  2;
	constexpr uint32_t FUNCTION_ALT3 =  7;

	constexpr uint32_t PULL_NONE = 0;
	constexpr uint32_t PULL_DOWN = 1;
	constexpr uint32_t PULL_UP   = 2;

	constexpr ptrdiff_t GPFSEL0   = 0x200000;
	constexpr ptrdiff_t GPFSEL1   = 0x200004;
	constexpr ptrdiff_t GPFSEL2   = 0x200008;
	constexpr ptrdiff_t GPFSEL3   = 0x20000c;
	constexpr ptrdiff_t GPFSEL4   = 0x200010;
	constexpr ptrdiff_t GPFSEL5   = 0x200014;
	constexpr ptrdiff_t GPSET0    = 0x20001c;
	constexpr ptrdiff_t GPSET1    = 0x200020;
	constexpr ptrdiff_t GPCLR0    = 0x200028;
	constexpr ptrdiff_t GPLEV0    = 0x200034;
	constexpr ptrdiff_t GPLEV1    = 0x200038;
	constexpr ptrdiff_t GPEDS0    = 0x200040;
	constexpr ptrdiff_t GPEDS1    = 0x200044;
	constexpr ptrdiff_t GPREN0    = 0x20004c;
	constexpr ptrdiff_t GPHEN0    = 0x200064;
	constexpr ptrdiff_t GPHEN1    = 0x200068;
	constexpr ptrdiff_t GPAFEN0   = 0x200088;
	constexpr ptrdiff_t GPPUD     = 0x200094;
	constexpr ptrdiff_t GPPUDCLK0 = 0x200098;
	constexpr ptrdiff_t GPPUDCLK1 = 0x20009c;
	constexpr ptrdiff_t GPPUPPDN0 = 0x2000e4;
}
