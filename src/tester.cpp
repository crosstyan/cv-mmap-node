//
// Created by crosstyan on 24-9-23.
//
#include <FrameReceiverImpl.hpp>
#include <iostream>
#include <print>

constexpr auto SHM_NAME = "/psm_default";
constexpr auto ZMQ_ADDR = "ipc:///tmp/0";


int main() {
	auto impl = app::FrameReceiverImpl(SHM_NAME, ZMQ_ADDR);
	impl.setOnFrame([](const app::sync_message_t &msg, std::span<const uint8_t> data) {
		// get the last 40 bytes of the data
		const auto offset = data.size() - 40;
		std::print("[main] frame@{} {}x{}x{}; {}\n",
				   msg.frame_count, msg.info.width, msg.info.height, msg.info.channels,
				   app::to_hex(data.subspan(offset, 40)));
	});
	if (auto res = impl.start(); res.has_value()) {
		std::cout << "Started" << std::endl;
	} else {
		std::cerr << "Failed to start: " << res.error() << std::endl;
	}
	std::this_thread::sleep_for(std::chrono::seconds(3));
	// stop would be called by the destructor
	// impl.stop();
	return 0;
}
