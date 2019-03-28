/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED
#define STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED

#include <cstdint>
#include <stdexcept>
#include <structocol/serialization.hpp>
#include <structocol/type_utilities.hpp>
#include <variant>

namespace structocol {

template <typename... Msgs>
class protocol_handler {
	template <typename Buff, typename HandlerFunc>
	using process_impl_ptr = void (*)(Buff&, const HandlerFunc&);
	template <typename Buff, typename HandlerFunc, typename Msg>
	static process_impl_ptr<Buff, HandlerFunc> make_process_impl() {
		return [](Buff& buffer, const HandlerFunc& handler) { handler(deserialize<Msg>(buffer)); };
	}

	template <typename Buff>
	using receive_impl_ptr = std::variant<Msgs...> (*)(Buff&);
	template <typename Buff, typename Msg>
	static receive_impl_ptr<Buff> make_receive_impl() {
		return [](Buff & buffer) -> std::variant<Msgs...> {
			return deserialize<Msg>(buffer);
		};
	}

public:
	using type_index_t = sufficient_uint_t<sizeof...(Msgs)>;
	template <typename Buff, typename Msg>
	static void send_message(Buff& buffer, const Msg& msg) {
		constexpr auto type_index = index_of_type_v<Msg, Msgs...>;
		serialize(buffer, type_index_t{type_index});
		serialize(buffer, msg);
	}

	template <typename Buff>
	static std::variant<Msgs...> receive_message(Buff& buffer) {
		receive_impl_ptr<Buff> impl_table[] = {make_receive_impl<Buff, Msgs>()...};
		auto type_index = deserialize<type_index_t>(buffer);
		if(type_index >= sizeof...(Msgs)) {
			throw std::invalid_argument("Invalid message type.");
		}
		return impl_table[type_index](buffer);
	}

	template <typename Buff, typename HandlerFunc>
	static void process_message(Buff& buffer, const HandlerFunc& handler) {
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
