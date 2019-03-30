/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_STREAM_BUFFER_INCLUDED
#define STRUCTOCOL_STREAM_BUFFER_INCLUDED

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace structocol {

class istream_buffer {
	std::istream& stream_;

public:
	istream_buffer(std::istream& stream) : stream_{stream} {}

	template <std::size_t bytes>
	std::array<std::byte, bytes> read() {
		std::array<std::byte, bytes> buf;
		stream_.read(reinterpret_cast<char*>(buf.data()), buf.size());
		if(!stream_) throw std::runtime_error("Couldn't read the requested amount of bytes.");
		return buf;
	}

	template <std::size_t bytes>
	std::optional<std::array<std::byte, bytes>> try_read() {
		std::array<std::byte, bytes> buf;
		stream_.read(reinterpret_cast<char*>(buf.data()), buf.size());
		if(!stream_) {
			return std::nullopt;
		}
		return buf;
	}
};

class ostream_buffer {
	std::ostream& stream_;

public:
	ostream_buffer(std::ostream& stream) : stream_{stream} {}

	template <std::size_t bytes>
	void write(const std::array<std::byte, bytes>& data) {
		stream_.write(reinterpret_cast<const char*>(data.data()), data.size());
		if (!stream_) throw std::runtime_error("Couldn't write the given amount of bytes.");
	}
};

} // namespace structocol

#endif // STRUCTOCOL_STREAM_BUFFER_INCLUDED
