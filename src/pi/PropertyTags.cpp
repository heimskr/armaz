// Credit: https://github.com/rsta2/circle/blob/master/lib/bcmpropertytags.cpp

#include <string.h>

#include "assert.h"
#include "BCM2835.h"
#include "Mailbox.h"
#include "Memory.h"
#include "printf.h"
#include "PropertyTags.h"
#include "Synchronize.h"

namespace Armaz::PropertyTags {
	struct PropertyBuffer {
		uint32_t bufferSize; // In bytes
		uint32_t code;
		uint8_t  tags[0];
	} __attribute__((packed));

	constexpr uint32_t CODE_REQUEST          = 0x00000000;
	constexpr uint32_t CODE_RESPONSE_SUCCESS = 0x80000000;
	constexpr uint32_t CODE_RESPONSE_FAILURE = 0x80000001;

	bool getTag(uint32_t id, void *tag, uint32_t tag_size, uint32_t requested_param_size) {
		assert(tag);
		assert(sizeof(PropertyTag) + sizeof(uint32_t) <= tag_size);

		PropertyTag *header = reinterpret_cast<PropertyTag *>(tag);
		header->tagID = id;
		header->bufferSize = tag_size - sizeof(PropertyTag);
		header->valueLength = requested_param_size & ~VALUE_LENGTH_RESPONSE;

		if (!getTags(tag, tag_size))
			return false;

		header->valueLength &= ~VALUE_LENGTH_RESPONSE;
		return header->valueLength != 0;
	}

	bool getTags(void *tags, uint32_t tags_size) {
		assert(tags);
		assert(sizeof(PropertyTag) + sizeof(uint32_t) <= tags_size);
		uint32_t buffer_size = sizeof(PropertyBuffer) + tags_size + sizeof(uint32_t);
		assert((buffer_size & 3) == 0);

		PropertyBuffer *buffer = reinterpret_cast<PropertyBuffer *>(Memory::getCoherentPage(Memory::SLOT_PROP_MAILBOX));
		buffer->bufferSize = buffer_size;
		buffer->code = CODE_REQUEST;
		memcpy(buffer->tags, tags, tags_size);

		uint32_t *end_tag = reinterpret_cast<uint32_t *>(buffer->tags + tags_size);
		*end_tag = PROPTAG_END;

		dataSyncBarrier();

		uint32_t buffer_address = BUS_ADDRESS((uintptr_t) buffer);
		if (Mailbox::writeRead(CHANNEL_OUT, buffer_address) != buffer_address)
			return false;

		dataMemBarrier();

		if (buffer->code != CODE_RESPONSE_SUCCESS)
			return false;

		memcpy(tags, buffer->tags, tags_size);
		return true;
	}
}
