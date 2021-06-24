// Credit: https://github.com/rsta2/circle

#include "assert.h"
#include "aarch64/MMIO.h"
#include "interrupts/IRQ.h"
#include "pi/GPIO.h"

#include "pi/UART.h"
#include "lib/printf.h"

namespace Armaz::GPIO {
	Pin::Pin() {}

	Pin::Pin(unsigned pin_, Mode mode_, Manager &manager_): mode(mode_), manager(&manager_) {
		assign(pin_);
		setMode(mode_, true);
	}

	Pin::~Pin() {}

	void Pin::assign(unsigned pin_) {
		assert(pin == PINS);
		pin = pin_;

		// if (PINS <= pin)
		// 	pin = CMachineInfo::Get()->GetGPIOPin((VirtualPin) pin);

		assert(pin < PINS);
		registerOffset = (pin / 32) * 4;
		registerMask = 1 << (pin % 32);
	}

	void Pin::setMode(Mode mode_, bool init) {
		assert(static_cast<int>(mode_) < static_cast<int>(Mode::Unknown));
		mode = mode_;
		int intmode = static_cast<int>(mode);

		if ((int) Mode::AlternateFunction0 <= intmode && intmode <= (int) Mode::AlternateFunction5) {
			if (init)
				setMode(PullMode::Off);

			setAlternateFunction(intmode - static_cast<int>(Mode::AlternateFunction0));
			return;
		}

		if (init && mode == Mode::Output)
			setMode(PullMode::Off);

		assert(pin < PINS);
		ptrdiff_t sel_reg = GPFSEL0 + (pin / 10) * 4;
		uint32_t shift = 3 * (pin % 10);

		// spinLock.acquire();
		uint32_t new_value = MMIO::read(sel_reg);
		new_value |= (mode == Mode::Output? 1 : 0) << shift;
		new_value &= ~(7 << shift);
		MMIO::write(sel_reg, new_value);
		// spinLock.release();

		if (init)
			switch (mode) {
				case Mode::Input:
					setMode(PullMode::Off);
					break;
				case Mode::Output:
					write(false);
					break;
				case Mode::InputPullUp:
					setMode(PullMode::Up);
					break;
				case Mode::InputPullDown:
					setMode(PullMode::Down);
					break;
				default:
					break;
			}
	}

	void Pin::write(bool value_) {
		assert(pin < PINS);
		// Output level can be set in input mode for subsequent switch to output
		assert(static_cast<int>(mode) < static_cast<int>(Mode::AlternateFunction0));
		value = value_;
		MMIO::write((value? GPSET0 : GPCLR0) + registerOffset, registerMask);
	}

	bool Pin::read() const {
		assert(pin < PINS);
		assert(mode == Mode::Input || mode == Mode::InputPullUp || mode == Mode::InputPullDown);
		return !!(MMIO::read(GPLEV0 + registerOffset) & registerMask);
	}

	void Pin::invert() {
		assert(mode == Mode::Output);
		write(!value);
	}

	void Pin::connect(InterruptHandler handler_, void *param_, bool auto_ack) {
		assert(mode == Mode::Input || mode == Mode::InputPullUp || mode == Mode::InputPullDown);
		assert(interrupt == Interrupt::Unknown);
		assert(interrupt2 == Interrupt::Unknown);
		assert(handler_);
		assert(!handler);

		handler = handler_;
		param = param_;
		autoAck = auto_ack;

		assert(manager);
		manager->connectInterrupt(this);
	}

	void Pin::disconnect() {
		assert(mode == Mode::Input || mode == Mode::InputPullUp || mode == Mode::InputPullDown);
		assert(interrupt == Interrupt::Unknown);
		assert(interrupt2 == Interrupt::Unknown);

		assert(handler);
		handler = nullptr;

		assert(manager);
		manager->disconnectInterrupt(this);
	}

