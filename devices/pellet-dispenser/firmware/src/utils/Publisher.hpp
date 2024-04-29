#pragma once

#include <Error.hpp>
#include <optional>
#include <utility>

template <typename T> class Publisher {
public:
	inline std::pair<std::optional<T>, Error> Value() const {
		return {d_value, d_error};
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
	void Publish(std::optional<T> &&value, Error error) {
		d_value = std::move(value);
		d_error = error;
	}

private:
	std::optional<T> d_value;
	Error            d_error;
};
