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