	void Pin::enable(Interrupt interrupt_) {
		assert(mode == Mode::Input || mode == Mode::InputPullUp || mode == Mode::InputPullDown);
		assert(handler);

		assert(interrupt == Interrupt::Unknown);
		assert(interrupt_ != interrupt2);
		interrupt = interrupt_;

		const ptrdiff_t reg = GPREN0 + registerOffset + ((int) interrupt_ - (int) Interrupt::RisingEdge) * 12;

		// spinLock.acquire();
		MMIO::write(reg, MMIO::read(reg) | registerMask);
		// spinLock.release();
	}

	void Pin::disableInterrupt() {
		assert(mode == Mode::Input || mode == Mode::InputPullUp || mode == Mode::InputPullDown);

		const ptrdiff_t reg = GPREN0 + registerOffset + ((int) interrupt - (int) Interrupt::RisingEdge) * 12;

		// spinLock.acquire();
		MMIO::write(reg, MMIO::read(reg) & ~registerMask);
		// spinLock.release();

		interrupt = Interrupt::Unknown;
	}

	void Pin::enable2(Interrupt interrupt_) {
		assert(mode == Mode::Input || mode == Mode::InputPullUp || mode == Mode::InputPullDown);
		assert(handler);

		assert(interrupt2 == Interrupt::Unknown);
		assert(interrupt_ != interrupt);
		interrupt2 = interrupt_;

		ptrdiff_t reg = GPREN0 + registerOffset + ((int) interrupt_ - (int) Interrupt::RisingEdge) * 12;

		// spinLock.acquire();
		MMIO::write(reg, MMIO::read(reg) | registerMask);
		// spinLock.release();
	}

	void Pin::disableInterrupt2() {
		assert(mode == Mode::Input || mode == Mode::InputPullUp || mode == Mode::InputPullDown);

		ptrdiff_t reg = GPREN0 + registerOffset + ((int) interrupt2 - (int) Interrupt::RisingEdge) * 12;

		// spinLock.acquire();
		MMIO::write(reg, MMIO::read(reg) & ~registerMask);
		// spinLock.release();

		interrupt2 = Interrupt::Unknown;
	}

	void Pin::acknowledgeInterrupt() {
		assert(handler);
		assert(!autoAck);
		MMIO::write(GPEDS0 + registerOffset, registerMask);
	}

	void Pin::writeAll(unsigned value_, uint32_t mask) {
		uint32_t clear = ~value_ & mask;
		if (clear != 0)
			MMIO::write(GPCLR0, clear);

		uint32_t set = value_ & mask;
		if (set != 0)
			MMIO::write(GPSET0, set);
	}

	uint32_t Pin::readAll() {
		return MMIO::read(GPLEV0);
	}

	void Pin::setMode(PullMode pull_mode) {
		// spinLock.acquire();

		assert(pin < PINS);
		const ptrdiff_t mode_reg = GPPUPPDN0 + (pin / 16) * 4;
		const unsigned shift = (pin % 16) * 2;

		assert(static_cast<int>(pull_mode) <= 2);
		static const unsigned mode_map[3] = {0, 2, 1};

		MMIO::write(mode_reg, (MMIO::read(mode_reg) & ~(3 << shift)) | (mode_map[(int) pull_mode] << shift));

		// spinLock.release();
	}

	void Pin::setAlternateFunction(unsigned function) {
		assert(pin < PINS);
		const ptrdiff_t sel_reg = GPFSEL0 + (pin / 10) * 4;
		const unsigned shift = (pin % 10) * 3;

		assert(function <= 5);
		static const unsigned function_map[6] = {4, 5, 6, 7, 3, 2};

		// spinLock.acquire();
		MMIO::write(sel_reg, (MMIO::read(sel_reg) & ~(7 << shift)) | (function_map[function] << shift));
		// spinLock.release();
	}

	void Pin::interruptHandler() {
		assert(mode == Mode::Input || mode == Mode::InputPullUp || mode == Mode::InputPullDown);
		assert((int) interrupt < (int) Interrupt::Unknown || (int) interrupt2 < (int) Interrupt::Unknown);
		assert(handler);
		handler(param);
	}

