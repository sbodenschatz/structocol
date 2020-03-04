#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <cstdio>

#include <algorithm>
#include <catch2/catch.hpp>
#include <memory>
#include <structocol/stdio_buffer.hpp>

namespace {
struct checked_rewinder {
	std::FILE* fh_;
	long start_pos_;
	long expected_length_;
	checked_rewinder(std::FILE* fh, long expected_length)
			: fh_{fh}, start_pos_{std::ftell(fh)}, expected_length_{expected_length} {}
	void rewind() {
		// REQUIRE(std::fflush(fh_) == 0);
		REQUIRE(std::ftell(fh_) == start_pos_ + expected_length_);
		REQUIRE(std::fseek(fh_, -expected_length_, SEEK_CUR) == 0);
	}
	void check() {
		long cur_pos = std::ftell(fh_);
		CHECK(cur_pos == start_pos_ + expected_length_);
		start_pos_ = cur_pos;
		std::fseek(fh_, 0, SEEK_CUR);
	}
};
} // namespace

TEST_CASE("stdio_buffer can read back data written to it (reads after writes)", "[stdio_buffer]") {
	std::unique_ptr<std::FILE, void (*)(std::FILE*)> file_handle = {std::fopen("test_file.temp", "w+b"),
																	[](std::FILE* f) { std::fclose(f); }};
	structocol::stdio_buffer iob(file_handle.get());

	SECTION("sequences of single byte writes and read backs") {
		checked_rewinder cr(file_handle.get(), 5);
		iob.write(std::array{std::byte('H')});
		iob.write(std::array{std::byte('e')});
		iob.write(std::array{std::byte('l')});
		iob.write(std::array{std::byte('l')});
		iob.write(std::array{std::byte('o')});
		cr.rewind();
		REQUIRE(iob.read<1>().front() == std::byte('H'));
		REQUIRE(iob.read<1>().front() == std::byte('e'));
		REQUIRE(iob.read<1>().front() == std::byte('l'));
		REQUIRE(iob.read<1>().front() == std::byte('l'));
		REQUIRE(iob.read<1>().front() == std::byte('o'));
		cr.check();
	}
	SECTION("single multi-byte write and sequence of single byte read backs") {
		checked_rewinder cr(file_handle.get(), 5);
		iob.write(std::array{std::byte('W'), std::byte('o'), std::byte('r'), std::byte('l'), std::byte('d')});
		cr.rewind();
		REQUIRE(iob.read<1>().front() == std::byte('W'));
		REQUIRE(iob.read<1>().front() == std::byte('o'));
		REQUIRE(iob.read<1>().front() == std::byte('r'));
		REQUIRE(iob.read<1>().front() == std::byte('l'));
		REQUIRE(iob.read<1>().front() == std::byte('d'));
		cr.check();
	}
	SECTION("sequence of single byte writes and single multi-bytes read back") {
		checked_rewinder cr(file_handle.get(), 5);
		iob.write(std::array{std::byte('H')});
		iob.write(std::array{std::byte('E')});
		iob.write(std::array{std::byte('L')});
		iob.write(std::array{std::byte('L')});
		iob.write(std::array{std::byte('O')});
		cr.rewind();
		auto data = iob.read<5>();
		REQUIRE(data.at(0) == std::byte('H'));
		REQUIRE(data.at(1) == std::byte('E'));
		REQUIRE(data.at(2) == std::byte('L'));
		REQUIRE(data.at(3) == std::byte('L'));
		REQUIRE(data.at(4) == std::byte('O'));
		cr.check();
	}
	SECTION("multi-byte write and read back") {
		checked_rewinder cr(file_handle.get(), 5);
		iob.write(std::array{std::byte('W'), std::byte('O'), std::byte('R'), std::byte('L'), std::byte('D')});
		cr.rewind();
		auto data = iob.read<5>();
		REQUIRE(data.at(0) == std::byte('W'));
		REQUIRE(data.at(1) == std::byte('O'));
		REQUIRE(data.at(2) == std::byte('R'));
		REQUIRE(data.at(3) == std::byte('L'));
		REQUIRE(data.at(4) == std::byte('D'));
		cr.check();
	}
}

TEST_CASE("stdio_buffer can read back data written to it (interleaved)", "[stdio_buffer]") {
	std::unique_ptr<std::FILE, void (*)(std::FILE*)> file_handle = {std::fopen("test_file.temp", "w+b"),
																	[](std::FILE* f) { std::fclose(f); }};
	structocol::stdio_buffer iob(file_handle.get());

	SECTION("sequences of single byte writes and read backs") {
		checked_rewinder cr(file_handle.get(), 1);
		iob.write(std::array{std::byte('H')});
		cr.rewind();
		REQUIRE(iob.read<1>().front() == std::byte('H'));
		cr.check();
		iob.write(std::array{std::byte('e')});
		cr.rewind();
		REQUIRE(iob.read<1>().front() == std::byte('e'));
		cr.check();
		iob.write(std::array{std::byte('l')});
		cr.rewind();
		REQUIRE(iob.read<1>().front() == std::byte('l'));
		cr.check();
		iob.write(std::array{std::byte('l')});
		cr.rewind();
		REQUIRE(iob.read<1>().front() == std::byte('l'));
		cr.check();
		iob.write(std::array{std::byte('o')});
		cr.rewind();
		REQUIRE(iob.read<1>().front() == std::byte('o'));
		cr.check();
	}
}
