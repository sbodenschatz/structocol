/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_TYPE_UTILITIES_INCLUDED
#define STRUCTOCOL_TYPE_UTILITIES_INCLUDED

#include <cstddef>

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

} // namespace structocol

#endif // STRUCTOCOL_TYPE_UTILITIES_INCLUDED
