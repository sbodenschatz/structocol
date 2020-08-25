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
void init_primitive(float& v) {
	v = 123.456f;
}
void init_primitive(double& v) {
	v = 23456.67891;
}
void init_primitive(long double& v) {
	v = 345678.912345;
}
} // namespace

TEMPLATE_TEST_CASE("serialization size of integral types is calculated correctly", "[serialization_size]", std::uint8_t,
				   std::int8_t, std::uint16_t, std::int16_t, std::uint32_t, std::int32_t, std::uint64_t, std::int64_t) {
	structocol::vector_buffer vb;
	TestType inval;
	init_primitive(inval);
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
}

TEMPLATE_TEST_CASE("serialization size of floating point types is calculated correctly", "[serialization_size]", float,
				   double) {
	structocol::vector_buffer vb;
	TestType inval;
	init_primitive(inval);
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
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

TEMPLATE_TEST_CASE("serialization size of aggregate types is calculated correctly", "[serialization_size]", test_a,
				   test_b) {
	structocol::vector_buffer vb;
	auto inval = init_aggregate<TestType>();
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
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
	m.emplace(FT{123}, ST{127});
	m.emplace(FT{10}, ST{42});
	m.emplace(FT{1}, ST{124});
	m.emplace(FT{100}, ST{20});
	m.emplace(FT{100}, ST{25});
	if constexpr(std::is_signed_v<FT>) {
		m.emplace(FT{-123}, ST{127});
		m.emplace(FT{-10}, ST{42});
		m.emplace(FT{-1}, ST{124});
		m.emplace(FT{-100}, ST{20});
		m.emplace(FT{-100}, ST{25});
	}
	if constexpr(std::is_signed_v<ST>) {
		m.emplace(FT{122}, ST{-127});
		m.emplace(FT{9}, ST{-42});
		m.emplace(FT{2}, ST{-124});
		m.emplace(FT{101}, ST{-20});
		m.emplace(FT{101}, ST{-25});
	}
	if constexpr(std::is_signed_v<FT> && std::is_signed_v<ST>) {
		m.emplace(FT{-122}, ST{-127});
		m.emplace(FT{-9}, ST{-42});
		m.emplace(FT{-2}, ST{-124});
		m.emplace(FT{-101}, ST{-20});
		m.emplace(FT{-101}, ST{-25});
	}
}
} // namespace

