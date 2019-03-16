/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED
#define STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED

#include "serialization.hpp"
#include "type_utilities.hpp"
#include <cstdint>
#include <stdexcept>

namespace structocol {

template <typename... Msgs>
class protocol_handler {
	template <typename Buff, typename HandlerFunc>
	using process_impl_ptr = void (*)(Buff&, HandlerFunc&);
	template <typename Buff, typename HandlerFunc, typename Msg>
	static process_impl_ptr<Buff, HandlerFunc> make_process_impl() {
		return [](Buff& buffer, HandlerFunc& handler) { handler(deserialize<Msg>(buffer)); };
	}

public:
	using type_index_t = std::uint64_t;
	template <typename Buff, typename Msg>
	void send_message(Buff& buffer, const Msg& msg) {
		constexpr auto type_index = index_of_type_v<Msg, Msgs...>;
		serialize(buffer, type_index_t{type_index});
		serialize(buffer, msg);
	}

	template <typename Buff, typename HandlerFunc>
	void process_message(Buff& buffer, HandlerFunc& handler) {
		process_impl_ptr<Buff, HandlerFunc> impl_table[] = {make_process_impl<Buff, HandlerFunc, Msgs>()...};
		auto type_index = deserialize<type_index_t>(buffer);
		if(type_index >= sizeof...(Msgs)) {
			throw std::invalid_argument("Invalid message type.");
		}
		impl_table[type_index](buffer, handler);
	}
};

} // namespace structocol

#endif // STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED
