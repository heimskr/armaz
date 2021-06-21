#include "lib/printf.h"
#include "storage/MBR.h"

namespace Armaz {
	void MBREntry::debug() {
		printf("    Attributes: 0x%02x\n", attributes);
		printf("    Type: 0x%02x\n", type);
		printf("    Start CHS: (%u, %u, %u)\n", startCHS.getCylinders(), startCHS.heads, startCHS.sectors);
		printf("    Start LBA: %u (%u)\n", startCHS.toLBA(), startLBA);
		printf("    Last LBA: %u\n", lastSectorCHS.toLBA());
		printf("    Sectors: %u (%llu bytes)\n", sectors, 512ul * static_cast<size_t>(sectors));
	}

	void MBR::debug() {
		printf("Disk ID: 0x%02x%02x%02x%02x\n", diskID[0], diskID[1], diskID[2], diskID[3]);
		printf("-------------\n");
		printf("First entry:\n");
		firstEntry.debug();
		printf("-------------\n");
		printf("Second entry:\n");
		secondEntry.debug();
		printf("-------------\n");
		printf("Third entry:\n");
		thirdEntry.debug();
		printf("-------------\n");
		printf("Fourth entry:\n");
		fourthEntry.debug();
		printf("-------------\n");
		printf("Signature: %02x%02x\n", signature[0], signature[1]);
	}
}
