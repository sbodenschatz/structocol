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

#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT
#include <boost/asio/buffer.hpp>
#endif

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
	std::optional<std::array<std::byte, bytes>> try_read() {
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

	std::size_t total_capacity() const noexcept {
		return raw_vector_.capacity();
	}

	std::size_t writable_capacity() const noexcept {
		return raw_vector_.capacity() - raw_vector_.size();
	}

	void reserve(std::size_t writable_capacity) {
		raw_vector_.reserve(raw_vector_.size() + writable_capacity);
	}

	std::size_t available_bytes() const noexcept {
		return raw_vector_.size() - read_offset_;
	}

	void clear() noexcept {
		raw_vector_.clear();
		read_offset_ = 0;
	}

#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT
	using const_buffers_type = decltype(
			boost::asio::dynamic_buffer(std::declval<std::vector<std::byte>&>()))::const_buffers_type;

	std::size_t size() const {
		return available_bytes();
	}

	std::size_t max_size() const {
		return raw_vector_.max_size();
	}

	std::size_t capacity() const {
		return total_capacity();
	}

	const_buffers_type data() const {
		return boost::asio::buffer(raw_vector_.data() + read_offset_, raw_vector_.size() - read_offset_);
	}

	void consume(std::size_t n) {
		read_offset_ = std::min(read_offset_+n,raw_vector_.size());
	}

#endif
};

} // namespace structocol

#endif // STRUCTOCOL_VECTOR_BUFFER_INCLUDED
