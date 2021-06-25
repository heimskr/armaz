#pragma once

// Credit: https://github.com/rsta2/circle

#include <stdint.h>

#include "Config.h"
#include "aarch64/Synchronize.h"

namespace Armaz {
	class Spinlock {
		public:
			Spinlock(Level = Level::IRQ);
			void acquire();
			void release();
			static void enable();
		private:
			Level level = Level::IRQ;

#ifdef ARM_ALLOW_MULTI_CORE
		public:
			~Spinlock();
		private:
			uint32_t locked = 0;
			static bool enabled;
#endif
	};
}