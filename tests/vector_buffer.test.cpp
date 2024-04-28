#include <algorithm>
#include <catch2/catch_all.hpp>
#include <structocol/vector_buffer.hpp>

TEST_CASE("vector_buffer can read back data written to it (reads after writes)", "[vector_buffer]") {
	structocol::vector_buffer vb;

	SECTION("sequences of single byte writes and read backs") {
		vb.write(std::array{std::byte('H')});
		vb.write(std::array{std::byte('e')});
		vb.write(std::array{std::byte('l')});
		vb.write(std::array{std::byte('l')});
		vb.write(std::array{std::byte('o')});
		REQUIRE(vb.read<1>().front() == std::byte('H'));
		REQUIRE(vb.read<1>().front() == std::byte('e'));
		REQUIRE(vb.read<1>().front() == std::byte('l'));
		REQUIRE(vb.read<1>().front() == std::byte('l'));
		REQUIRE(vb.read<1>().front() == std::byte('o'));
	}
	SECTION("single multi-byte write and sequence of single byte read backs") {
		vb.write(std::array{std::byte('W'), std::byte('o'), std::byte('r'), std::byte('l'), std::byte('d')});
		REQUIRE(vb.read<1>().front() == std::byte('W'));
		REQUIRE(vb.read<1>().front() == std::byte('o'));
		REQUIRE(vb.read<1>().front() == std::byte('r'));
		REQUIRE(vb.read<1>().front() == std::byte('l'));
		REQUIRE(vb.read<1>().front() == std::byte('d'));
	}
	SECTION("sequence of single byte writes and single multi-bytes read back") {
		vb.write(std::array{std::byte('H')});
		vb.write(std::array{std::byte('E')});
		vb.write(std::array{std::byte('L')});
		vb.write(std::array{std::byte('L')});
		vb.write(std::array{std::byte('O')});
		auto data = vb.read<5>();
		REQUIRE(data.at(0) == std::byte('H'));
		REQUIRE(data.at(1) == std::byte('E'));
		REQUIRE(data.at(2) == std::byte('L'));
		REQUIRE(data.at(3) == std::byte('L'));
		REQUIRE(data.at(4) == std::byte('O'));
	}
	SECTION("multi-byte write and read back") {
		vb.write(std::array{std::byte('W'), std::byte('O'), std::byte('R'), std::byte('L'), std::byte('D')});
		auto data = vb.read<5>();
		REQUIRE(data.at(0) == std::byte('W'));
		REQUIRE(data.at(1) == std::byte('O'));
		REQUIRE(data.at(2) == std::byte('R'));
		REQUIRE(data.at(3) == std::byte('L'));
		REQUIRE(data.at(4) == std::byte('D'));
	}
}

TEST_CASE("vector_buffer can read back data written to it (interleaved)", "[vector_buffer]") {
	structocol::vector_buffer vb;

	SECTION("sequences of single byte writes and read backs") {
		vb.write(std::array{std::byte('H')});
		REQUIRE(vb.read<1>().front() == std::byte('H'));
		vb.write(std::array{std::byte('e')});
		REQUIRE(vb.read<1>().front() == std::byte('e'));
		vb.write(std::array{std::byte('l')});
		REQUIRE(vb.read<1>().front() == std::byte('l'));
		vb.write(std::array{std::byte('l')});
		REQUIRE(vb.read<1>().front() == std::byte('l'));
		vb.write(std::array{std::byte('o')});
		REQUIRE(vb.read<1>().front() == std::byte('o'));
	}
}

TEST_CASE("vector_buffer trim reuses already read memory to regain writable capacity and keeps readable data",
		  "[vector_buffer]") {
	structocol::vector_buffer vb;
	std::array<std::byte, 256> data;
	std::generate(data.begin(), data.end(), [b = std::uint8_t{0}]() mutable { return std::byte{b++}; });
	vb.write(data);
	auto half_data = vb.read<128>();
	SECTION("preparing and checking buffer state") {
		REQUIRE(std::equal(data.begin(), data.begin() + 128, half_data.begin(), half_data.end()));
		REQUIRE(vb.available_bytes() == 128);
		REQUIRE(vb.total_capacity() >= 256);
	}
	SECTION("performing trim and checking new state") {
		auto old_write_cap = vb.writable_capacity();
		auto old_total_cap = vb.total_capacity();
		vb.trim();
		REQUIRE(vb.available_bytes() == 128);
		REQUIRE(vb.total_capacity() == old_total_cap);
		REQUIRE(vb.writable_capacity() == old_write_cap + 128);
		auto quarter_data = vb.read<64>();
		REQUIRE(std::equal(data.begin() + 128, data.begin() + 192, quarter_data.begin(), quarter_data.end()));
		REQUIRE(vb.available_bytes() == 64);
	}
	SECTION("reserve and trim") {
		vb.trim();
		[[maybe_unused]] auto quarter_data = vb.read<64>();
		vb.reserve(4096);
		REQUIRE(vb.writable_capacity() >= 4096);
		auto old_write_cap = vb.writable_capacity();
		vb.trim();
		REQUIRE(vb.available_bytes() == 64);
		REQUIRE(vb.writable_capacity() == old_write_cap + 64);
		REQUIRE(vb.writable_capacity() >= 4096 + 64);
		auto rest = vb.read<64>();
		REQUIRE(std::equal(data.begin() + 192, data.end(), rest.begin(), rest.end()));
		REQUIRE(vb.available_bytes() == 0);
		vb.trim();
		REQUIRE(vb.writable_capacity() == old_write_cap + 128);
		REQUIRE(vb.writable_capacity() == vb.total_capacity());
	}
}
