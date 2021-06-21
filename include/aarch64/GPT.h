#pragma once

#include <cstdint>
#include <string>

namespace Armaz::GPT {
	struct GUID {
		uint32_t timeLow;
		uint16_t timeMid;
		uint16_t timeHigh: 12;
		uint8_t version: 4;
		uint8_t clock[2];
		uint8_t node[6];

		void print(bool newline = true);
		operator std::string() const;
		operator bool() const;
	};

	struct Header {
		uint64_t signature;
		uint32_t revision;
		uint32_t headerSize;
		uint32_t crc32;
		uint32_t reserved;
		uint64_t currentLBA;
		uint64_t otherLBA;
		uint64_t firstLBA; // First usable LBA for partitions
		uint64_t lastLBA;  // Last usable LBA for partitions
		GUID guid;
		uint64_t startLBA; // Starting LBA of partition entries
		uint32_t partitionCount;
		uint32_t partitionEntrySize;
		uint32_t partitionsCRC32;
	};

	struct PartitionEntry {
		GUID typeGUID;
		GUID partitionGUID;
		uint64_t firstLBA;
		uint64_t lastLBA;
		uint64_t attributes;
		char16_t name[36];

		void printName(bool newline = true);
		operator std::string() const;
	};
}
