#pragma once

namespace details {

template <typename Lambda> struct Deferrer {
	Lambda lambda;

	~Deferrer() {
		lambda();
	};
};

struct Defer_void {};

template <typename Lambda>
Deferrer<Lambda> operator*(Defer_void, Lambda &&lambda) {
	return {lambda};
}

// some
#define _DEFER_UNIQUE_NAME_INNER(a, b)   a##b
#define _DEFER_UNIQUE_NAME(base, line)   fu_DEFER_UNIQUE_NAME_INNER(base, line)
#define _DEFER_NAME                      fu_DEFER_UNIQUE_NAME(zz_defer, __LINE__)
#define defer                            auto fu_DEFER_NAME = details::Defer_void{} *[&]()

} // namespace details
