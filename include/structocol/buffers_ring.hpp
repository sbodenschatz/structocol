#ifndef STRUCTOCOL_BUFFERS_RING_INCLUDED
#define STRUCTOCOL_BUFFERS_RING_INCLUDED

#include <deque>

namespace structocol {

namespace detail {
template <typename T>
static std::enable_if_t<has_clear_member_v<T>> clear_user_data(T& user_data) {
	user_data.clear();
}
template <typename T>
static std::enable_if_t<!has_clear_member_v<T>> clear_user_data(T& user_data) {
	user_data = {};
}

template <typename Buffer_Type, typename User_Data_T>
struct buffers_ring_element {
	Buffer_Type buffer;
	User_Data_T user_data;
	void clear() {
		buffer.clear();
		clear_user_data(user_data);
	}
};

template <typename Buffer_Type>
struct buffers_ring_element<Buffer_Type, void> {
	Buffer_Type buffer;
	void clear() {
		buffer.clear();
	}
};

} // namespace detail

template <typename Buffer_Type, typename User_Data_Type = void>
class buffers_ring {

public:
	using element_type = detail::buffers_ring_element<Buffer_Type, User_Data_Type>;

	element_type& obtain_back() {
		if(recycle_buffers.empty()) {
			active_buffers.emplace_back();
		} else {
			active_buffers.push_back(std::move(recycle_buffers.front()));
			recycle_buffers.pop_front();
		}
		return active_buffers.back();
	}
	void recycle_front() {
		if(!active_buffers.empty()) {
			active_buffers.front().clear();
			recycle_buffers.push_back(std::move(active_buffers.front()));
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

private:
	std::deque<element_type> active_buffers;
	std::deque<element_type> recycle_buffers;
};

} // namespace structocol

#endif // STRUCTOCOL_BUFFERS_RING_INCLUDED
