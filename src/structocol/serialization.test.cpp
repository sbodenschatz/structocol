#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
#include <structocol/serialization.hpp>
#include <structocol/vector_buffer.hpp>

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
