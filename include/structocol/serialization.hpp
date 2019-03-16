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
#include <deque>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

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

template <typename T>
struct dynamic_container_serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, const T& val) {
		structocol::serialize(buffer, std::uint64_t{val.size()});
		for(const auto& elem : val) {
			structocol::serialize(buffer, elem);
		}
	}
	template <typename Buff>
	static T deserialize(Buff& buffer) {
		T val;
		auto size = structocol::deserialize<std::uint64_t>(buffer);
		for(std::uint64_t i = 0; i < size; ++i) {
			val.insert(val.end(), structocol::deserialize<typename T::value_type>(buffer));
		}
		return val;
	}
};

template <typename... T>
struct tuple_serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, const std::tuple<T...>& val) {
		std::apply([&buffer](const auto&... elems) { (structocol::serialize(buffer, elems), ...); }, val);
	}
	template <typename Buff>
	static std::tuple<T...> deserialize(Buff& buffer) {
		return deserialize_impl(buffer, std::make_index_sequence<sizeof...(T)>{});
	}

private:
	template <typename Buff, std::size_t... indseq>
	static std::tuple<T...> deserialize_impl(Buff& buffer, std::index_sequence<indseq...>) {
		// Can't simply be this due to sequencing rules:
		// return T{deserialize<boost::pfr::tuple_element_t<indseq, T>>(buffer)...};
		std::tuple<std::optional<T>...> elements;
		// Fold over comma is correctly sequenced left to right:
		(deserialize_element<T, indseq>(buffer, elements), ...);
		return std::tuple(std::move(*std::get<indseq>(elements))...);
	}
	template <typename ElemT, std::size_t index, typename Buff, typename Tup>
	static void deserialize_element(Buff& buffer, Tup& tup) {
		std::get<index>(tup) = structocol::deserialize<ElemT>(buffer);
	}
};

template <typename FT, typename ST>
struct pair_serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, const std::pair<FT, ST>& val) {
		structocol::serialize(buffer, val.first);
		structocol::serialize(buffer, val.second);
	}
	template <typename Buff>
	static std::pair<FT, ST> deserialize(Buff& buffer) {
		auto first = structocol::deserialize<FT>(buffer);
		return std::pair(std::move(first), structocol::deserialize<ST>(buffer));
	}
};

template <typename... T>
struct variant_serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, const std::variant<T...>& val) {
		structocol::serialize(buffer, std::uint64_t{val.index()});
		std::visit([&buffer](const auto& content) { structocol::serialize(buffer, content); }, val);
	}
	template <typename Buff>
	static std::variant<T...> deserialize(Buff& buffer) {
		auto index = structocol::deserialize<std::uint64_t>(buffer);
		deserialze_impl_ptr<Buff> impl_table[] = {make_deserialize_impl<Buff, T>()...};
		return impl_table[index](buffer);
	}

private:
	template <typename Buff>
	using deserialze_impl_ptr = std::variant<T...> (*)(Buff&);
	template <typename Buff, typename Content>
	static deserialze_impl_ptr<Buff> make_deserialize_impl() {
		return [](Buff & buffer) -> std::variant<T...> {
			return structocol::deserialize<Content>(buffer);
		};
	}
};

template <typename T>
struct optional_serializer {
	template <typename Buff>
	static void serialize(Buff& buffer, const std::optional<T>& val) {
		structocol::serialize(buffer, val.has_value());
		if(val.has_value()) {
			structocol::serialize(buffer, *val);
		}
	}
	template <typename Buff>
	static std::optional<T> deserialize(Buff& buffer) {
		auto has_val = structocol::deserialize<bool>(buffer);
		if(has_val) {
			return structocol::deserialize<T>(buffer);
		} else {
			return std::nullopt;
		}
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
		// return T{deserialize<boost::pfr::tuple_element_t<indseq, T>>(buffer)...};
		std::tuple<std::optional<boost::pfr::tuple_element_t<indseq, T>>...> elements;
		// Fold over comma is correctly sequenced left to right:
		(deserialize_element<boost::pfr::tuple_element_t<indseq, T>, indseq>(buffer, elements), ...);
		return T{std::move(*std::get<indseq>(elements))...};
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
template <>
struct serializer<char> : single_byte_serializer<char> {};
#ifdef __cpp_char8_t
struct serializer<char8_t> : single_byte_serializer<char8_t> {};
#endif
template <>
struct serializer<bool> : single_byte_serializer<bool> {};

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
static_assert(sizeof(char16_t) * CHAR_BIT == 16, "(De)Serialization is only supported for architectures "
												 "where char16_t is exactly 16 bits big (not larger).");
template <>
struct serializer<char16_t> : integral_big_endian_serializer<char16_t> {};
static_assert(sizeof(char32_t) * CHAR_BIT == 32, "(De)Serialization is only supported for architectures "
												 "where char32_t is exactly 32 bits big (not larger).");
template <>
struct serializer<char32_t> : integral_big_endian_serializer<char32_t> {};

template <typename T>
struct serializer<std::vector<T>> : dynamic_container_serializer<std::vector<T>> {};
template <typename T>
struct serializer<std::list<T>> : dynamic_container_serializer<std::list<T>> {};
template <typename T>
struct serializer<std::deque<T>> : dynamic_container_serializer<std::deque<T>> {};
template <typename K, typename V>
struct serializer<std::map<K, V>> : dynamic_container_serializer<std::map<K, V>> {};
template <typename T>
struct serializer<std::set<T>> : dynamic_container_serializer<std::set<T>> {};
template <typename K, typename V>
struct serializer<std::multimap<K, V>> : dynamic_container_serializer<std::multimap<K, V>> {};
template <typename T>
struct serializer<std::multiset<T>> : dynamic_container_serializer<std::multiset<T>> {};

template <typename T>
struct serializer<std::basic_string<T>> : dynamic_container_serializer<std::basic_string<T>> {};
template <>
struct serializer<std::wstring> {
	// Serializing wstring is not supported because its encoding is too implementation-defined.
	template <typename Buff>
	static void serialize(Buff& buffer, const std::wstring& val) = delete;
	template <typename Buff>
	static std::wstring deserialize(Buff& buffer) = delete;
};

template <typename FT, typename ST>
struct serializer<std::pair<FT, ST>> : pair_serializer<FT, ST> {};
template <typename... T>
struct serializer<std::tuple<T...>> : tuple_serializer<T...> {};

template <typename... T>
struct serializer<std::variant<T...>> : variant_serializer<T...> {};
template <typename T>
struct serializer<std::optional<T>> : optional_serializer<T> {};

template <>
struct serializer<std::monostate> {
	template <typename Buff>
	static void serialize(Buff&, const std::monostate&) {}
	template <typename Buff>
	static std::monostate deserialize(Buff&) {
		return {};
	}
};

// TODO: variant, optional, floating point
// TODO: Make sizes and index smaller?

template <typename Buff, typename T>
void serialize(Buff& buffer, const T& val) {
	serializer<std::remove_const_t<T>>::serialize(buffer, val);
}
template <typename T, typename Buff>
T deserialize(Buff& buffer) {
	return serializer<std::remove_const_t<T>>::deserialize(buffer);
}

} // namespace structocol

#endif // STRUCTOCOL_SERIALIZATION_INCLUDED
