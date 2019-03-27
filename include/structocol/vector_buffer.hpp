/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_VECTOR_BUFFER_INCLUDED
#define STRUCTOCOL_VECTOR_BUFFER_INCLUDED

#include <algorithm>
#include <array>
#include <cassert>
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
#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT
	std::size_t size_ = 0;
#endif

public:
	template <std::size_t bytes>
	std::array<std::byte, bytes> read() {
		std::array<std::byte, bytes> ret;
		if(bytes > available_bytes()) throw std::length_error("Not enough bytes left in buffer.");
		auto start = raw_vector_.begin() + read_offset_;
		std::copy(start, start + bytes, ret.begin());
		read_offset_ += bytes;
		return ret;
	}

	template <std::size_t bytes>
	std::optional<std::array<std::byte, bytes>> try_read() {
		std::optional<std::array<std::byte, bytes>> ret = std::array<std::byte, bytes>{};
		if(bytes > available_bytes()) return std::nullopt;
		auto start = raw_vector_.begin() + read_offset_;
		std::copy(start, start + bytes, ret->begin());
		read_offset_ += bytes;
		return ret;
	}

	template <std::size_t bytes>
	void write(const std::array<std::byte, bytes>& data) {
#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT
		assert(raw_vector_.size() == size_ &&
			   "write MUST NOT be called when there are prepare()d but not commit()ed writes.");
		raw_vector_.insert(raw_vector_.end(), data.begin(), data.end());
		size_ = raw_vector_.size();
#else
		raw_vector_.insert(raw_vector_.end(), data.begin(), data.end());
#endif
	}

	const std::vector<std::byte>& raw_vector() const noexcept {
		return raw_vector_;
	}
	std::vector<std::byte>& raw_vector() noexcept {
		return raw_vector_;
	}

	void trim() noexcept {
		raw_vector_.erase(raw_vector_.begin(), raw_vector_.begin() + read_offset_);
#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT
		size_ -= read_offset_;
#endif
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
#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT
		return size_ - read_offset_;
#else
		return raw_vector_.size() - read_offset_;
#endif
	}

	void clear() noexcept {
		raw_vector_.clear();
		read_offset_ = 0;
#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT
		size_ = 0;
#endif
	}

#ifdef STRUCTOCOL_ENABLE_ASIO_SUPPORT
	using const_buffers_type = decltype(
			boost::asio::dynamic_buffer(std::declval<std::vector<std::byte>&>()))::const_buffers_type;
	using mutable_buffers_type = decltype(
			boost::asio::dynamic_buffer(std::declval<std::vector<std::byte>&>()))::mutable_buffers_type;

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
		return const_buffers_type(
				boost::asio::buffer(raw_vector_.data() + read_offset_, raw_vector_.size() - read_offset_));
	}

	void consume(std::size_t n) {
		read_offset_ = std::min(read_offset_ + n, raw_vector_.size());
	}

	mutable_buffers_type prepare(std::size_t n) {
		raw_vector_.resize(size_ + n);
		return boost::asio::buffer(boost::asio::buffer(raw_vector_) + size_, n);
	}

	void commit(std::size_t n) {
		size_ += std::min(n, raw_vector_.size() - size_);
		raw_vector_.resize(size_);
	}
#endif
};

} // namespace structocol

#endif // STRUCTOCOL_VECTOR_BUFFER_INCLUDED
