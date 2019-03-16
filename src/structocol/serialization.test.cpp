#include <algorithm>
#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <structocol/serialization.hpp>
#include <structocol/vector_buffer.hpp>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
void init_primitive(std::uint8_t& v) {
	v = 0xAB;
}
void init_primitive(std::int8_t& v) {
	v = -0x7C;
}
void init_primitive(std::uint16_t& v) {
	v = 0xABCD;
}
void init_primitive(std::int16_t& v) {
	v = -0x7CDE;
}
void init_primitive(std::uint32_t& v) {
	v = 0xABCDEF12;
}
void init_primitive(std::int32_t& v) {
	v = -0x7CDEF123;
}
void init_primitive(std::uint64_t& v) {
	v = 0xABCDEF1234567890;
}
void init_primitive(std::int64_t& v) {
	v = -0x7CDEF123456789AB;
}
} // namespace

TEMPLATE_TEST_CASE("serialization and deserialization of primitive types preserves value", "[serialization]",
				   std::uint8_t, std::int8_t, std::uint16_t, std::int16_t, std::uint32_t, std::int32_t,
				   std::uint64_t, std::int64_t) {
	structocol::vector_buffer vb;
	TestType inval;
	init_primitive(inval);
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
T init_aggregate() {
	if constexpr(std::is_same_v<T, test_a>) {
		return {-0x5678, 0xFEDCBA9876543210, 0x7A};
	} else if constexpr(std::is_same_v<T, test_b>) {
		return {-0x76543210ABCDEF89, init_aggregate<test_a>()};
	} else {
		return {};
	}
}
} // namespace

TEMPLATE_TEST_CASE("serialization and deserialization of aggregate types preserves value", "[serialization]",
				   test_a, test_b) {
	structocol::vector_buffer vb;
	auto inval = init_aggregate<TestType>();
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
}

namespace {
template <typename T, typename VT = typename T::value_type,
		  typename = decltype(std::declval<T>().insert(std::declval<T>().end(), std::declval<VT>())),
		  std::enable_if_t<std::is_integral_v<VT>, int> dummy = 0>
void init_vocabulary(T& seq) {
	std::generate_n(std::inserter(seq, seq.end()), 32, [v = VT{0}]() mutable { return v += 3; });
	if constexpr(std::is_signed_v<VT>) {
		std::generate_n(std::inserter(seq, seq.end()), 32, [v = VT{0}]() mutable { return v -= 3; });
	}
}
template <typename T, typename VT = typename T::value_type,
		  typename = decltype(std::declval<T>().insert(std::declval<T>().end(), std::declval<VT>())),
		  typename FT = typename VT::first_type, typename ST = typename VT::second_type,
		  std::enable_if_t<std::is_same_v<std::pair<FT, ST>, VT> && // map type
								   std::is_integral_v<FT> &&		// with integral key
								   std::is_integral_v<ST>,			// and integral value
						   int>
				  dummy = 0>
void init_vocabulary(T& m) {
	m.emplace(123, 127);
	m.emplace(10, 42);
	m.emplace(1, 124);
	m.emplace(100, 20);
	m.emplace(100, 25);
	if constexpr(std::is_signed_v<FT>) {
		m.emplace(-123, 127);
		m.emplace(-10, 42);
		m.emplace(-1, 124);
		m.emplace(-100, 20);
		m.emplace(-100, 25);
	}
	if constexpr(std::is_signed_v<ST>) {
		m.emplace(122, -127);
		m.emplace(9, -42);
		m.emplace(2, -124);
		m.emplace(101, -20);
		m.emplace(101, -25);
	}
	if constexpr(std::is_signed_v<FT> && std::is_signed_v<ST>) {
		m.emplace(-122, -127);
		m.emplace(-9, -42);
		m.emplace(-2, -124);
		m.emplace(-101, -20);
		m.emplace(-101, -25);
	}
}
} // namespace

