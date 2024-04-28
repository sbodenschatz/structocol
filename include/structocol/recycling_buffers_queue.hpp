/*
 * Structocol project
 *
 * Copyright 2020
 */

#ifndef STRUCTOCOL_RECYCLING_BUFFERS_QUEUE_INCLUDED
#define STRUCTOCOL_RECYCLING_BUFFERS_QUEUE_INCLUDED

#include "type_utilities.hpp"
#include <deque>
#include <stack>
#include <type_traits>

namespace structocol {

namespace detail {

template <typename T>
class recycling_stack_container : protected std::stack<T> {
public:
	using std::stack<T>::push;
	using std::stack<T>::pop;
	using std::stack<T>::empty;
	using std::stack<T>::size;
	T& current() noexcept {
		return this->top();
	}
	const T& current() const noexcept {
		return this->top();
	}
};

} // namespace detail

template <typename Buffer_Type, typename User_Data_Type = void>
class recycling_buffers_queue : public buffers_pool<Buffer_Type, detail::recycling_stack_container,User_Data_Type> {};

} // namespace structocol

#endif // STRUCTOCOL_RECYCLING_BUFFERS_QUEUE_INCLUDED
