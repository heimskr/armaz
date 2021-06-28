#include <memory>

#include "Log.h"
#include "Test.h"
#include "util.h"
#include "fs/tfat/ThornFAT.h"
#include "lib/printf.h"
#include "pi/UART.h"
#include "storage/EMMC.h"
#include "storage/MBR.h"
#include "storage/Partition.h"

namespace Armaz {
	static EMMCDevice emmc;
	static MBR mbr;
	static bool mbr_read = false;
	std::unique_ptr<Partition> partition;
	std::unique_ptr<ThornFAT::ThornFATDriver> driver;

	static bool readMBR() {
		if (mbr_read)
			return true;
		if (!emmc.isReady()) {
			Log::error("Can't read MBR: EMMCDevice isn't ready");
			return false;
		}

		ssize_t status;
		if ((status = emmc.read(&mbr, sizeof(mbr))) < 0) {
			Log::error("Failed to read MBR (%ld).", status);
			return false;
		}
		return mbr_read = true;
	}

	bool test(const std::string &string) {
		return test(Util::splitToVector(string, " ", true));
	}

	bool test(const std::vector<std::string> &pieces) {
		if (pieces.empty())
			return true;

		const std::string &front = pieces.front();

#define Success(args...) do { Log::success(args); return true; } while (0)
#define Error(args...) do { Log::error(args); return false; } while (0)
#define CheckDriver() do { if (!driver) Error("Driver isn't initialized. Use tfat init."); } while (0)

		if (front == "emmc") {
			if (pieces.size() != 2 || pieces[1] != "init")
				Error("Usage: emmc init");
			else if (!emmc.init())
				Error("Failed to initialize EMMCDevice.");
			else
				Success("Initialized EMMCDevice.");
		} else if (front == "mbr") {
			if (readMBR())
				mbr.debug();
			else
				return false;
		} else if (front == "tfat") {
			auto usage = [] { Error("Usage:\n- tfat init\n- tfat make"); };
			if (pieces.size() < 2 || pieces[1] == "?")
				return usage();

			const std::string &verb = pieces[1];
			if (verb == "init") {
				if (!readMBR())
					return false;

				if (partition && driver) {
					Log::info("Partition and driver are already initialized.");
					return true;
				}

				if (!partition) {
					if ((partition = std::make_unique<Partition>(emmc, mbr.thirdEntry)))
						Log::success("Partition initialized.");
					else
						Error("Couldn't initialize partition.");
				}

				if (!driver) {
					if ((driver = std::make_unique<ThornFAT::ThornFATDriver>(partition.get())))
						Log::success("Driver initialized.");
					else
						Error("Couldn't initialize driver.");
				}
			} else if (verb == "make") {
				CheckDriver();

				if (driver->make(sizeof(ThornFAT::DirEntry) * 5))
					Success("ThornFAT filesystem created.");
				else
					Error("Couldn't create ThornFAT filesystem.");
			} else
				return usage();
		} else if (front == "readdir") {
			CheckDriver();

			const char *path = 2 <= pieces.size()? pieces[1].c_str() : "/";

			int status;
			if ((status = driver->readdir(path, [](const char *str, off_t offset) {
				printf("- %s @ %lld\n", str, offset);
			})))
				Error("readdir failed: %d", status);
		} else if (front == "readblock") {
			if (pieces.size() != 2)
				Error("Usage: readblock <byte offset>");

			if (!partition)
				Error("Partition isn't initialized.");

			unsigned long byte_offset;
			if (!Util::parseUlong(pieces[1], byte_offset))
				Error("Invalid byte offset");

			if (byte_offset % 512)
				Error("Byte offset must be a multiple of 512");

			char bytes[512];
			ssize_t status;
			if ((status = partition->read(bytes, sizeof(bytes), byte_offset)) < 0)
				Error("Couldn't read from partition (%ld)", status);

			for (int i = 0; i < 512; ++i) {
				printf("%02x", bytes[i]);
				if (i % 64 == 63)
					UART::write('\n');
			}

			UART::write('\n');
			for (int i = 0; i < 512 / 4; ++i)
				printf("%3d: %d\n", i, ((int *) bytes)[i]);
		} else if (front == "create") {
			if (pieces.size() != 2)
				Error("Usage: create <path>");
			CheckDriver();
			const int status = driver->create(pieces[1].c_str(), 0666);
			if (status != 0)
				Error("Status: %d\n", status);
			else
				Success("Created \e[1m%s\e[22m.", pieces[1].c_str());
		} else
			Error("Unknown command: %s", front.c_str());

		return true;
	}
}
