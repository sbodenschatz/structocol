/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED
#define STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED

#include "exceptions.hpp"
#include <cstdint>
#include <structocol/serialization.hpp>
#include <structocol/type_utilities.hpp>
#include <variant>

namespace structocol {

template <typename... Msgs>
class protocol_handler {
	template <typename Buff, typename HandlerFunc>
	using process_impl_ptr = void (*)(Buff&, HandlerFunc&&);
	template <typename Buff, typename HandlerFunc, typename Msg>
	static process_impl_ptr<Buff, HandlerFunc> make_process_impl() {
		return [](Buff& buffer, HandlerFunc&& handler) { handler(deserialize<Msg>(buffer)); };
	}

	template <typename Buff>
	using decode_impl_ptr = std::variant<Msgs...> (*)(Buff&);
	template <typename Buff, typename Msg>
	static decode_impl_ptr<Buff> make_decode_impl() {
		return [](Buff & buffer) -> std::variant<Msgs...> {
			return deserialize<Msg>(buffer);
		};
	}

public:
	using type_index_t = sufficient_uint_t<sizeof...(Msgs)>;
	using any_message_t = std::variant<Msgs...>;
	template <typename Buff, typename Msg>
	static void encode_message(Buff& buffer, const Msg& msg) {
		constexpr auto type_index = index_of_type_v<Msg, Msgs...>;
		serialize(buffer, type_index_t{type_index});
		serialize(buffer, msg);
	}

	template <typename Msg>
	static std::size_t calculate_message_size(const Msg& msg) {
		constexpr auto type_index = index_of_type_v<Msg, Msgs...>;
		return serialized_size(type_index_t{type_index}) + serialized_size(msg);
	}

	template <typename Buff>
	static any_message_t decode_message(Buff& buffer) {
		decode_impl_ptr<Buff> impl_table[] = {make_decode_impl<Buff, Msgs>()...};
		auto type_index = deserialize<type_index_t>(buffer);
		if(type_index >= sizeof...(Msgs)) {
			throw deserialization_data_error("Invalid message type.");
		}
		return impl_table[type_index](buffer);
	}

	template <typename Buff, typename HandlerFunc>
	static void process_message(Buff& buffer, HandlerFunc&& handler) {
		process_impl_ptr<Buff, HandlerFunc> impl_table[] = {make_process_impl<Buff, HandlerFunc, Msgs>()...};
		auto type_index = deserialize<type_index_t>(buffer);
		if(type_index >= sizeof...(Msgs)) {
			throw deserialization_data_error("Invalid message type.");
		}
		impl_table[type_index](buffer, std::forward<HandlerFunc>(handler));
	}
};

} // namespace structocol

#endif // STRUCTOCOL_PROTOCOL_HANDLER_INCLUDED
