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
	static std::string cwd = "/";
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
		} else if (front == "ls") {
			CheckDriver();
			const std::string path = 2 <= pieces.size()? FS::simplifyPath(cwd, pieces[1]) : cwd;
			const int status = driver->readdir(path.c_str(), [](const char *str, off_t offset) {
				printf("- %s @ %lld\n", str, offset);
			});
			if (status != 0)
				Error("readdir status: %d", status);
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
			const std::string path = FS::simplifyPath(cwd, pieces[1]);
			const int status = driver->create(path.c_str(), 0666);
			if (status != 0)
				Error("create status: %d", status);
			Success("Created \e[1m%s\e[22m.", path.c_str());
		} else if (front == "mkdir") {
			if (pieces.size() != 2)
				Error("Usage: mkdir <path>");
			CheckDriver();
			const std::string path = FS::simplifyPath(cwd, pieces[1]);
			const int status = driver->mkdir(path.c_str(), 0666);
			if (status != 0)
				Error("mkdir status: %d", status);
			Success("Created \e[1m%s\e[22m.", path.c_str());
		} else if (front == "read") {
			if (pieces.size() != 2)
				Error("Usage: read <path>");
			CheckDriver();
			const std::string path = FS::simplifyPath(cwd, pieces[1]);
			size_t size;
			int status = driver->getsize(path.c_str(), size);
			if (status != 0)
				Error("getsize status: %d", status);
			if (size == 0) {
				Log::warn("File is empty.");
				return true;
			}
			Log::info("Size: %lu", size);
			char *buffer = new char[size];
			if (!buffer)
				Error("Couldn't allocate buffer.");
			status = driver->read(path.c_str(), buffer, size, 0);
			if (status < 0) {
				delete[] buffer;
				Error("read status: %d", status);
			}
			Log::success("Read \e[1m%lu\e[22m bytes from \e[1m%s\e[22m:", size, path.c_str());
			for (size_t i = 0; i < size; ++i)
				UART::write(buffer[i]);
			UART::write('\n');
			delete[] buffer;
		} else if (front == "write") {
			if (pieces.size() < 2)
				Error("Usage: write <path> [data]...");
			CheckDriver();
			const std::string path = FS::simplifyPath(cwd, pieces[1]);
			std::string data;
			for (size_t i = 2; i < pieces.size(); ++i) {
				if (!data.empty())
					data.push_back(' ');
				data += pieces[i];
			}
			int status = driver->truncate(path.c_str(), data.size());
			if (status != 0)
				Error("truncate status: %d", status);
			Log::success("Truncated file to %lu byte%s.", data.size(), data.size() == 1? "" : "s");
			status = driver->write(path.c_str(), data.c_str(), data.size(), 0);
			if (status < 0)
				Error("write status: %d", status);
			Success("Wrote \e[1m%lu\e[22m byte%s to \e[1m%s\e[22m.", data.size(), data.size() == 1? "" : "s",
			        path.c_str());
		} else if (front == "rm") {
			if (pieces.size() != 2)
				Error("Usage: rm <path>");
			CheckDriver();
			const std::string path = FS::simplifyPath(cwd, pieces[1]);
			int status = driver->unlink(path.c_str());
			if (status != 0)
				Error("unlink status: %d", status);
			Success("Successfully removed \e[1m%s\e[22m.", path.c_str());
		} else if (front == "size") {
			if (pieces.size() != 2)
				Error("Usage: size <path>");
			CheckDriver();
			size_t size;
			const std::string path = FS::simplifyPath(cwd, pieces[1]);
			int status = driver->getsize(path.c_str(), size);
			if (status != 0)
				Error("getsize status: %d", status);
			Success("Size of \e[1m%s\e[22m: \e[1m%lu\e[22m", path.c_str(), size);
		} else if (front == "path") {
			if (pieces.size() != 2)
				Error("Usage: path <path>");
			Success("Expanded path: \e[1m%s\e[22m", FS::simplifyPath(cwd, pieces[1]).c_str());
		} else if (front == "cd") {
			if (pieces.size() != 2)
				Error("Usage: cd <path>");
			CheckDriver();
			const std::string path = FS::simplifyPath(cwd, pieces[1]);
			int status = driver->isdir(path.c_str());
			if (status != 1)
				Error("Not a directory: \e[1m%s\e[22m", path.c_str());
			cwd = path;
			Success("Changed directory to \e[1m%s\e[22m", cwd.c_str());
		} else if (front == "pwd") {
			CheckDriver();
			Log::info("Current working directory: \e[1m%s\e[22m", cwd.c_str());
		} else
			Error("Unknown command: %s", front.c_str());

		return true;
	}
}
