#ifndef STRUCTOCOL_BUFFERS_POOL_INCLUDED
#define STRUCTOCOL_BUFFERS_POOL_INCLUDED

#include "type_utilities.hpp"
#include <deque>
#include <type_traits>

namespace structocol {

namespace detail {
template <typename T>
static std::enable_if_t<has_clear_member_v<T>> clear_buffers_pool_user_data(T& user_data) {
	user_data.clear();
}
template <typename T>
static std::enable_if_t<!has_clear_member_v<T>> clear_buffers_pool_user_data(T& user_data) {
	user_data = {};
}

struct buffers_pool_element_no_copy {
	constexpr buffers_pool_element_no_copy() = default;
	constexpr buffers_pool_element_no_copy(const buffers_pool_element_no_copy&) noexcept = delete;
	constexpr buffers_pool_element_no_copy& operator=(const buffers_pool_element_no_copy&) noexcept = delete;
	constexpr buffers_pool_element_no_copy(buffers_pool_element_no_copy&&) noexcept {}
	constexpr buffers_pool_element_no_copy& operator=(buffers_pool_element_no_copy&&) noexcept {
		return *this;
	}
	~buffers_pool_element_no_copy() noexcept = default;
};

template <typename Buffer_Type, typename User_Data_T>
struct buffers_pool_element : buffers_pool_element_no_copy {
	Buffer_Type buffer;
	User_Data_T user_data;
	void clear() {
		buffer.clear();
		clear_buffers_pool_user_data(user_data);
	}
};

template <typename Buffer_Type>
struct buffers_pool_element<Buffer_Type, void> : buffers_pool_element_no_copy {
	Buffer_Type buffer;
	void clear() {
		buffer.clear();
	}
};

} // namespace detail

template <typename Buffer_Type, template<typename> typename Recycling_Container, typename User_Data_Type = void>
class buffers_pool {

public:
	using element_type = detail::buffers_pool_element<Buffer_Type, User_Data_Type>;

	element_type& obtain_back() {
		if(recycle_buffers.empty()) {
			active_buffers.emplace_back();
		} else {
			active_buffers.push_back(std::move(recycle_buffers.current()));
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
	Recycling_Container<element_type> recycle_buffers;
};

} // namespace structocol

#endif // STRUCTOCOL_BUFFERS_POOL_INCLUDED
