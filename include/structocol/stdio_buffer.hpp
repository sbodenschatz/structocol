/*
 * Structocol project
 *
 * Copyright 2020
 */

#ifndef STRUCTOCOL_STDIO_BUFFER_INCLUDED
#define STRUCTOCOL_STDIO_BUFFER_INCLUDED

#include <array>
#include <cstddef>
#include <cstdio>
#include <optional>

namespace structocol {

class stdio_buffer {
	std::FILE* file_handle_;

public:
	stdio_buffer(std::FILE* file_handle) : file_handle_(file_handle) {}

	template <std::size_t bytes>
	std::array<std::byte, bytes> read() {
		return {};
	}

	template <std::size_t bytes>
	std::optional<std::array<std::byte, bytes>> try_read() {
		std::array<std::byte, bytes> buf;
		auto bytes_read = std::fread(reinterpret_cast<char*>(buf.data()), 1, buf.size(), file_handle_);
		if(bytes_read != bytes) {
			return std::nullopt;
		}
		return buf;
	}

	template <std::size_t bytes>
	void write(const std::array<std::byte, bytes>& data) {
		static_cast<void>(data);
	}
};

} // namespace structocol

#endif // STRUCTOCOL_STDIO_BUFFER_INCLUDED
