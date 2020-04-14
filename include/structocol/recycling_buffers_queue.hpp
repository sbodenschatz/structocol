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
static std::enable_if_t<has_clear_member_v<T>> clear_recycling_buffers_queue_user_data(T& user_data) {
	user_data.clear();
}
template <typename T>
static std::enable_if_t<!has_clear_member_v<T>> clear_recycling_buffers_queue_user_data(T& user_data) {
	user_data = {};
}

struct recycling_buffers_queue_element_no_copy {
	constexpr recycling_buffers_queue_element_no_copy() = default;
	constexpr recycling_buffers_queue_element_no_copy(const recycling_buffers_queue_element_no_copy&) noexcept = delete;
	constexpr recycling_buffers_queue_element_no_copy& operator=(const recycling_buffers_queue_element_no_copy&) noexcept = delete;
	constexpr recycling_buffers_queue_element_no_copy(recycling_buffers_queue_element_no_copy&&) noexcept {}
	constexpr recycling_buffers_queue_element_no_copy& operator=(recycling_buffers_queue_element_no_copy&&) noexcept {
		return *this;
	}
	~recycling_buffers_queue_element_no_copy() noexcept = default;
};

template <typename Buffer_Type, typename User_Data_T>
struct recycling_buffers_queue_element : recycling_buffers_queue_element_no_copy {
	Buffer_Type buffer;
	User_Data_T user_data;
	void clear() {
		buffer.clear();
		clear_recycling_buffers_queue_user_data(user_data);
	}
};

template <typename Buffer_Type>
struct recycling_buffers_queue_element<Buffer_Type, void> : recycling_buffers_queue_element_no_copy {
	Buffer_Type buffer;
	void clear() {
		buffer.clear();
	}
};

} // namespace detail

template <typename Buffer_Type, typename User_Data_Type = void>
class recycling_buffers_queue {
public:
	using element_type = detail::recycling_buffers_queue_element<Buffer_Type, User_Data_Type>;

	element_type& obtain_back() {
		if(recycle_buffers.empty()) {
			active_buffers.emplace_back();
		} else {
			active_buffers.push_back(std::move(recycle_buffers.top()));
			recycle_buffers.pop();
		}
		return active_buffers.back();
	}
	void recycle_front() {
		if(!active_buffers.empty()) {
			active_buffers.front().clear();
			recycle_buffers.push(std::move(active_buffers.front()));
			active_buffers.pop_front();
		}
	}
	element_type& front() noexcept {
		return active_buffers.front();
	}
	const element_type& front() const noexcept {
		return active_buffers.front();
	}
	bool empty() const noexcept {
		return active_buffers.empty();
	}
	std::size_t size() const noexcept {
		return active_buffers.size();
	}
	std::size_t capacity() const noexcept {
		return active_buffers.size() + recycle_buffers.size();
	}

private:
	std::deque<element_type> active_buffers;
	std::stack<element_type> recycle_buffers;
};

} // namespace structocol

#endif // STRUCTOCOL_RECYCLING_BUFFERS_QUEUE_INCLUDED
