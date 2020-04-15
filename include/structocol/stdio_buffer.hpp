/*
 * Structocol project
 *
 * Copyright 2020
 */

#ifndef STRUCTOCOL_STDIO_BUFFER_INCLUDED
#define STRUCTOCOL_STDIO_BUFFER_INCLUDED

#include "exceptions.hpp"
#include <array>
#include <cstddef>
#include <cstdio>
#include <optional>
#include <stdexcept>

namespace structocol {

class stdio_buffer {
	std::FILE* file_handle_;

public:
	explicit stdio_buffer(std::FILE* file_handle) : file_handle_(file_handle) {}

	template <std::size_t bytes>
	std::array<std::byte, bytes> read() {
		std::array<std::byte, bytes> buf;
		auto bytes_read = std::fread(reinterpret_cast<char*>(buf.data()), 1, buf.size(), file_handle_);
		if(bytes_read != bytes) {
			if(std::ferror(file_handle_)) {
				throw io_error("IO error while reading the requested bytes.");
				// The caller can still pull the exact error out of errno if they want to, but because std::strerror is
				// not thread-safe, this function doesn't do that itself because it doesn't know if other threads are
				// also calling std::strerror and what the locking strategy used by the calling application is.
			} else if(std::feof(file_handle_)) {
				throw io_error("Not enough bytes left to read in the file represented by the handle.");
			} else {
				throw io_error("fread() read less bytes than requested, but no error or EOF indication was set.");
			}
		}
		return buf;
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
		auto bytes_written = fwrite(reinterpret_cast<const char*>(data.data()), 1, data.size(), file_handle_);
		if(bytes_written != bytes) {
			if(std::ferror(file_handle_)) {
				throw io_error("IO error while writing the supplied bytes.");
				// The caller can still pull the exact error out of errno if they want to, but because std::strerror is
				// not thread-safe, this function doesn't do that itself because it doesn't know if other threads are
				// also calling std::strerror and what the locking strategy used by the calling application is.
			} else {
				throw io_error("fwrite() wrote less bytes than requested, but no error indication was set.");
			}
		}
	}
};

} // namespace structocol

#endif // STRUCTOCOL_STDIO_BUFFER_INCLUDED
