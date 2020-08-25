#include <algorithm>
#include <catch2/catch.hpp>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <structocol/exceptions.hpp>
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
template <int mantissa_len, int exponent_len, int exponent_bias, typename T,
		  std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
constexpr std::array<std::byte, sizeof(T)> fp_big_endian_bytes(T v) {
	// This function generates the expected big endian bytes for a given floating point value without using
	// reinterpret_cast, memcpy or similar mechanisms that depend on the native endianess. Instead, the bits are
	// calculated purely using floating point arithmetic and toggling the corresponding bits in software.
	// This is done to validate the architecture-dependent endianess code against an architecture-independently
	// generated expected value. It is expressly not designed for actual usage outside the test code.
	static_assert(exponent_len + mantissa_len + 1 == sizeof(T) * CHAR_BIT);
	static_assert(FLT_RADIX == 2);
	int exponent = std::ilogb(v);
	T significand = std::scalbn(v, -exponent);
	unsigned char sign = (significand <= T(-0.0)) ? 1 : 0;
	exponent += exponent_bias;
	T mantissa = std::fabs(significand) - 1;
	std::uint_least64_t mantissa_bits = 0;
	for(int i = 0; i < mantissa_len; ++i) {
		mantissa_bits <<= 1;
		if(mantissa >= T(0.5)) {
			mantissa -= T(0.5);
			mantissa_bits |= 1;
		}
		mantissa *= 2;
	}
	// We calculated the components (sign, exponent, mantissa_bits), now encode them in big endian IEEE format.
	std::array<std::byte, sizeof(T)> res{};
	int byte_index = 0;
	int bit_index = 7;
	res[byte_index] = std::byte(sign << bit_index);
	bit_index--;
	int exp_bits_left = exponent_len;
	while(exp_bits_left) {
		int num_bits_now = std::min(exp_bits_left, bit_index + 1);
		res[byte_index] |= std::byte(((exponent >> (exp_bits_left - num_bits_now)) & ((1 << num_bits_now) - 1))
									 << (bit_index - num_bits_now + 1));
		exp_bits_left -= num_bits_now;
		bit_index -= num_bits_now;
		if(bit_index < 0) {
			bit_index = 7;
			byte_index++;
		}
	}
	int bits_left = mantissa_len;
	if(bit_index < 7) {
		int num_bits_now = std::min(bits_left, bit_index + 1);
		res[byte_index] |= std::byte(((mantissa_bits >> (bits_left - num_bits_now)) & ((1 << num_bits_now) - 1))
									 << (bit_index - num_bits_now + 1));
		bits_left -= num_bits_now;
		bit_index -= num_bits_now;
		if(bit_index < 0) {
			bit_index = 7;
			byte_index++;
		}
	}
	while(bits_left) {
		int num_bits_now = 8;
		res[byte_index] |= std::byte((mantissa_bits >> (bits_left - num_bits_now)) & 0xFF);
		bits_left -= num_bits_now;
		byte_index++;
	}

	return res;
}
void init_primitive(float& v, std::array<std::byte, sizeof(float)>& expected, std::size_t index) {
	constexpr float values[] = {123.456f, 0.00001f, -123.456f, 12300000.0001f, 1e20f, -420000000.0000234f};
	v = values[index];
	expected = fp_big_endian_bytes<23, 8, 127>(values[index]);
}
void init_primitive(double& v, std::array<std::byte, sizeof(double)>& expected, std::size_t index) {
	constexpr double values[] = {23456.67891, -456.6789123, 0.0000000000001, 1e200, -42e42, -12345678900000.321};
	v = values[index];
	expected = fp_big_endian_bytes<52, 11, 1023>(values[index]);
}

} // namespace

TEMPLATE_TEST_CASE("serialization and deserialization of integral types preserves value", "[serialization]",
				   std::uint8_t, std::int8_t, std::uint16_t, std::int16_t, std::uint32_t, std::int32_t, std::uint64_t,
				   std::int64_t) {
	structocol::vector_buffer vb;
	TestType inval;
	init_primitive(inval);
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
}

