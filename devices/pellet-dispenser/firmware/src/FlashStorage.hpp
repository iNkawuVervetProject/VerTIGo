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

namespace details {

// compile time FNV-1a
constexpr uint32_t
Hash32_CT(const char *str, uint32_t basis = uint32_t(2166136261UL)) {
	return *str == 0
	           ? basis
	           : Hash32_CT(str + 1, (basis ^ str[0]) * uint32_t(16777619UL));
}

template <typename T> class TypeID {
public:
	constexpr static size_t typeID() {
		return details::Hash32_CT(__PRETTY_FUNCTION__);
	}
};

} // namespace details

struct FlashObjectHeader {
	size_t Hash;
	size_t UniqueCode;
	size_t Size;
};

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

	inline static void Save(const T &obj) {
		printf("Object Hash is %x\n", Hash);
	}

private:
};
