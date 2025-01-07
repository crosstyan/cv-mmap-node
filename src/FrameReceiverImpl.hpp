//
// Created by crosstyan on 24-9-23.
//

#ifndef FRAMERECEIVERIMPL_HPP
#define FRAMERECEIVERIMPL_HPP
#include <cstdint>
#include <optional>
#include <thread>
#include <atomic>
#include <expected>
#include <utility>
#include <format>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <SyncMsg.hpp>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef TRACE_LOG_STDOUT
#include <iostream>
#include <print>
#endif

namespace app {
inline std::string to_hex(const std::span<const uint8_t> data) {
	std::stringstream ss;
	for (const auto &byte : data) {
		ss << std::format("{:02X}", byte);
	}
	return ss.str();
}

const char *depth_to_string(const Depth depth) {
	switch (depth) {
	case Depth::U8:
		return "U8";
	case Depth::S8:
		return "S8";
	case Depth::U16:
		return "U16";
	case Depth::S16:
		return "S16";
	case Depth::F16:
		return "F16";
	case Depth::S32:
		return "S32";
	case Depth::F32:
		return "F32";
	case Depth::F64:
		return "F64";
	default:
		return "unknown";
	}
}

const char *pixel_format_to_string(const PixelFormat fmt) {
	switch (fmt) {
	case PixelFormat::RGB:
		return "RGB";
	case PixelFormat::BGR:
		return "BGR";
	case PixelFormat::RGBA:
		return "RGBA";
	case PixelFormat::BGRA:
		return "BGRA";
	case PixelFormat::GRAY:
		return "GRAY";
	case PixelFormat::YUV:
		return "YUV";
	case PixelFormat::YUYV:
		return "YUYV";
	default:
		return "unknown";
	}
}

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

	inline std::optional<std::span<const uint8_t>> frame_buffer() {
		if (shm_ptr != nullptr && reference_frame_info.has_value()) {
			const auto buffer_size = reference_frame_info.value().buffer_size;
			return std::span<const uint8_t>(static_cast<const uint8_t *>(shm_ptr), buffer_size);
		}
		return std::nullopt;
	}

public:
	FrameReceiverImpl(std::string shm_name, std::string zmq_addr) : shm_name(std::move(shm_name)), zmq_addr(std::move(zmq_addr)) {}
	~FrameReceiverImpl() { stop(); }
	[[nodiscard]]
	std::expected<unit_t, std::string> start() {
		using ue_t = std::unexpected<std::string>;
		if (is_running) {
			return ue_t("Already running");
		}
		has_init   = false;
		this->ctx  = zmq::context_t{};
		this->sock = zmq::socket_t(ctx, zmq::socket_type::sub);
		this->sock.set(zmq::sockopt::conflate, true);
		try {
			sock.connect(zmq_addr);
		} catch (const zmq::error_t &e) {
			return ue_t(std::format("Failed to connect to `{}`; {}", zmq_addr, e.what()));
		}
		// subscribe to all topics (we only have one kind of message anyway)
		constexpr auto topic_buffer = std::array<uint8_t, 1>{FRAME_TOPIC_MAGIC};
		sock.set(zmq::sockopt::subscribe, zmq::buffer(topic_buffer));
#ifdef TRACE_LOG_STDOUT
		std::println("Connected to {}", zmq_addr);
#endif
		shm_fd = shm_open(shm_name.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (shm_fd == -1) {
			return ue_t(std::format("Failed to open shared memory `{}`; {}", shm_name, strerror(errno)));
		}
#ifdef TRACE_LOG_STDOUT
		std::println("Opened shared memory {}", shm_name);
#endif
		receiver_thread = std::make_unique<std::thread>([this] {
#ifdef TRACE_LOG_STDOUT
			std::println("Thread started");
#endif
			is_running         = true;
			const auto on_init = [this](const sync_message_t &msg) -> bool {
#ifdef TRACE_LOG_STDOUT
				std::println("Received init message: "
							 "frame_count={}, "
							 "width={}, "
							 "height={}, "
							 "channels={}, "
							 "depth={}, "
							 "buffer_size={}"
							 "pixel_format={}",
							 msg.frame_count,
							 msg.info.width,
							 msg.info.height,
							 msg.info.channels,
							 depth_to_string(msg.info.depth),
							 msg.info.buffer_size,
							 pixel_format_to_string(msg.info.pixel_format));
#endif
				reference_frame_info = msg.info;
				shm_ptr              = mmap(nullptr, msg.info.buffer_size, PROT_READ, MAP_SHARED, shm_fd.value(), 0);
				if (shm_ptr == MAP_FAILED) {
#ifdef TRACE_LOG_STDOUT
					std::println("Failed to map shared memory: {}", strerror(errno));
#endif
					return false;
				}
				if (on_frame) {
					on_frame(msg, frame_buffer().value());
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
				const auto msg_opt = sync_message_t::unmarshal(std::span<uint8_t>(static_cast<uint8_t *>(msg.data()), *res));
				if (not msg_opt) {
#ifdef TRACE_LOG_STDOUT
					std::println("Failed to unmarshal message {}", to_hex(std::span<uint8_t>(static_cast<uint8_t *>(msg.data()), *res)));
#endif
					continue;
				}
				if (not has_init) {
					if (not on_init(msg_opt.value())) {
#ifdef TRACE_LOG_STDOUT
						std::println("Failed to initialize");
#endif
						is_running = false;
					}
				} else {
					if (on_frame) {
						on_frame(msg_opt.value(), frame_buffer().value());
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
		if (receiver_thread) {
			if (receiver_thread->joinable()) {
#ifdef TRACE_LOG_STDOUT
				std::println("Joining thread");
#endif
				receiver_thread->join();
			}
		}
		receiver_thread.reset();
		if (shm_ptr != nullptr) {
#ifdef TRACE_LOG_STDOUT
			std::println("Unmapping shared memory");
#endif
			if (reference_frame_info) {
				const auto buffer_size = reference_frame_info.value().buffer_size;
				munmap(shm_ptr, buffer_size);
			}
			shm_ptr = nullptr;
		}
		reference_frame_info.reset();
		if (shm_fd) {
#ifdef TRACE_LOG_STDOUT
			std::println("Closing shared memory");
#endif
			close(*shm_fd);
		}
		shm_fd.reset();
		has_init = false;
	}
};
}

#endif // FRAMERECEIVERIMPL_HPP