TEMPLATE_TEST_CASE("serialization and deserialization of sequence containers preserves value",
				   "[serialization]", std::vector<std::uint8_t>, std::vector<std::int8_t>,
				   std::vector<std::uint16_t>, std::vector<std::int16_t>, std::vector<std::uint32_t>,
				   std::vector<std::int32_t>, std::vector<std::uint64_t>, std::vector<std::int64_t>,
				   std::list<std::uint8_t>, std::list<std::int8_t>, std::list<std::uint16_t>,
				   std::list<std::int16_t>, std::list<std::uint32_t>, std::list<std::int32_t>,
				   std::list<std::uint64_t>, std::list<std::int64_t>, std::deque<std::uint8_t>,
				   std::deque<std::int8_t>, std::deque<std::uint16_t>, std::deque<std::int16_t>,
				   std::deque<std::uint32_t>, std::deque<std::int32_t>, std::deque<std::uint64_t>,
				   std::deque<std::int64_t>) {
	structocol::vector_buffer vb;
	TestType inval;
	init_vocabulary(inval);
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
}
TEMPLATE_TEST_CASE("serialization and deserialization of associative containers preserves value",
				   "[serialization]", std::set<std::uint8_t>, std::set<std::int8_t>, std::set<std::uint16_t>,
				   std::set<std::int16_t>, std::set<std::uint32_t>, std::set<std::int32_t>,
				   std::set<std::uint64_t>, std::set<std::int64_t>, std::multiset<std::uint8_t>,
				   std::multiset<std::int8_t>, std::multiset<std::uint16_t>, std::multiset<std::int16_t>,
				   std::multiset<std::uint32_t>, std::multiset<std::int32_t>, std::multiset<std::uint64_t>,
				   std::multiset<std::int64_t>, (std::map<std::uint8_t, std::uint8_t>),
				   (std::map<std::int8_t, std::uint8_t>), (std::map<std::uint8_t, std::int8_t>),
				   (std::map<std::int8_t, std::int8_t>), (std::map<std::uint16_t, std::uint16_t>),
				   (std::map<std::int16_t, std::uint16_t>), (std::map<std::uint16_t, std::int16_t>),
				   (std::map<std::int16_t, std::int16_t>), (std::map<std::uint32_t, std::uint32_t>),
				   (std::map<std::int32_t, std::uint32_t>), (std::map<std::uint32_t, std::int32_t>),
				   (std::map<std::int32_t, std::int32_t>), (std::map<std::uint64_t, std::uint64_t>),
				   (std::map<std::int64_t, std::uint64_t>), (std::map<std::uint64_t, std::int64_t>),
				   (std::map<std::int64_t, std::int64_t>), (std::multimap<std::uint8_t, std::uint8_t>),
				   (std::multimap<std::int8_t, std::uint8_t>), (std::multimap<std::uint8_t, std::int8_t>),
				   (std::multimap<std::int8_t, std::int8_t>), (std::multimap<std::uint16_t, std::uint16_t>),
				   (std::multimap<std::int16_t, std::uint16_t>), (std::multimap<std::uint16_t, std::int16_t>),
				   (std::multimap<std::int16_t, std::int16_t>), (std::multimap<std::uint32_t, std::uint32_t>),
				   (std::multimap<std::int32_t, std::uint32_t>), (std::multimap<std::uint32_t, std::int32_t>),
				   (std::multimap<std::int32_t, std::int32_t>), (std::multimap<std::uint64_t, std::uint64_t>),
				   (std::multimap<std::int64_t, std::uint64_t>), (std::multimap<std::uint64_t, std::int64_t>),
				   (std::multimap<std::int64_t, std::int64_t>)) {
	structocol::vector_buffer vb;
	TestType inval;
	init_vocabulary(inval);
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
}

namespace {
void init_vocabulary(std::string& s) {
	s = "Hello World!";
}
void init_vocabulary(std::u16string& s) {
	s = u"Hello World!";
}
void init_vocabulary(std::u32string& s) {
	s = U"Hello World!";
}
} // namespace

#ifdef __cpp_char8_t
#define OPTIONAL_U8STR , std::u8string
#else
#define OPTIONAL_U8STR
#endif
TEMPLATE_TEST_CASE("serialization and deserialization of strings preserves value", "[serialization]",
				   std::string, std::u16string, std::u32string OPTIONAL_U8STR) {
	structocol::vector_buffer vb;
	TestType inval;
	init_vocabulary(inval);
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
}
