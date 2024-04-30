#pragma once

#include <Error.hpp>
#include <optional>
#include <utility>

template <typename T> class Publisher {
public:
	inline const T &Value() const {
		return d_value.value();
	}

	inline void Clear() {
		d_value = std::nullopt;
		d_error = Error::NO_ERROR;
	}

	inline bool HasValue() const {
		return d_value.has_value();
	}

	inline bool HasError() const {
		return d_error != Error::NO_ERROR;
	}

	inline Error Err() const {
		return d_error;
	}

protected:
	void PublishValue(const T &value) {
		d_value = value;
		d_error = Error::NO_ERROR;
	}

	void PublishError(Error err) {
		d_error = Error::NO_ERROR;
		d_value = std::nullopt;
	}

private:
	std::optional<T> d_value;
	Error            d_error;
};
