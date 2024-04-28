#ifndef STRUCTOCOL_BUFFERS_RING_INCLUDED
#define STRUCTOCOL_BUFFERS_RING_INCLUDED

#include "type_utilities.hpp"
#include <deque>
#include <queue>
#include <type_traits>
#include "buffers_pool.hpp"

namespace structocol {

namespace detail {

template <typename T>
class recycling_ring_container : protected std::queue<T> {
public:
	using std::queue<T>::push;
	using std::queue<T>::pop;
	using std::queue<T>::empty;
	using std::queue<T>::size;
	T& current() noexcept {
		return this->front();
	}
	const T& current() const noexcept {
		return this->front();
	}
};

} // namespace detail

template <typename Buffer_Type, typename User_Data_Type = void>
class buffers_ring : public buffers_pool<Buffer_Type, detail::recycling_ring_container, User_Data_Type> {};

} // namespace structocol

#endif // STRUCTOCOL_BUFFERS_RING_INCLUDED
