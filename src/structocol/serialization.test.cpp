#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
#include <structocol/serialization.hpp>
#include <structocol/vector_buffer.hpp>
#include <tuple>

namespace {
void init(std::uint8_t& v) {
	v = 0xAB;
}
void init(std::int8_t& v) {
	v = -0x7C;
}
void init(std::uint16_t& v) {
	v = 0xABCD;
}
void init(std::int16_t& v) {
	v = -0x7CDE;
}
void init(std::uint32_t& v) {
	v = 0xABCDEF12;
}
void init(std::int32_t& v) {
	v = -0x7CDEF123;
}
void init(std::uint64_t& v) {
	v = 0xABCDEF1234567890;
}
void init(std::int64_t& v) {
	v = -0x7CDEF123456789AB;
}
} // namespace

TEMPLATE_TEST_CASE("serialization and deserialization of primitive types preserves value", "[serialization]",
				   std::uint8_t, std::int8_t, std::uint16_t, std::int16_t, std::uint32_t, std::int32_t,
				   std::uint64_t, std::int64_t) {
	structocol::vector_buffer vb;
	TestType inval;
	init(inval);
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
}

namespace {
struct test_a {
	std::int16_t x;
	std::uint64_t y;
	std::int8_t z;
};

struct test_b {
	std::int64_t x;
	test_a y;
};

bool operator==(const test_a& lhs, const test_a& rhs) noexcept {
	return std::tie(lhs.x, lhs.y, lhs.z) == std::tie(rhs.x, rhs.y, rhs.z);
}
bool operator==(const test_b& lhs, const test_b& rhs) noexcept {
	return std::tie(lhs.x, lhs.y) == std::tie(rhs.x, rhs.y);
}

template <typename T>
T init() {
	if constexpr(std::is_same_v<T, test_a>) {
		return {-0x5678, 0xFEDCBA9876543210, 0x7A};
	} else if constexpr(std::is_same_v<T, test_b>) {
		return {-0x76543210ABCDEF89, init<test_a>()};
	} else {
		return {};
	}
}
} // namespace

TEMPLATE_TEST_CASE("serialization and deserialization of aggregate types preserves value", "[serialization]",
				   test_a, test_b) {
	structocol::vector_buffer vb;
	auto inval = init<TestType>();
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
}
