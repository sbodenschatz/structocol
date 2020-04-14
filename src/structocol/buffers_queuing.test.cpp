#include <catch2/catch.hpp>
#include <structocol/buffers_ring.hpp>
#include <structocol/serialization.hpp>
#include <structocol/vector_buffer.hpp>

namespace {
template <typename BQ, typename T>
void obtain_write(BQ& bq, const T& value) {
	auto& be = bq.obtain_back();
	structocol::serialize(be.buffer, value);
}
template <typename BQ, typename T>
void read_check_recycle(BQ& bq, const T& value) {
	auto& be = bq.front();
	auto read_val = structocol::deserialize<T>(be.buffer);
	bq.recycle_front();
	CHECK(read_val == value);
}
} // namespace

TEMPLATE_TEST_CASE("Queuing of active buffers works correctly in FIFO order.", "[buffers_queuing]",
				   structocol::buffers_ring<structocol::vector_buffer<>>) {
	TestType bq;
	using namespace std::literals;
	obtain_write(bq, 1234);
	obtain_write(bq, 2345);
	obtain_write(bq, 3456);
	obtain_write(bq, 4567);
	read_check_recycle(bq, 1234);
	read_check_recycle(bq, 2345);
	read_check_recycle(bq, 3456);
	obtain_write(bq, "Test"s);
	obtain_write(bq, "Hello"s);
	obtain_write(bq, "World"s);
	read_check_recycle(bq, 4567);
	read_check_recycle(bq, "Test"s);
	read_check_recycle(bq, "Hello"s);
	read_check_recycle(bq, "World"s);
}

TEMPLATE_TEST_CASE("Queued buffers are correctly recycled and cleared.", "[buffers_queuing]",
				   structocol::buffers_ring<structocol::vector_buffer<>>) {
	TestType bq;
	for(int i = 0; i < 10; ++i) {
		auto& be = bq.obtain_back();
		be.buffer.reserve(1024);
		CHECK(be.buffer.total_capacity() == 1024);
		be.buffer.write(std::array{std::byte('H'), std::byte('e'), std::byte('l'), std::byte('l'), std::byte('o')});
		CHECK(be.buffer.available_bytes() == 5);
	}
	CHECK(bq.size() == 10);
	for(int i = 0; i < 10; ++i) {
		CHECK(bq.front().buffer.total_capacity() == 1024);
		bq.recycle_front();
	}
	CHECK(bq.empty());
	// All buffers have been consumed.
	// We should now be able to reuse the recycled buffers.
	for(int i = 0; i < 10; ++i) {
		auto& be = bq.obtain_back();
		CHECK(be.buffer.total_capacity() == 1024);
		CHECK(be.buffer.available_bytes() == 0);
	}
}
