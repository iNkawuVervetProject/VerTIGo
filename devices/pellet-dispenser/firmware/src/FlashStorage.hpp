#pragma once

#include "boards/pico.h"
#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"
#include <cstdint>
#include <stdexcept>
#include <type_traits>

// Utils to comppute unique hash for a class
namespace details {

constexpr uint32_t stol_impl(const char *str, uint32_t value) {
	if (*str == 0) {
		return value;
	}
	assert('0' <= *str && *str <= '9');
	return stol_impl(str + 1, (*str - '0' + 10 * value));
};

constexpr uint32_t stol(const char *str) {
	return stol_impl(str, 0);
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
    uint32_t UUID           = details::stol(__TIME__)>
class FlashStorage : public details::InstanceCounter {
public:
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

	inline static void Save(const T &obj) {
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
			uint interrupts = save_and_disable_interrupts();
			flash_range_erase(Offset, SectorSize * FLASH_SECTOR_SIZE);
			restore_interrupts(interrupts);
			i = 0;
		}

		uint8_t           buffer[PagesPerObject * FLASH_PAGE_SIZE];
		FlashObjectHeader h = {.UniqueCode = UUID};

		memcpy(buffer, &h, sizeof(FlashObjectHeader));
		memcpy(buffer + sizeof(FlashObjectHeader), &obj, sizeof(obj));

		uint interrupts = save_and_disable_interrupts();
		flash_range_program(
		    Offset + i * PagesPerObject * FLASH_PAGE_SIZE,
		    buffer,
		    PagesPerObject * FLASH_PAGE_SIZE
		);
		restore_interrupts(interrupts);
	}

	static void Debug() {
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

		constexpr static auto printPage = [](const uint8_t *data) {
			printf(
			    "%.*s\n",
			    32 * 3 + 4 + 8 + 3,
			    "----------------------------------------------------------"
			    "------------------------------------------"
			);
			printLine(data);
			printLine(data + 32);
			printLine(data + 64);
			printf(
			    "%.*s\n",
			    32 * 3 + 4 + 8 + 3,
			    "----------------------------------------------------------"
			    "------------------------------------------"
			);
		};
		for (size_t i = 0; i < SectorSize * FLASH_SECTOR_SIZE; i += 128) {
			printLine(reinterpret_cast<const uint8_t *>(XIP_BASE + Offset + i));
		}
	}

	    private : static constexpr uint Offset =
		              PICO_FLASH_SIZE_BYTES - SectorSize * FLASH_SECTOR_SIZE;
	};
