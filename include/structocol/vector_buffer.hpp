/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_VECTOR_BUFFER_INCLUDED
#define STRUCTOCOL_VECTOR_BUFFER_INCLUDED

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

namespace structocol {

class vector_buffer {
	std::vector<std::byte> raw_vector_;
	std::size_t read_offset_ = 0;

public:
	template <std::size_t bytes>
	std::array<std::byte, bytes> read() {
		std::array<std::byte, bytes> ret;
		auto after_read_offset = read_offset_ + bytes;
		if(after_read_offset > raw_vector_.size())
			throw std::length_error("Not enough bytes left in buffer.");
		std::copy(raw_vector_.begin() + read_offset_, raw_vector_.begin() + after_read_offset, ret.begin());
		read_offset_ = after_read_offset;
		return ret;
	}

	template <std::size_t bytes>
	std::optional<std::array<std::byte, bytes>> read() {
		std::optional<std::array<std::byte, bytes>> ret = std::array<std::byte, bytes>{};
		auto after_read_offset = read_offset_ + bytes;
		if(after_read_offset > raw_vector_.size()) return std::nullopt;
		std::copy(raw_vector_.begin() + read_offset_, raw_vector_.begin() + after_read_offset, ret->begin());
		read_offset_ = after_read_offset;
		return ret;
	}

	template <std::size_t bytes>
	void write(const std::array<std::byte, bytes>& data) {
		raw_vector_.insert(raw_vector_.end(), data.begin(), data.end());
	}

	const std::vector<std::byte>& raw_vector() const noexcept {
		return raw_vector_;
	}
	std::vector<std::byte>& raw_vector() noexcept {
		return raw_vector_;
	}

	void trim() noexcept {
		raw_vector_.erase(raw_vector_.begin(), raw_vector_.begin() + read_offset_);
		read_offset_ = 0;
	}

	std::size_t capacity() const noexcept {
		return raw_vector_.capacity();
	}

	std::size_t available_bytes() const noexcept {
		return raw_vector_.size() - read_offset_;
	}

	void clear() noexcept {
		raw_vector_.clear();
		read_offset_ = 0;
	}
};

} // namespace structocol

#endif // STRUCTOCOL_VECTOR_BUFFER_INCLUDED
