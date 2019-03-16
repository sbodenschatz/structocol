/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_SERIALIZATION_INCLUDED
#define STRUCTOCOL_SERIALIZATION_INCLUDED

#include <array>
#include <boost/pfr.hpp>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace structocol {

static_assert(CHAR_BIT == 8, "(De)Serialization is only supported for architectures with 8-bit bytes.");

template <typename T>
struct serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, const T& val) {
		// TODO: Implement
	}
	template <typename Buff>
	static T deserialize(Buff& buffer) {
		// TODO: Implement
		return T{};
	}
};

template <typename T>
struct single_byte_serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, T val) {
		buffer.write(std::array{std::byte(val)});
	}
	template <typename Buff>
	static T deserialize(Buff& buffer) {
		return std::to_integer<T>(buffer.read<1>().front());
	}
};

template <typename T>
struct integral_big_endian_serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, T val) {
		uint uval = val;
		std::array<std::byte, sizeof(T)> data;
		for(int i = 0; i < sizeof(T); i++) {
			int shift = (sizeof(T) - 1 - i) * CHAR_BIT;
			uint mask = uint(0xFFu) << shift;
			data[i] = std::byte((uval & mask) >> shift);
		}
		buffer.write(data);
	}
	template <typename Buff>
	static T deserialize(Buff& buffer) {
		auto data = buffer.read<sizeof(T)>();
		uint val = 0;
		for(int i = 0; i < sizeof(T); i++) {
			int shift = (sizeof(T) - 1 - i) * CHAR_BIT;
			val |= std::to_integer<uint>(data[i]) << shift;
		}
		if constexpr(std::is_signed_v<T>) {
			return to_signed(val);
		} else {
			return val;
		}
	}

private:
	using uint = std::make_unsigned_t<T>;
	static T to_signed(uint val) {
		// See https://stackoverflow.com/a/13208789
		auto min = std::numeric_limits<T>::min();
		if(val <= static_cast<uint>(min)) return static_cast<T>(val);
		if(val >= static_cast<uint>(min)) return static_cast<T>(val - static_cast<uint>(min)) + min;
		throw std::overflow_error("Value not representable in target type.");
	}
};

template <>
struct serializer<std::uint8_t> : single_byte_serializer<std::uint8_t> {};
template <>
struct serializer<std::int8_t> : single_byte_serializer<std::int8_t> {};

/// Note: Multi-byte integers are serialized in big endian format.

template <>
struct serializer<std::uint16_t> : integral_big_endian_serializer<std::uint16_t> {};
template <>
struct serializer<std::int16_t> : integral_big_endian_serializer<std::int16_t> {};
template <>
struct serializer<std::uint32_t> : integral_big_endian_serializer<std::uint32_t> {};
template <>
struct serializer<std::int32_t> : integral_big_endian_serializer<std::int32_t> {};
template <>
struct serializer<std::uint64_t> : integral_big_endian_serializer<std::uint64_t> {};
template <>
struct serializer<std::int64_t> : integral_big_endian_serializer<std::int64_t> {};

template <typename Buff, typename T>
void serialize(Buff& buffer, const T& val) {
	serializer<T>::serialize(buffer, val);
}
template <typename T, typename Buff>
T deserialize(Buff& buffer) {
	return serializer<T>::deserialize(buffer);
}

} // namespace structocol

#endif // STRUCTOCOL_SERIALIZATION_INCLUDED
