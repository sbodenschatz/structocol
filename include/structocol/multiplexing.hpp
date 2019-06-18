#ifndef STRUCTOCOL_MULTIPLEXING_INCLUDED
#define STRUCTOCOL_MULTIPLEXING_INCLUDED

#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT

#include "serialization.hpp"
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/read.hpp>

namespace structocol {

namespace detail {
template <typename ItermediateCompletionHandler, typename Executor>
struct composed_async_op : ItermediateCompletionHandler {
	Executor executor;
	using ItermediateCompletionHandler::operator();
	using executor_type = Executor;
	auto get_executor() const noexcept {
		return executor;
	}
};

template <typename CompletionHandler, typename Executor>
composed_async_op(CompletionHandler, Executor)
		->composed_async_op<CompletionHandler, Executor>;

} // namespace detail

template <typename LenghtFieldType, typename AsyncReadStream, typename Buffer, typename CompletionToken>
decltype(auto) async_read_multiplexed(AsyncReadStream& stream, Buffer& buffer, CompletionToken&& ct) {
	using signature_type = void(boost::system::error_code ec, std::size_t);
	using result_type = boost::asio::async_result<std::decay_t<CompletionToken>, signature_type>;
	using handler_type = typename result_type::completion_handler_type;
	handler_type handler(std::forward<CompletionToken>(ct));
	result_type res(handler);
	auto executor = boost::asio::get_associated_executor(handler, stream.get_executor());
	boost::asio::async_read(
			stream, buffer.dynamic_view(structocol::serialized_size<LenghtFieldType>()),
			detail::composed_async_op{
					[handler = std::move(handler), &buffer, &stream](boost::system::error_code ec, std::size_t) {
						if(ec)
							handler(ec, 0);
						else {
							auto length = structocol::deserialize<LenghtFieldType>(buffer);
							boost::asio::async_read(stream, buffer.dynamic_view(length), std::move(handler));
						}
					},
					std::move(executor)});
	return res.get();
}

template <typename LenghtFieldType, typename ProtocolHandler, typename AsyncReadStream, typename Buffer,
		  typename Handler, typename ErrorHandler>
void async_process_multiplexed(AsyncReadStream& stream, Buffer& buffer, Handler&& handler,
							   ErrorHandler&& error_handler) {
	auto executor = boost::asio::get_associated_executor(handler, stream.get_executor());
	async_read_multiplexed(
			stream, buffer,
			boost::asio::bind_executor(executor, [&buffer, handler = std::forward<Handler>(handler),
												  error_handler = std::forward<ErrorHandler>(error_handler)](
														 boost::system::error_code ec, std::size_t) {
				if(ec) {
					error_handler(ec);
				} else {
					ProtocolHandler::process_message(buffer, std::move(handler));
				}
			}));
}

template <typename ProtocolHandler, typename LengthFieldType, typename MessageType, typename Buffer>
void encode_message_multiplexed(Buffer& buffer, const MessageType& msg) {
	auto len = ProtocolHandler::calculate_message_size(msg);
	if(len > std::numeric_limits<LengthFieldType>::max()) {
		throw std::length_error("Message too long for given length type.");
	}
	structocol::serialize(buffer, static_cast<LengthFieldType>(len));
	ProtocolHandler::encode_message(buffer, msg);
}

} // namespace structocol

#endif // STRUCTOCOL_ENABLE_ASIO_SUPPORT

#endif // STRUCTOCOL_MULTIPLEXING_INCLUDED
