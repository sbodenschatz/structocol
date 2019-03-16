/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_SERIALIZATION_INCLUDED
#define STRUCTOCOL_SERIALIZATION_INCLUDED

#include <array>
#include <boost/pfr/precise/core.hpp>
#include <boost/pfr/precise/tuple_size.hpp>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace structocol {

static_assert(CHAR_BIT == 8, "(De)Serialization is only supported for architectures with 8-bit bytes.");

template <typename Buff, typename T>
void serialize(Buff& buffer, const T& val);
template <typename T, typename Buff>
T deserialize(Buff& buffer);

template <typename T>
struct single_byte_serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, T val) {
		buffer.write(std::array{std::byte(val)});
	}
	template <typename Buff>
	static T deserialize(Buff& buffer) {
		return std::to_integer<T>(buffer.template read<1>().front());
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
		auto data = buffer.template read<sizeof(T)>();
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

namespace detail {
template <typename T, typename Enable = void>
struct general_serializer {
	static_assert(sizeof(T) == -1, // Always false but depends on T
				  "The requested type is not supported for serialization out of the box. If its "
				  "serialization is desired specialize structocol::serializer<T> for it.");
};

template <typename T>
struct general_serializer<T, std::enable_if_t<std::is_aggregate_v<T>>> {
	template <typename Buff>
	static void serialize(Buff& buffer, const T& val) {
		boost::pfr::for_each_field(val,
								   [&buffer](const auto& field) { structocol::serialize(buffer, field); });
	}
	template <typename Buff>
	static T deserialize(Buff& buffer) {
		return deserialize_impl(buffer, std::make_index_sequence<boost::pfr::tuple_size_v<T>>{});
	}

private:
	template <typename Buff, std::size_t... indseq>
	static T deserialize_impl(Buff& buffer, std::index_sequence<indseq...>) {
		// Can't simply be this due to sequencing rules:
		// return T{deserialize<boost::pfr::tuple_element_t<indseq, T>>(buffer)};
		std::tuple<std::optional<boost::pfr::tuple_element_t<indseq, T>>...> elements;
		// Fold over comma is correctly sequenced left to right:
		(deserialize_element<boost::pfr::tuple_element_t<indseq, T>, indseq>(buffer, elements), ...);
		return T{*std::get<indseq>(elements)...};
	}
	template <typename ElemT, std::size_t index, typename Buff, typename Tup>
	static void deserialize_element(Buff& buffer, Tup& tup) {
		std::get<index>(tup) = structocol::deserialize<ElemT>(buffer);
	}
};

// TODO: Specialized for other categories like enums.

} // namespace detail

template <typename T>
struct serializer : detail::general_serializer<T> {};

template <typename T>
struct serializer<T*> {
	static_assert(sizeof(T) == -1, // Always false but depends on T
				  "Pointer types are intentionally excluded from serialization because they would be "
				  "invalid if deserialized in a different process, etc.");
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
