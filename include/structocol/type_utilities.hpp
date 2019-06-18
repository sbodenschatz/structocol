/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_TYPE_UTILITIES_INCLUDED
#define STRUCTOCOL_TYPE_UTILITIES_INCLUDED

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace structocol {

template <std::size_t index, typename T, typename... Args>
struct nth_type {
	using type = typename nth_type<index - 1, Args...>::type;
};

template <typename T, typename... Args>
struct nth_type<0, T, Args...> {
	using type = T;
};

template <typename T, typename U, typename... Args>
struct index_of_type {
	static constexpr std::size_t value = index_of_type<T, Args...>::value + 1;
};

template <typename T, typename U>
struct index_of_type<T, U> {};

template <typename T, typename... Args>
struct index_of_type<T, T, Args...> {
	static constexpr std::size_t value = 0;
};

template <typename T>
struct index_of_type<T, T> {
	static constexpr std::size_t value = 0;
};

template <typename T, typename... Types>
constexpr std::size_t index_of_type_v = index_of_type<T, Types...>::value;

namespace detail {
template <std::size_t val>
constexpr auto sufficient_uint_helper() {
	if constexpr(val <= std::numeric_limits<std::uint8_t>::max()) {
		return std::uint8_t{};
	} else if constexpr(val <= std::numeric_limits<std::uint16_t>::max()) {
		return std::uint16_t{};
	} else if constexpr(val <= std::numeric_limits<std::uint32_t>::max()) {
		return std::uint32_t{};
	} else {
		return std::uint64_t{};
	}
}

template <std::int64_t val>
constexpr auto sufficient_int_helper() {
	if constexpr(std::numeric_limits<std::int8_t>::min() <= val && val <= std::numeric_limits<std::int8_t>::max()) {
		return std::int8_t{};
	} else if constexpr(std::numeric_limits<std::int16_t>::min() <= val &&
						val <= std::numeric_limits<std::int16_t>::max()) {
		return std::int16_t{};
	} else if constexpr(std::numeric_limits<std::int32_t>::min() <= val &&
						val <= std::numeric_limits<std::int32_t>::max()) {
		return std::int32_t{};
	} else {
		return std::int64_t{};
	}
}
} // namespace detail

template <std::size_t val>
struct sufficient_uint {
	using type = decltype(detail::sufficient_uint_helper<val>());
};
template <std::size_t val>
using sufficient_uint_t = typename sufficient_uint<val>::type;

template <std::int64_t val>
struct sufficient_int {
	using type = decltype(detail::sufficient_int_helper<val>());
};
template <std::int64_t val>
using sufficient_int_t = typename sufficient_int<val>::type;

template <typename,typename = std::void_t<>>
struct has_clear_member : std::false_type {};
template <typename T>
struct has_clear_member<T,std::void_t<decltype(std::declval<T&>().clear())>> : std::true_type {};
template <class T>
inline constexpr bool has_clear_member_v = has_clear_member<T>::value;

} // namespace structocol

#endif // STRUCTOCOL_TYPE_UTILITIES_INCLUDED
