#pragma once

#include "hardware/sync.h"
#include "pico/lock_core.h"
#include <array>
#include <type_traits>

template <typename T, size_t N, std::enable_if_t<N >= 1> * = nullptr>
class Queue {
public:
	inline Queue() {
		lock_init(&q->core, next_striped_spin_lock_num());
	};

	inline size_t Size() const {
		uint32_t save = spin_lock_blocking(q->core.spin_lock);
		size_t   size = size();
		spin_unlock(q->core.spin_lock, save);
		return size;
	}

	inline

	    inline bool
	    Add(const T &obj) {
		return add(obj, false);
	}

	inline void AddBlocking(const T &obj) {
		add(obj, true);
	}

	inline bool Remove(T &obj) {
		return remove(obj, false);
	}

	inline void RemoveBlocking(T &obj) {
		remove(obj, true);
	}

private:
	lock_core_t d_core;

	std::array<T, N> d_data;
	uint16_t         d_writePtr = 0;
	uint16_t         d_readPtr  = 0;
	uint16_t         d_count    = 0;
};
