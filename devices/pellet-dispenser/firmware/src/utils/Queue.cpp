#include "Queue.hpp"
#include "pico/lock_core.h"

namespace details {
void lock(lock_core_t *core, uint num) {
	lock_init(core, num);
}
} // namespace details
