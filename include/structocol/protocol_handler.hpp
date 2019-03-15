/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED
#define STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED

#include "type_utilities.hpp"

namespace structocol {

template <typename... Msgs>
class protocol_handler {
	template <typename Buff, typename HandlerFunc>
	using process_impl_ptr = void (*)(Buff&, HandlerFunc&);
	template <typename Buff, typename HandlerFunc, typename Msg>
	static process_impl_ptr<Buff, HandlerFunc> make_process_impl() {
		return [](Buff& buffer, HandlerFunc& handler) {};
	}

public:
	template <typename Buff, typename Msg>
	void send_message(Buff& buffer, const Msg& msg) {
		constexpr auto type_index = detail::index_of_type_v<Msg, Msgs...>;
	}

	template <typename Buff, typename HandlerFunc>
	void process_message(Buff& buffer, HandlerFunc& handler) {
		impl_ptr impl_table[] = {make_process_impl<Buff, HandlerFunc, Msgs>()...};
	}
};

} // namespace structocol

#endif // STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED
