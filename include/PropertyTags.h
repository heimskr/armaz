#pragma once

// Credit: https://github.com/rsta2/circle/blob/master/include/circle/bcmpropertytags.h

#include <stdint.h>

namespace Armaz {
	struct PropertyTag {
		uint32_t tagID;
		uint32_t bufferSize;
		uint32_t valueLength;
	} __attribute__((packed));

	struct PropertyTagMemory {
		PropertyTag tag;
		uint32_t baseAddress;
		uint32_t size;
	} __attribute__((packed));

	// Usable for either model or revision.
	struct PropertyTagBoard {
		PropertyTag tag;
		uint32_t board;
	} __attribute__((packed));

	namespace PropertyTags {
		bool getTag(uint32_t id, void *tag, uint32_t tag_size, uint32_t requested_param_size = 0);
		bool getTags(void *tags, uint32_t tags_size);

		constexpr uint8_t CHANNEL_OUT = 8;
		constexpr uint32_t VALUE_LENGTH_RESPONSE = 1 << 31;
	}

	constexpr uint32_t PROPTAG_END                    = 0x00000000;
	constexpr uint32_t PROPTAG_GET_FIRMWARE_REVISION  = 0x00000001;
	constexpr uint32_t PROPTAG_SET_CURSOR_INFO        = 0x00008010;
	constexpr uint32_t PROPTAG_SET_CURSOR_STATE       = 0x00008011;
	constexpr uint32_t PROPTAG_GET_BOARD_MODEL        = 0x00010001;
	constexpr uint32_t PROPTAG_GET_BOARD_REVISION     = 0x00010002;
	constexpr uint32_t PROPTAG_GET_MAC_ADDRESS        = 0x00010003;
	constexpr uint32_t PROPTAG_GET_BOARD_SERIAL       = 0x00010004;
	constexpr uint32_t PROPTAG_GET_ARM_MEMORY         = 0x00010005;
	constexpr uint32_t PROPTAG_GET_VC_MEMORY          = 0x00010006;
	constexpr uint32_t PROPTAG_SET_POWER_STATE        = 0x00028001;
	constexpr uint32_t PROPTAG_GET_CLOCK_RATE         = 0x00030002;
	constexpr uint32_t PROPTAG_GET_MAX_CLOCK_RATE     = 0x00030004;
	constexpr uint32_t PROPTAG_GET_TEMPERATURE        = 0x00030006;
	constexpr uint32_t PROPTAG_GET_MIN_CLOCK_RATE     = 0x00030007;
	constexpr uint32_t PROPTAG_GET_TURBO              = 0x00030009;
	constexpr uint32_t PROPTAG_GET_MAX_TEMPERATURE    = 0x0003000a;
	constexpr uint32_t PROPTAG_GET_EDID_BLOCK         = 0x00030020;
	constexpr uint32_t PROPTAG_GET_THROTTLED          = 0x00030046;
	constexpr uint32_t PROPTAG_NOTIFY_XHCI_RESET      = 0x00030058;
	constexpr uint32_t PROPTAG_SET_CLOCK_RATE         = 0x00038002;
	constexpr uint32_t PROPTAG_SET_TURBO              = 0x00038009;
	constexpr uint32_t PROPTAG_SET_SET_GPIO_STATE     = 0x00038041;
	constexpr uint32_t PROPTAG_SET_SDHOST_CLOCK       = 0x00038042;
	constexpr uint32_t PROPTAG_ALLOCATE_BUFFER        = 0x00040001;
	constexpr uint32_t PROPTAG_GET_DISPLAY_DIMENSIONS = 0x00040003;
	constexpr uint32_t PROPTAG_GET_PITCH              = 0x00040008;
	constexpr uint32_t PROPTAG_GET_TOUCHBUF           = 0x0004000f;
	constexpr uint32_t PROPTAG_GET_GPIO_VIRTBUF       = 0x00040010;
	constexpr uint32_t PROPTAG_GET_NUM_DISPLAYS       = 0x00040013;
	constexpr uint32_t PROPTAG_SET_PHYS_WIDTH_HEIGHT  = 0x00048003;
	constexpr uint32_t PROPTAG_SET_VIRT_WIDTH_HEIGHT  = 0x00048004;
	constexpr uint32_t PROPTAG_SET_DEPTH              = 0x00048005;
	constexpr uint32_t PROPTAG_SET_VIRTUAL_OFFSET     = 0x00048009;
	constexpr uint32_t PROPTAG_SET_PALETTE            = 0x0004800b;
	constexpr uint32_t PROPTAG_WAIT_FOR_VSYNC         = 0x0004800e;
	constexpr uint32_t PROPTAG_SET_BACKLIGHT          = 0x0004800f;
	constexpr uint32_t PROPTAG_SET_DISPLAY_NUM        = 0x00048013;
	constexpr uint32_t PROPTAG_SET_TOUCHBUF           = 0x0004801f;
	constexpr uint32_t PROPTAG_SET_GPIO_VIRTBUF       = 0x00048020;
	constexpr uint32_t PROPTAG_GET_COMMAND_LINE       = 0x00050001;
	constexpr uint32_t PROPTAG_GET_DMA_CHANNELS       = 0x00060001;
}
