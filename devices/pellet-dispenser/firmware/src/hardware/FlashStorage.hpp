#pragma once

#include "Log.hpp"
#include "boards/pico.h"
#include "build_timestamp.h"
#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>

#include "../utils/Defer.hpp"

// Utils to comppute unique hash for a class
namespace details {

constexpr uint32_t stol_impl(const char *str, uint32_t value) {
	if (*str == 0) {
		return value;
	}
	return stol_impl(str + 1, (*str - '0' + 10 * value));
};

constexpr uint32_t stol(const char *str) {
	return stol_impl(str, 0);
}

constexpr char tolower(char c) {
	if (c >= 65 && c <= 92) {
		return c + 32;
	}
	return c;
}

constexpr int getMonFromAbbr(const char *_abbr) {
	char abbr[3] = {tolower(_abbr[0]), tolower(_abbr[1]), tolower(_abbr[2])};

	if (abbr[0] == 'j' && abbr[1] == 'a' && abbr[2] == 'n') {
		return 0;
	}
	if (abbr[0] == 'f' && abbr[1] == 'e' && abbr[2] == 'b') {
		return 1;
	}
	if (abbr[0] == 'm' && abbr[1] == 'a' && abbr[2] == 'r') {
		return 2;
	}
	if (abbr[0] == 'a' && abbr[1] == 'p' && abbr[2] == 'r') {
		return 3;
	}
	if (abbr[0] == 'm' && abbr[1] == 'a' && abbr[2] == 'y') {
		return 4;
	}
	if (abbr[0] == 'j' && abbr[1] == 'u' && abbr[2] == 'n') {
		return 5;
	}
	if (abbr[0] == 'j' && abbr[1] == 'u' && abbr[2] == 'l') {
		return 6;
	}
	if (abbr[0] == 'a' && abbr[1] == 'u' && abbr[2] == 'g') {
		return 7;
	}
	if (abbr[0] == 's' && abbr[1] == 'e' && abbr[2] == 'p') {
		return 8;
	}
	if (abbr[0] == 'o' && abbr[1] == 'c' && abbr[2] == 't') {
		return 9;
	}
	if (abbr[0] == 'n' && abbr[1] == 'o' && abbr[2] == 'v') {
		return 10;
	}
	if (abbr[0] == 'd' && abbr[1] == 'e' && abbr[2] == 'c') {
		return 11;
	}
	return (-1);
}

constexpr int64_t epochFromDateAndTime(const char *date, const char *time) {
	char dayStr[3] = {date[4], date[5], 0};

	uint32_t month = getMonFromAbbr(date);

	uint32_t day  = stol(dayStr);
	uint32_t year = stol(date + 7);

	char hoursStr[3]   = {time[0], time[1], 0};
	char minutesStr[3] = {time[3], time[4], 0};

	int64_t hours   = stol(hoursStr);
	int64_t minutes = stol(minutesStr);
	int64_t seconds = stol(time + 6);

	int64_t days = int64_t(year - 1970) * 365 + 31 * month + day;

	return days * 24 * 3600 + hours * 3600 + minutes * 60 + seconds;
}

struct InstanceCounter {
	inline static size_t Value = 0;
};

} // namespace details

struct FlashObjectHeader {
	uint32_t UniqueCode;
};

template <
    class T,
    size_t   PagesPerObject = 1,
    size_t   SectorSize     = 1,
    uint32_t UUID           = BUILD_EPOCH>
class FlashStorage : public details::InstanceCounter {
public:
	const static uint32_t MagicWord = UUID;

	using Type = std::remove_cv_t<std::remove_reference_t<T>>;

	static constexpr size_t MaxObjectSize =
	    PagesPerObject * FLASH_PAGE_SIZE - sizeof(FlashObjectHeader);

	static constexpr size_t PagesPerSector =
	    FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE;

	static_assert(
	    PagesPerObject > 0, "Invalid PagesPerObject: must be strictly positive"
	);
	static_assert(
	    PagesPerObject <= PagesPerSector,
	    "Invalid PagesPerObject: must fit in a single sector"
	);
	static_assert(
	    PagesPerSector % PagesPerObject == 0,
	    "Invalid PagesPerObject: must be a divisor of number of pages in a "
	    "sector"
	);

	static_assert(
	    sizeof(T) < MaxObjectSize,
	    "Maximal Allowed Object Size exceeded, try Increase "
	    "PagesPerObject"
	);

