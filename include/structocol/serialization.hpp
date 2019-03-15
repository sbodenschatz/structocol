/*
 * Structocol project
 *
 * Copyright 2019
 */

#ifndef STRUCTOCOL_SERIALIZATION_INCLUDED
#define STRUCTOCOL_SERIALIZATION_INCLUDED

#include <cstddef>
#include <cstdint>

namespace structocol {

template <typename T>
struct serializer {
	template <typename Buff>
	void serialize(Buff& buffer, const T& val) {
		// TODO: Implement
	}
	template <typename Buff>
	T deserialize(Buff& buffer) {
		// TODO: Implement
	}
};

template <>
struct serializer<std::uint8_t> {
	template <typename Buff>
	void serialize(Buff& buffer, std::uint8_t val) {
		buffer.write({std::byte(val)});
	}
	template <typename Buff>
	std::uint8_t deserialize(Buff& buffer) {
		return std::to_integer<std::uint8_t>(buffer.read<1>().front());
	}
};

template <>
struct serializer<std::int8_t> {
	template <typename Buff>
	void serialize(Buff& buffer, std::int8_t val) {
		buffer.write({std::byte(val)});
	}
	template <typename Buff>
	std::int8_t deserialize(Buff& buffer) {
		return std::to_integer<std::int8_t>(buffer.read<1>().front());
	}
};

/// Note: Multi-byte integers are serialized in big endian format.

template <>
struct serializer<std::uint16_t> {
	template <typename Buff>
	void serialize(Buff& buffer, std::uint16_t val) {
		buffer.write({std::byte((val & 0xFF00) >> 8), std::byte(val & 0x00FF)});
	}
	template <typename Buff>
	std::uint16_t deserialize(Buff& buffer) {
		auto data = buffer.read<2>();
		return (std::to_integer<std::uint16_t>(data[0]) << 8) | (std::to_integer<std::uint16_t>(data[1]));
	}
};

template <>
struct serializer<std::int16_t> {
	template <typename Buff>
	void serialize(Buff& buffer, std::int16_t val) {
		buffer.write({std::byte((val & 0xFF00) >> 8), std::byte(val & 0x00FF)});
	}
	template <typename Buff>
	std::int16_t deserialize(Buff& buffer) {
		auto data = buffer.read<2>();
		return (std::to_integer<std::int16_t>(data[0]) << 8) | (std::to_integer<std::int16_t>(data[1]));
	}
};

} // namespace structocol

#endif // STRUCTOCOL_SERIALIZATION_INCLUDED
