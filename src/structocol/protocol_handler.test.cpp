#include <catch2/catch.hpp>
#include <map>
#include <string>
#include <structocol/protocol_handler.hpp>
#include <structocol/vector_buffer.hpp>
#include <variant>
#include <vector>

namespace {
struct hello_msg {
	std::string name;
};
bool operator==(const hello_msg& a, const hello_msg& b) {
	return a.name == b.name;
}
struct lobby_msg {
	std::vector<std::string> users;
};
bool operator==(const lobby_msg& a, const lobby_msg& b) {
	return a.users == b.users;
}
struct enter_result_msg {
	std::string name;
	std::uint64_t score;
};
bool operator==(const enter_result_msg& a, const enter_result_msg& b) {
	return std::tie(a.name, a.score) == std::tie(b.name, b.score);
}
struct score_board_msg {
	std::map<std::string, std::uint64_t> scores;
};
bool operator==(const score_board_msg& a, const score_board_msg& b) {
	return a.scores == b.scores;
}
} // namespace

TEST_CASE("protocol handler message stream arrives (receive_message) as it was sent", "[protocol_handler]") {
	using msg_t = std::variant<hello_msg, lobby_msg, enter_result_msg, score_board_msg>;
	std::vector<msg_t> msg_seq{hello_msg{"John Doe"}, lobby_msg{{"John Doe", "Jane Smith"}},
							   enter_result_msg{"John Doe", 9001},
							   score_board_msg{{{"John Doe", 9001}, {"Jane Smith", 10000}}}};
	structocol::protocol_handler<hello_msg, lobby_msg, enter_result_msg, score_board_msg> ph;
	structocol::vector_buffer vb;
	for(const auto& mvar : msg_seq) {
		std::visit([&ph, &vb](const auto& m) { ph.send_message(vb, m); }, mvar);
	}

	std::vector<msg_t> msg_seq_recv;
	for(std::size_t i = 0; i < msg_seq.size(); ++i) {
		msg_seq_recv.push_back(ph.receive_message(vb));
	}
	REQUIRE(msg_seq_recv == msg_seq);
}
TEST_CASE("protocol handler message stream arrives (using process_message) as it was sent",
		  "[protocol_handler]") {
	using msg_t = std::variant<hello_msg, lobby_msg, enter_result_msg, score_board_msg>;
	std::vector<msg_t> msg_seq{hello_msg{"John Doe"}, lobby_msg{{"John Doe", "Jane Smith"}},
							   enter_result_msg{"John Doe", 9001},
							   score_board_msg{{{"John Doe", 9001}, {"Jane Smith", 10000}}}};
	structocol::protocol_handler<hello_msg, lobby_msg, enter_result_msg, score_board_msg> ph;
	structocol::vector_buffer vb;
	for(const auto& mvar : msg_seq) {
		std::visit([&ph, &vb](const auto& m) { ph.send_message(vb, m); }, mvar);
	}

	std::vector<msg_t> msg_seq_proc;
	for(std::size_t i = 0; i < msg_seq.size(); ++i) {
		ph.process_message(vb, [&msg_seq_proc](const auto& m) { msg_seq_proc.push_back(m); });
	}
	REQUIRE(msg_seq_proc == msg_seq);
}
