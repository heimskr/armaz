#pragma once

// Credit: https://github.com/rsta2/circle

#include <cstdint>

namespace Armaz::MMU {
	constexpr uint32_t SCTLR_EL1_WXN = 1 << 19;
	constexpr uint32_t SCTLR_EL1_I   = 1 << 12;
	constexpr uint32_t SCTLR_EL1_C   = 1 <<  2;
	constexpr uint32_t SCTLR_EL1_A   = 1 <<  1;
	constexpr uint32_t SCTLR_EL1_M   = 1 <<  0;

	void enable();
	void disable();

	struct Level2TableDescriptor {
		uint64_t value11     : 2,  // set to 3
		         ignored1    : 14, // set to 0
		         tableAddress: 32, // table base address [47:16]
		         reserved0   : 4,  // set to 0
		         ignored2    : 7,  // set to 0
		         pxnTable    : 1,  // set to 0
		         uxnTable    : 1,  // set to 0
		         apTable     : 2,
		         nsTable     : 1;  // RES0, set to 0
	} __attribute__((packed));

	struct Level2BlockDescriptor {
		uint64_t value01      : 2,  // set to 1
		         attrIndex    : 3,  // [2:0], see MAIR_EL1
		         ns           : 1,  // RES0, set to 0
		         ap           : 2,  // [2:1]
		         sh           : 2,  // [1:0]
		         af           : 1,  // set to 1, will fault otherwise
		         ng           : 1,  // set to 0
		         reserved0    : 17, // set to 0
		         outputAddress: 19, // [47:29]
		         reserved1    : 4,  // set to 0
		         continous    : 1,  // set to 0
		         pxn          : 1,  // set to 0, 1 for device memory
		         uxn          : 1,  // set to 1
		         ignored      : 9;  // set to 0
	} __attribute__((packed));

	struct Level2InvalidDescriptor {
		uint64_t value0 : 1, // set to 0
		         ignored: 63;
	} __attribute__((packed));

	union Level2Descriptor {
		Level2TableDescriptor   table;
		Level2BlockDescriptor   block;
		Level2InvalidDescriptor invalid;
	} __attribute__((packed));

	struct Level3PageDescriptor {
		uint64_t value11      : 2,  // set to 3
		         attrIndex    : 3,  // [2:0], see mair_el1
		         ns           : 1,  // RES0, set to 0
		         ap           : 2,  // [2:1]
		         sh           : 2,  // [1:0]
		         af           : 1,  // set to 1, will fault otherwise
		         ng           : 1,  // set to 0
		         reserved0    : 4,  // set to 0
		         outputAddress: 32, // [47:16]
		         reserved1    : 4,  // set to 0
		         continous    : 1,  // set to 0
		         pxn          : 1,  // set to 0, 1 for device memory
		         uxn          : 1,  // set to 1
		         ignored      : 9;  // set to 0
	} __attribute__((packed));

	struct Level3InvalidDescriptor {
		uint64_t value0 : 1, // set to 0
		         ignored: 63;
	} __attribute__((packed));

	union Lavel3Descriptor {
		Level3PageDescriptor    page;
		Level3InvalidDescriptor invalid;
	} __attribute__((packed));

}