TEMPLATE_TEST_CASE("serialization and deserialization of floating point types preserves value and produces the "
				   "expected bytes (big endian encoding)",
				   "[serialization]", float, double) {
	structocol::vector_buffer vb;
	TestType inval;
	std::array<std::byte, sizeof(TestType)> expected_data{};
	SECTION("Test value 0") {
		init_primitive(inval, expected_data, 0);
	}
	SECTION("Test value 1") {
		init_primitive(inval, expected_data, 1);
	}
	SECTION("Test value 2") {
		init_primitive(inval, expected_data, 2);
	}
	SECTION("Test value 3") {
		init_primitive(inval, expected_data, 3);
	}
	SECTION("Test value 4") {
		init_primitive(inval, expected_data, 4);
	}
	SECTION("Test value 5") {
		init_primitive(inval, expected_data, 5);
	}
	structocol::serialize(vb, inval);
	CHECK(std::equal(vb.raw_vector().begin(), vb.raw_vector().end(), expected_data.begin(), expected_data.end(),
					 [](const std::byte& a, const std::byte& b) {
						 return static_cast<unsigned char>(a) == static_cast<unsigned char>(b);
					 }));
	auto outval = structocol::deserialize<TestType>(vb);
	CHECK(inval == Approx(outval));
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

TEMPLATE_TEST_CASE("serialization and deserialization of aggregate types preserves value", "[serialization]", test_a,
				   test_b) {
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

TEMPLATE_TEST_CASE("serialization and deserialization of sequence containers preserves value", "[serialization]",
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
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
}
TEMPLATE_TEST_CASE("serialization and deserialization of associative containers preserves value", "[serialization]",
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
TEMPLATE_TEST_CASE("serialization and deserialization of strings preserves value", "[serialization]", std::string,
				   std::u16string, std::u32string OPTIONAL_U8STR) {
	structocol::vector_buffer vb;
	TestType inval;
	init_vocabulary(inval);
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<TestType>(vb);
	REQUIRE(inval == outval);
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

TEMPLATE_TEST_CASE("serialization and deserialization of variants preserves value", "[serialization]", std::monostate,
				   std::string, std::vector<std::uint32_t>, std::int8_t, std::uint8_t, std::int16_t, std::uint16_t,
				   std::int32_t, std::uint32_t, std::int64_t, std::uint64_t) {
	structocol::vector_buffer vb;
	using var_t = std::variant<std::monostate, std::string, std::vector<std::uint32_t>, std::int8_t, std::uint8_t,
							   std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t>;
	var_t inval = init_sum_type<TestType>();
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<var_t>(vb);
	REQUIRE(inval == outval);
}

TEMPLATE_TEST_CASE("serialization and deserialization of optionals preserves value", "[serialization]", std::string,
				   std::vector<std::uint32_t>, std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t,
				   std::uint32_t, std::int64_t, std::uint64_t) {
	structocol::vector_buffer vb;
	SECTION("with value") {
		std::optional<TestType> inval = init_sum_type<TestType>();
		structocol::serialize(vb, inval);
		auto outval = structocol::deserialize<std::optional<TestType>>(vb);
		REQUIRE(inval == outval);
	}
	SECTION("empty") {
		std::optional<TestType> inval;
		structocol::serialize(vb, inval);
		auto outval = structocol::deserialize<std::optional<TestType>>(vb);
		REQUIRE(inval == outval);
	}
}

TEST_CASE("serialization and deserialization of variable length integers preserves value", "[serialization]") {
	auto inval = GENERATE(as<std::size_t>{}, 1, 5, 0xF, 42, 0xFF, 0xFFF, 0xFFFF, 0xFFFF'F, 0xFFFF'FF, 0xFFFF'FFF,
						  0xFFFF'FFFF, 0xFFFF'FFFF'F, 0xFFFF'FFFF'FF, 0xFFFF'FFFF'FFF, 0xFFFF'FFFF'FFFF,
						  0xFFFF'FFFF'FFFF'F, 0xFFFF'FFFF'FFFF'FF, 0xFFFF'FFFF'FFFF'FFF, 0xFFFF'FFFF'FFFF'FFFF);
	structocol::vector_buffer vb;
	structocol::varint_serializer::serialize(vb, inval);
	auto outval = structocol::varint_serializer::deserialize(vb);
	REQUIRE(inval == outval);
}

TEST_CASE("serialization of varint_t applies correctly and preserves value.", "[serialization]") {
	auto inval = GENERATE(as<structocol::varint_t>{}, 1, 5, 0xF, 42, 0xFF, 0xFFF, 0xFFFF, 0xFFFF'F, 0xFFFF'FF,
						  0xFFFF'FFF, 0xFFFF'FFFF, 0xFFFF'FFFF'F, 0xFFFF'FFFF'FF, 0xFFFF'FFFF'FFF, 0xFFFF'FFFF'FFFF,
						  0xFFFF'FFFF'FFFF'F, 0xFFFF'FFFF'FFFF'FF, 0xFFFF'FFFF'FFFF'FFF, 0xFFFF'FFFF'FFFF'FFFF);
	structocol::vector_buffer vb;
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<structocol::varint_t>(vb);
	REQUIRE(inval == outval);
}

TEST_CASE("attempting to deserialize a too large varint_t results in an exception", "[serialization]") {
	structocol::vector_buffer vb;
	constexpr auto max_valid_bytes = (sizeof(std::size_t) * 8) / 7 + ((((sizeof(std::size_t) * 8) % 7) != 0) ? 1 : 0);
	for(std::size_t i = 0; i < max_valid_bytes; ++i) {
		vb.write(std::array{std::byte(0xFFu)});
	}
	vb.write(std::array{std::byte(0x7Fu)}); // The additional byte that ensures the error condition.
	CHECK_THROWS_AS(structocol::deserialize<structocol::varint_t>(vb), structocol::deserialization_data_error);
}

namespace {
enum class test_enum { a, b, c, d };
} // namespace

TEST_CASE("serialization and deserialization of enums (enum class) preserves value", "[serialization]") {
	auto inval = GENERATE(test_enum::a, test_enum::b, test_enum::c, test_enum::d);
	structocol::vector_buffer vb;
	structocol::serialize(vb, inval);
	auto outval = structocol::deserialize<test_enum>(vb);
	REQUIRE(inval == outval);
}

namespace {
struct empty_aggregate {};
} // namespace

TEST_CASE("Empty aggregates are serialized as a zero-length byte sequence", "[serialization]") {
	auto inval = empty_aggregate{};
	structocol::vector_buffer vb;
	structocol::serialize(vb, inval);
	REQUIRE(vb.available_bytes() == 0);
	[[maybe_unused]] auto outval = structocol::deserialize<empty_aggregate>(vb);
	REQUIRE(vb.available_bytes() == 0);
}

TEMPLATE_TEST_CASE("serialization and deserialization of variants preserves value", "[serialization]", std::bitset<0>,
				   std::bitset<1>, std::bitset<2>, std::bitset<4>, std::bitset<8>, std::bitset<16>, std::bitset<32>,
				   std::bitset<64>, std::bitset<128>, std::bitset<256>, std::bitset<512>, std::bitset<1024>,
				   std::bitset<2048>, std::bitset<3>, std::bitset<5>, std::bitset<7>, std::bitset<11>, std::bitset<13>,
				   std::bitset<17>, std::bitset<19>, std::bitset<23>, std::bitset<29>, std::bitset<31>, std::bitset<37>,
				   std::bitset<41>) {
	constexpr const char* test_pattern = "1101110100000011101111011011011010110001000101000111000101010111"
										 "0100111110010100010111101000111000010011010000001101100011011011"
										 "1001101000000001100101011100111000110100100110010001110100110001"
										 "1000011000101001101101001000101001000111100010101010100001111101"
										 "1101010000001010111010000101110010101001100100111101001101000010"
										 "0000111000111101011001101010000001011111100011000010100111000101"
										 "0011001110110101101111111011101001010010100001100000111101001110"
										 "0011110100110110101011001101010100101000110110000101001011101111"
										 "1111001110101001001011000001110101100000110100000011001000010111"
										 "1111101101000011110001000010010110000111000100110101110101000001"
										 "1011111100100100011111010100101001101111000100101011101111111001"
										 "1000111111000010000110000101001101010001010011100011100111001010"
										 "0010010010010011000000100110000011000100101010111000100101101100"
										 "0000000111100111001101100011111011110111101111011011100010111001"
										 "1011000011001110110100011110111110101000101011110101001101000010"
										 "1010111001110011101110011111001001111101111101101010001110011100"
										 "1110110000100000011001010111111010001111100001101010111011011101"
										 "1010010100101111011001100100000111110001011100001010010101001101"
										 "0010101011101100100001011110011001110000000000000100100101001100"
										 "0010011111101011001111101010111110101111100111001101110101000010"
										 "0110110010001110111101001111100000010001100101101110000001000110"
										 "1011110000010111100000100011111110000010111010111010011101011011"
										 "1101111000001000000000100111001100100000101101110001001111110000"
										 "0001110000100110111100100111111100001000111101001110011000001001"
										 "1000100011001110111111011101111000110000011111111010001011110101"
										 "0010101111110111110100011101111011001010011101100111011110001110"
										 "1111111110101011001110111010011101011101111101000001111111101011"
										 "1011000011101101111001011100011001001100101011000001101001101100"
										 "0100010010000011100000110111000001101010100011000111001001101011"
										 "0000011001110010000011011100101001010001110101110001111100100000"
										 "1001111110100100010000001000001101110011100110111100001000001101"
										 "0001000111111010111110101100111100111100011111001110011111101010";
	TestType inval(test_pattern);
	structocol::vector_buffer vb;
	structocol::serialize(vb, inval);
	constexpr auto expected_bytes = inval.size() / 8 + ((inval.size() % 8 != 0) ? 1 : 0);
	REQUIRE(vb.available_bytes() == expected_bytes);
	auto outval = structocol::deserialize<TestType>(vb);
	CHECK(vb.available_bytes() == 0);
	CHECK(outval == inval);
}

TEST_CASE("correctly serialized magic_numbers have the expected length and pass the deserialization check",
		  "[serialization]") {
	SECTION("for a char sequence") {
		structocol::magic_number<'H', 'e', 'l', 'l', 'o'> inval;
		structocol::vector_buffer vb;
		structocol::serialize(vb, inval);
		CHECK(vb.available_bytes() == 5);
		[[maybe_unused]] auto outval = structocol::deserialize<decltype(inval)>(vb);
	}
	SECTION("for a heterogeneously-typed number sequence") {
		using magic_number_type = structocol::magic_number<std::uint8_t(42), std::uint16_t(1234), std::uint32_t(987654),
														   std::uint64_t(0xFF00FF00)>;
		magic_number_type inval;
		structocol::vector_buffer vb;
		structocol::serialize(vb, inval);
		CHECK(vb.available_bytes() == 15);
		[[maybe_unused]] auto outval = structocol::deserialize<decltype(inval)>(vb);
	}
}

TEST_CASE("attempting to deserialize a magic_number from invalid data throws the expected exception",
		  "[serialization]") {
	SECTION("for a char sequence") {
		structocol::vector_buffer vb;
		vb.write(std::array{std::byte{'W'}});
		vb.write(std::array{std::byte{'o'}});
		vb.write(std::array{std::byte{'r'}});
		vb.write(std::array{std::byte{'l'}});
		vb.write(std::array{std::byte{'d'}});
		using magic_number_type = structocol::magic_number<'H', 'e', 'l', 'l', 'o'>;
		CHECK_THROWS_AS(structocol::deserialize<magic_number_type>(vb), structocol::deserialization_data_error);
	}
	SECTION("for a heterogeneously-typed number sequence") {
		using magic_number_type = structocol::magic_number<std::uint8_t(42), std::uint16_t(1234), std::uint32_t(987654),
														   std::uint64_t(0xFF00FF00)>;
		structocol::vector_buffer vb;
		vb.write(std::array{std::byte{'H'}});
		vb.write(std::array{std::byte{'e'}});
		vb.write(std::array{std::byte{'l'}});
		vb.write(std::array{std::byte{'l'}});
		vb.write(std::array{std::byte{'o'}});
		vb.write(std::array{std::byte{' '}});
		vb.write(std::array{std::byte{'W'}});
		vb.write(std::array{std::byte{'o'}});
		vb.write(std::array{std::byte{'r'}});
		vb.write(std::array{std::byte{'l'}});
		vb.write(std::array{std::byte{'d'}});
		vb.write(std::array{std::byte{'!'}});
		vb.write(std::array{std::byte{'1'}});
		vb.write(std::array{std::byte{'2'}});
		vb.write(std::array{std::byte{'3'}});
		CHECK_THROWS_AS(structocol::deserialize<magic_number_type>(vb), structocol::deserialization_data_error);
	}
}
