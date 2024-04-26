#pragma once
#include "hardware/sync.h"
#include "pico/lock_core.h"
#include <array>
#include <cstdint>
#include <type_traits>

template <
    typename T,
    size_t N,
    bool   Threadsafe          = false,
    std::enable_if_t<N >= 1> * = nullptr>
class Queue {
public:
	inline Queue() {}

	inline size_t Size() const {
		auto   save = lock();
		size_t size = d_count;
		unlock(save);
		return size;
	}

	inline bool Empty() const {
		return Size() == 0;
	}

	inline bool Full() const {
		return Size() == N;
	}

	inline bool TryAdd(T &&obj) {
		return add(std::forward<T>(obj), false);
	}

	template <std::enable_if_t<Threadsafe> * = nullptr>
	inline void AddBlocking(T &&obj) {
		add(std::forward<T>(obj), true);
	}

	inline bool TryRemove(T &obj) {
		return remove(obj, false);
	}

	template <std::enable_if_t<Threadsafe> * = nullptr>
	inline void RemoveBlocking(T &obj) {
		remove(obj, true);
	}

private:
	std::conditional_t<Threadsafe, lock_core_t, void> d_core;

	inline uint32_t lock() {
		if constexpr (Threadsafe == true) {
			return spin_lock_blocking(d_core.spin_lock);
		} else {
			return 0;
		}
	}

	inline void unlock(uint32_t saved) {
		if constexpr (Threadsafe == false) {
			return;
		}
		spin_unlock(d_core.spin_lock, saved);
	}

	inline void unlock_wait(uint32_t saved) {
		if constexpr (Threadsafe == false) {
			return;
		}
		lock_internal_spin_unlock_with_wait(&d_core, saved);
	}

	inline void unlock_notify(uint32_t saved) {
		if constexpr (Threadsafe == false) {
			return;
		}
		lock_internal_spin_unlock_with_notify(&d_core, saved);
	}

	inline bool add(T &&obj, bool block) {
		do {
			auto save = lock();
			if (d_count != N) {
				d_data[d_writePtr] = std::forward<T>(obj);
				d_count += 1;
				increment(d_writePtr);
				unlock_notify(save);
				return true;
			}
			if (block) {
				unlock_wait(save);
			} else {
				unlock(save);
				return false;
			}
		} while (true);
	}

	inline bool remove(T &obj, bool block) {
		do {
			auto save = lock();
			if (d_count > 0) {
				obj = std::move(d_data[d_readPtr]);
				increment(d_readPtr);
				d_count -= 1;
				unlock_notify(save);
				return true;
			}
			if (block) {
				unlock_wait(save);
			} else {
				unlock(save);
				return false;
			}
		} while (true);
	}

	inline void increment(uint16_t &value) {
		if (++value >= N) {
			value = 0;
		}
	}

	std::array<T, N> d_data;

	uint16_t         d_writePtr = 0;
	uint16_t         d_readPtr  = 0;
	uint16_t         d_count    = 0;
};