	inline static bool Load(T &obj) {
		uint interrupts = save_and_disable_interrupts();
		defer {
			restore_interrupts(interrupts);
		};

		Infof("FlashStorage: UUID:%x", UUID);
		const Type *valid = nullptr;
		for (size_t i = 0; i < PagesPerSector * SectorSize;
		     i += PagesPerObject) {
			const FlashObjectHeader *h = reinterpret_cast<FlashObjectHeader *>(
			    XIP_BASE + Offset + i * FLASH_PAGE_SIZE
			);
			if (h->UniqueCode == UUID) {
				valid = reinterpret_cast<const Type *>(
				    reinterpret_cast<const uint8_t *>(h) +
				    sizeof(FlashObjectHeader)
				);
			}
		}
		if (valid == nullptr) {
			return false;
		}
		obj = *valid;
		return true;
	}

	inline static void Save(const Type &obj) {
		uint interrupts = save_and_disable_interrupts();
		defer {
			restore_interrupts(interrupts);
		};

		if (isSameThanSaved(obj)) {
			Debugf("FlashStorage: not saving as it is the same");
			return;
		}

		size_t i;
		for (i = 0; i < PagesPerSector * SectorSize; i += PagesPerObject) {
			const uint8_t *ptr = reinterpret_cast<const uint8_t *>(
			    XIP_BASE + Offset + i * FLASH_PAGE_SIZE
			);
			bool isFree = true;
			for (size_t j = 0; j < sizeof(FlashObjectHeader) + sizeof(Type);
			     j++) {
				if (*(ptr + j) != 0xff) {
					isFree = false;
					break;
				}
			}
			if (isFree == true) {
				break;
			}
		}

		if (i == PagesPerSector * SectorSize) {
			flash_range_erase(Offset, SectorSize * FLASH_SECTOR_SIZE);
			i = 0;
		}

		uint8_t           buffer[PagesPerObject * FLASH_PAGE_SIZE];
		FlashObjectHeader h = {.UniqueCode = UUID};

		memcpy(buffer, &h, sizeof(FlashObjectHeader));
		memcpy(buffer + sizeof(FlashObjectHeader), &obj, sizeof(Type));
		memset(
		    buffer + sizeof(FlashObjectHeader) + sizeof(Type),
		    0xff,
		    PagesPerObject * FLASH_PAGE_SIZE - sizeof(FlashObjectHeader) -
		        sizeof(Type)
		);

		flash_range_program(
		    Offset + i * PagesPerObject * FLASH_PAGE_SIZE,
		    buffer,
		    PagesPerObject * FLASH_PAGE_SIZE
		);
		Infof("FlashStorage: saved at page %d", i * PagesPerObject);
	}

	static void Debug() {
		int interrupts = save_and_disable_interrupts();
		defer {
			restore_interrupts(interrupts);
		};

		constexpr size_t lineSize = 16; // 4 * 20 = 80

		constexpr static auto print4Bytes = [](const uint8_t *data) {
			printf("%02X %02X %02X %02X", data[0], data[1], data[2], data[3]);
		};
		constexpr static auto print16Bytes = [](const uint8_t *data) {
			print4Bytes(data);
			printf("  ");
			print4Bytes(data + 4);
			printf(" . ");
			print4Bytes(data + 8);
			printf("  ");
			print4Bytes(data + 12);
		};
		constexpr static auto printLine = [](const uint8_t *data) {
			printf("| ");
			print16Bytes(data);
			printf(" - ");
			print16Bytes(data + 16);
			printf(" |\n");
		};

		constexpr static auto printPage = [](const uint8_t *data, int index) {
			constexpr static size_t lineLength = 32 * 3 + 11;
			printf(
			    "+- %Page:%02d %.*s+\n",
			    index,
			    lineLength - 10,
			    "----------------------------------------------------------"
			    "----------------------------------------------------------"
			);
			printLine(data);
			printLine(data + 32);
			printLine(data + 64);
			printLine(data + 96);
			printf(
			    "|%.*s|\n",
			    lineLength,
			    "                                                              "
			    "                                                              "
			);
			printLine(data + 128);
			printLine(data + 160);
			printLine(data + 192);
			printLine(data + 224);
			printf(
			    "+%.*s+\n",
			    lineLength,
			    "----------------------------------------------------------"
			    "----------------------------------------------------------"
			);
		};
		for (size_t i = 0; i < SectorSize * FLASH_SECTOR_SIZE; i += 256) {
			printPage(
			    reinterpret_cast<const uint8_t *>(XIP_BASE + Offset + i),
			    i / 256
			);
		}
	}

private:
	static constexpr uint Offset =
	    PICO_FLASH_SIZE_BYTES - SectorSize * FLASH_SECTOR_SIZE;

	static bool isSameThanSaved(const Type &obj) {
		Type saved;
		if (!Load(saved)) {
			return false;
		}
		return memcmp(&saved, &obj, sizeof(Type)) == 0;
	}
};