	void Pin::disableAllInterrupts(unsigned pin_) {
		assert(pin_ < PINS);

		ptrdiff_t reg = GPREN0 + (pin_ / 32) * 4;
		const uint32_t mask = 1 << (pin_ % 32);

		// spinLock.acquire();
		for (; reg < GPAFEN0 + 4; reg += 12)
			MMIO::write(reg, MMIO::read(reg) & ~mask);
		// spinLock.release();
	}

	Manager::Manager() {
		for (unsigned pin_id = 0; pin_id < PINS; ++pin_id)
			pins[pin_id] = nullptr;
	}

	Manager::~Manager() {
#ifndef NDEBUG
		for (unsigned pin_id = 0; pin_id < PINS; ++pin_id)
			assert(!pins[pin_id]);
#endif
		if (irqConnected)
			Interrupts::disconnect(IRQ);
	}

	bool Manager::init() {
		assert(!irqConnected);
		Interrupts::connect(IRQ, interruptStub, this);
		return irqConnected = true;
	}

	void Manager::connectInterrupt(Pin *pin) {
		assert(irqConnected);
		assert(pin);
		const unsigned pin_id = pin->pin;
		assert(pin_id < PINS);
		assert(!pins[pin_id]);
		pins[pin_id] = pin;
	}

	void Manager::disconnectInterrupt(Pin *pin) {
		assert(irqConnected);
		assert(pin);
		unsigned pin_id = pin->pin;
		assert(pin_id < PINS);
		assert(pins[pin_id]);
		pins[pin_id] = nullptr;
	}

	void Manager::interruptHandler() {
		assert(irqConnected);
		uint32_t event_status = MMIO::read(GPEDS0);

		unsigned pin_id = 0;
		while (pin_id < PINS) {
			if (event_status & 1)
				break;
			event_status >>= 1;
			if (++pin_id % 32 == 0)
				event_status = MMIO::read(GPEDS0 + 4);
		}

		if (pin_id < PINS) {
			Pin *pin = pins[pin_id];
			if (pin) {
				pin->interruptHandler();
				if (pin->autoAck)
					MMIO::write(GPEDS0 + pin->registerOffset, pin->registerMask);
			} else {
				// disable all interrupt sources
				Pin::disableAllInterrupts(pin_id);
				MMIO::write(GPEDS0 + (pin_id / 32) * 4, 1 << (pin_id % 32));
			}
		}
	}

	void Manager::interruptStub(void *param) {
		assert(param);
		reinterpret_cast<Manager *>(param)->interruptHandler();
	}




	bool call(uint32_t pin_number, uint32_t value, uintptr_t base, uint32_t field_size, uint32_t field_max) {
		uint32_t field_mask = (1 << field_size) - 1;
	
		if (field_max < pin_number || field_mask < value)
			return false;

		uint32_t num_fields = 32 / field_size;
		uintptr_t reg = base + ((pin_number / num_fields) * 4);
		uint32_t shift = (pin_number % num_fields) * field_size;

		uint32_t curval = MMIO::read(reg);
		curval &= ~(field_mask << shift);
		curval |= value << shift;
		MMIO::write(reg, curval);

		return true;
	}

	bool set(uint32_t pin_number, uint32_t value) {
		return call(pin_number, value, GPSET0, 1, MAX_PIN);
	}

	bool clear(uint32_t pin_number, uint32_t value) {
		return call(pin_number, value, GPCLR0, 1, MAX_PIN);
	}

	bool pull(uint32_t pin_number, uint32_t value) {
		return call(pin_number, value, GPPUPPDN0, 2, MAX_PIN);
	}

	bool function(uint32_t pin_number, uint32_t value) {
		return call(pin_number, value, GPFSEL0, 3, MAX_PIN);
	}

	void useAsAlt3(uint32_t pin_number) {
		pull(pin_number, PULL_NONE);
		function(pin_number, FUNCTION_ALT3);
	}

	void useAsAlt5(uint32_t pin_number) {
		pull(pin_number, PULL_NONE);
		function(pin_number, FUNCTION_ALT5);
	}
}