TEMPLATE_TEST_CASE("serialization size of sequence containers is calculated correctly", "[serialization_size]",
				   std::vector<std::uint8_t>, std::vector<std::int8_t>, std::vector<std::uint16_t>,
				   std::vector<std::int16_t>, std::vector<std::uint32_t>, std::vector<std::int32_t>,
				   std::vector<std::uint64_t>, std::vector<std::int64_t>, std::list<std::uint8_t>,
				   std::list<std::int8_t>, std::list<std::uint16_t>, std::list<std::int16_t>, std::list<std::uint32_t>,
				   std::list<std::int32_t>, std::list<std::uint64_t>, std::list<std::int64_t>, std::deque<std::uint8_t>,
				   std::deque<std::int8_t>, std::deque<std::uint16_t>, std::deque<std::int16_t>,
				   std::deque<std::uint32_t>, std::deque<std::int32_t>, std::deque<std::uint64_t>,
				   std::deque<std::int64_t>) {
	structocol::vector_buffer vb;
	TestType inval;
	init_vocabulary(inval);
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
}
TEMPLATE_TEST_CASE("serialization size of associative containers is calculated correctly", "[serialization_size]",
				   std::set<std::uint8_t>, std::set<std::int8_t>, std::set<std::uint16_t>, std::set<std::int16_t>,
				   std::set<std::uint32_t>, std::set<std::int32_t>, std::set<std::uint64_t>, std::set<std::int64_t>,
				   std::multiset<std::uint8_t>, std::multiset<std::int8_t>, std::multiset<std::uint16_t>,
				   std::multiset<std::int16_t>, std::multiset<std::uint32_t>, std::multiset<std::int32_t>,
				   std::multiset<std::uint64_t>, std::multiset<std::int64_t>, (std::map<std::uint8_t, std::uint8_t>),
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
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
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
TEMPLATE_TEST_CASE("serialization size of strings is calculated correctly", "[serialization_size]", std::string,
				   std::u16string, std::u32string OPTIONAL_U8STR) {
	structocol::vector_buffer vb;
	TestType inval;
	init_vocabulary(inval);
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
}

namespace {
template <typename T>
T init_sum_type() {
	if constexpr(std::is_same_v<std::nullopt_t, T>) {
		return std::nullopt;
	} else if constexpr(std::is_same_v<std::monostate, T>) {
		return std::monostate{};
	} else if constexpr(std::is_integral_v<T>) {
		T v;
		init_primitive(v);
		return v;
	} else {
		T v;
		init_vocabulary(v);
		return v;
	}
}
} // namespace

TEMPLATE_TEST_CASE("serialization size of variants is calculated correctly", "[serialization_size]", std::monostate,
				   std::string, std::vector<std::uint32_t>, std::int8_t, std::uint8_t, std::int16_t, std::uint16_t,
				   std::int32_t, std::uint32_t, std::int64_t, std::uint64_t) {
	structocol::vector_buffer vb;
	using var_t = std::variant<std::monostate, std::string, std::vector<std::uint32_t>, std::int8_t, std::uint8_t,
							   std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t>;
	var_t inval = init_sum_type<TestType>();
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
}

TEMPLATE_TEST_CASE("serialization size of optionals is calculated correctly", "[serialization_size]", std::string,
				   std::vector<std::uint32_t>, std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t,
				   std::uint32_t, std::int64_t, std::uint64_t) {
	structocol::vector_buffer vb;
	SECTION("with value") {
		std::optional<TestType> inval = init_sum_type<TestType>();
		auto s = structocol::serialized_size(inval);
		structocol::serialize(vb, inval);
		REQUIRE(s == vb.available_bytes());
	}
	SECTION("empty") {
		std::optional<TestType> inval;
		auto s = structocol::serialized_size(inval);
		structocol::serialize(vb, inval);
		REQUIRE(s == vb.available_bytes());
	}
}

TEST_CASE("serialization size of variable length integers is calculated correctly", "[serialization_size]") {
	auto inval = GENERATE(as<std::size_t>{}, 1, 5, 0xF, 42, 0xFF, 0xFFF, 0xFFFF, 0xFFFF'F, 0xFFFF'FF, 0xFFFF'FFF,
						  0xFFFF'FFFF, 0xFFFF'FFFF'F, 0xFFFF'FFFF'FF, 0xFFFF'FFFF'FFF, 0xFFFF'FFFF'FFFF,
						  0xFFFF'FFFF'FFFF'F, 0xFFFF'FFFF'FFFF'FF, 0xFFFF'FFFF'FFFF'FFF, 0xFFFF'FFFF'FFFF'FFFF);
	structocol::vector_buffer vb;
	auto s = structocol::varint_serializer::size(inval);
	structocol::varint_serializer::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
}

TEST_CASE("serialization size of varint_t applies correctly an calculates the correct value.", "[serialization_size]") {
	auto inval = GENERATE(as<structocol::varint_t>{}, 1, 5, 0xF, 42, 0xFF, 0xFFF, 0xFFFF, 0xFFFF'F, 0xFFFF'FF,
						  0xFFFF'FFF, 0xFFFF'FFFF, 0xFFFF'FFFF'F, 0xFFFF'FFFF'FF, 0xFFFF'FFFF'FFF, 0xFFFF'FFFF'FFFF,
						  0xFFFF'FFFF'FFFF'F, 0xFFFF'FFFF'FFFF'FF, 0xFFFF'FFFF'FFFF'FFF, 0xFFFF'FFFF'FFFF'FFFF);
	structocol::vector_buffer vb;
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
}

namespace {
enum class test_enum { a, b, c, d };
} // namespace

TEST_CASE("serialization size of enums (enum class) is calculated correctly", "[serialization_size]") {
	auto inval = GENERATE(test_enum::a, test_enum::b, test_enum::c, test_enum::d);
	structocol::vector_buffer vb;
	auto s = structocol::serialized_size(inval);
	structocol::serialize(vb, inval);
	REQUIRE(s == vb.available_bytes());
}

TEMPLATE_TEST_CASE("serialization size of std::bitset is calculated correctly", "[serialization_size]", std::bitset<0>,
				   std::bitset<1>, std::bitset<2>, std::bitset<4>, std::bitset<8>, std::bitset<16>, std::bitset<32>,
				   std::bitset<64>, std::bitset<128>, std::bitset<256>, std::bitset<512>, std::bitset<1024>,
				   std::bitset<2048>, std::bitset<3>, std::bitset<5>, std::bitset<7>, std::bitset<11>, std::bitset<13>,
				   std::bitset<17>, std::bitset<19>, std::bitset<23>, std::bitset<29>, std::bitset<31>, std::bitset<37>,
				   std::bitset<41>) {
	TestType inval;
	auto s = structocol::serialized_size(inval);
	structocol::vector_buffer vb;
	structocol::serialize(vb, inval);
	CHECK(vb.available_bytes() == s);
}

TEST_CASE("serialization size of magic_number is calculated correctly", "[serialization]") {
	SECTION("for a char sequence") {
		structocol::magic_number<'H', 'e', 'l', 'l', 'o'> inval;
		auto s = structocol::serialized_size(inval);
		structocol::vector_buffer vb;
		structocol::serialize(vb, inval);
		CHECK(vb.available_bytes() == s);
	}
	SECTION("for a heterogeneously-typed number sequence") {
		using magic_number_type = structocol::magic_number<std::uint8_t(42), std::uint16_t(1234), std::uint32_t(987654),
														   std::uint64_t(0xFF00FF00)>;
		magic_number_type inval;
		auto s = structocol::serialized_size(inval);
		structocol::vector_buffer vb;
		structocol::serialize(vb, inval);
		CHECK(vb.available_bytes() == s);
	}
	SECTION("for a char sequence (type-based size function)") {
		structocol::magic_number<'H', 'e', 'l', 'l', 'o'> inval;
		auto s = structocol::serialized_size<structocol::magic_number<'H', 'e', 'l', 'l', 'o'>>();
		structocol::vector_buffer vb;
		structocol::serialize(vb, inval);
		CHECK(vb.available_bytes() == s);
	}
	SECTION("for a heterogeneously-typed number sequence (type-based size function)") {
		using magic_number_type = structocol::magic_number<std::uint8_t(42), std::uint16_t(1234), std::uint32_t(987654),
														   std::uint64_t(0xFF00FF00)>;
		magic_number_type inval;
		auto s = structocol::serialized_size<magic_number_type>();
		structocol::vector_buffer vb;
		structocol::serialize(vb, inval);
		CHECK(vb.available_bytes() == s);
	}
}
