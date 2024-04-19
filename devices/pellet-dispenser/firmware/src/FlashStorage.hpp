#pragma once

#include "hardware/flash.h"
#include <cstdio>
#include <type_traits>
#include <typeinfo>

#ifndef FLASH_STORAGE_NUMBER_OF_SECTORS
#define FLASH_STORAGE_NUMBER_OF_SECTORS 1
#endif

#ifndef FLASH_STORAGE_NUMBER_OF_PAGES
#define FLASH_STORAGE_NUMBER_OF_PAGES 1
#endif

#define FLASH_PAGES_PER_SECTOR (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)

static_assert(
    (FLASH_STORAGE_NUMBER_OF_PAGES <= FLASH_PAGES_PER_SECTOR) &&
        ((FLASH_PAGES_PER_SECTOR % FLASH_STORAGE_NUMBER_OF_PAGES) == 0),
    "Invalid page size"
);

// Utils to comppute unique hash for a class
namespace details {

// compile time FNV-1a
constexpr uint32_t
Hash32_CT(const char *str, uint32_t basis = uint32_t(2166136261UL)) {
	return *str == 0
	           ? basis
	           : Hash32_CT(str + 1, (basis ^ str[0]) * uint32_t(16777619UL));
}

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

template <typename T> class TypeID {
public:
	constexpr static size_t typeID() {
		return details::Hash32_CT(__PRETTY_FUNCTION__);
	}
};

} // namespace details

struct FlashObjectHeader {
	uint32_t UniqueCode;
	uint16_t Hash;
	uint16_t Size;
};

#ifdef FLASH_STORAGE_UUID
static constexpr int32_t FlashStorageUUID = FLASH_STORAGE_UUID;
#else
static constexpr uint32_t FlashStorageUUID = details::stol(__TIME__);
#endif

template <class T> class FlashStorage {
public:
	using Type                   = std::remove_cv_t<std::remove_reference_t<T>>;
	constexpr static size_t Hash = details::TypeID<Type>::typeID();

	static constexpr size_t MaxObjectSize =
	    FLASH_STORAGE_NUMBER_OF_PAGES * FLASH_PAGE_SIZE -
	    sizeof(FlashObjectHeader);

	static_assert(
	    sizeof(T) < MaxObjectSize,
	    "Maximal Allowed Object Size exceeded, try Increase "
	    "FLASH_STORAGE_NUMBER_OF_PAGES"
	);

	inline static bool Load(T &obj) {
		return false;
	}

	inline static void Save(const T &obj) {}

private:
};
