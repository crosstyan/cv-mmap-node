//
// Created by crosstyan on 24-9-23.
//

#ifndef FRAMERECEIVERIMPL_HPP
#define FRAMERECEIVERIMPL_HPP
#include <optional>
#include <thread>
#include <atomic>
#include <expected>
#include <utility>
#include <format>
#include <zmq_addon.hpp>
#include <SyncMsg.hpp>
#include <sys/mman.h>
#include <fcntl.h>

#ifdef TRACE_LOG_STDOUT
#include <iostream>
#include <print>
#endif

namespace app {
using unit_t     = std::monostate;
using on_frame_t = std::function<void(const sync_message_t &, std::span<const uint8_t>)>;
class FrameReceiverImpl {
	std::string shm_name;
	std::string zmq_addr;

	zmq::context_t ctx;
	zmq::socket_t sock;
	std::optional<int> shm_fd = std::nullopt;
	void *shm_ptr             = nullptr;

	std::optional<frame_info_t> reference_frame_info = std::nullopt;

	std::atomic_bool is_running                  = false;
	std::atomic_bool has_init                    = false;
	std::unique_ptr<std::thread> receiver_thread = nullptr;
	on_frame_t on_frame                          = nullptr;

public:
	FrameReceiverImpl(std::string shm_name, std::string zmq_addr) : shm_name(std::move(shm_name)), zmq_addr(std::move(zmq_addr)) {}
	[[nodiscard]]
	std::expected<unit_t, std::string> start() {
		using ue_t = std::unexpected<std::string>;
		if (is_running) {
			return ue_t("Already running");
		}
		has_init   = false;
		this->ctx  = zmq::context_t{};
		this->sock = zmq::socket_t(ctx, zmq::socket_type::pull);
		try {
			sock.connect(zmq_addr);
		} catch (const zmq::error_t &e) {
			return ue_t(std::format("Failed to connect to `{}`; {}", zmq_addr, e.what()));
		}
#ifdef TRACE_LOG_STDOUT
		std::print("Connected to {}\n", zmq_addr);
#endif
		shm_fd = shm_open(shm_name.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (shm_fd == -1) {
			return ue_t(std::format("Failed to open shared memory `{}`; {}", shm_name, strerror(errno)));
		}
#ifdef TRACE_LOG_STDOUT
		std::print("Opened shared memory {}\n", shm_name);
#endif
		receiver_thread = std::make_unique<std::thread>([this] {
#ifdef TRACE_LOG_STDOUT
			std::print("Thread started\n");
#endif
			is_running         = true;
			const auto on_init = [this](const sync_message_t &msg) -> bool {
#ifdef TRACE_LOG_STDOUT
				std::print("Received init message: frame_count={}, width={}, height={}, channels={}, depth={}, buffer_size={}\n",
						   msg.frame_count, msg.info.width, msg.info.height, msg.info.channels, msg.info.depth, msg.info.buffer_size);
#endif
				reference_frame_info = msg.info;
				shm_ptr              = mmap(nullptr, msg.info.buffer_size, PROT_READ, MAP_SHARED, shm_fd.value(), 0);
				if (shm_ptr == MAP_FAILED) {
#ifdef TRACE_LOG_STDOUT
					std::print("Failed to map shared memory: {}\n", strerror(errno));
#endif
					return false;
				}
				if (on_frame) {
					on_frame(msg, std::span<const uint8_t>(static_cast<const uint8_t *>(shm_ptr), msg.info.buffer_size));
				}
				has_init = true;
				return true;
			};
			while (is_running) {
				zmq::message_t msg;
				const auto res = sock.recv(msg, zmq::recv_flags::dontwait);
				if (!res) {
					continue;
				}
				auto msg_opt = sync_message_t::unmarshal(std::span<uint8_t>(static_cast<uint8_t *>(msg.data()), *res));
				if (!msg_opt.has_value()) {
					continue;
				}
				if (not has_init) {
					if (not on_init(msg_opt.value())) {
						is_running = false;
					}
				} else {
					if (on_frame) {
						on_frame(msg_opt.value(), std::span<const uint8_t>(static_cast<const uint8_t *>(shm_ptr), msg_opt.value().info.buffer_size));
					}
				}
			}
		});
		return {};
	}

	void setOnFrame(on_frame_t &&on_frame) {
		this->on_frame = std::move(on_frame);
	}

	[[nodiscard]]
	bool isRunning() const {
		return is_running;
	}

	void stop() {
		is_running = false;
		if (receiver_thread->joinable()) {
#ifdef TRACE_LOG_STDOUT
			std::print("Joining thread\n");
#endif
			receiver_thread->join();
		}
		receiver_thread.reset();
		if (shm_ptr != nullptr) {
#ifdef TRACE_LOG_STDOUT
			std::print("Unmapping shared memory\n");
#endif
			munmap(shm_ptr, reference_frame_info.value().buffer_size);
			shm_ptr = nullptr;
		}
		if (shm_fd.has_value()) {
#ifdef TRACE_LOG_STDOUT
			std::print("Closing shared memory\n");
#endif
			close(shm_fd.value());
		}
		shm_fd.reset();
		has_init = false;
	}
};
}

#endif // FRAMERECEIVERIMPL_HPP
