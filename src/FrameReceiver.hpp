//
// Created by crosstyan on 24-9-22.
//

#ifndef FRAMERECEIVER_HPP
#define FRAMERECEIVER_HPP
#include <nan.h>
#include <zmq_addon.hpp>
#include <optional>

namespace app {
// https://docs.opencv.org/4.x/d3/d63/classcv_1_1Mat.html
// See `Detailed Description`
// strides for each dimension
// stride[0]=channel
// stride[1]=channel*cols
// stride[2]=channel*cols*rows
struct __attribute__((packed)) frame_info_t {
	uint16_t width;
	uint16_t height;
	uint8_t channels;
	/// CV_8U, CV_8S, CV_16U, CV_16S, CV_16F, CV_32S, CV_32F, CV_64F
	uint8_t depth;
	uint32_t buffer_size;

	[[nodiscard]]
	int pixelWidth() const {
		return cv_depth_to_size(depth);
	}

	int marshal(std::span<uint8_t> buf) const {
		if (buf.size() < sizeof(frame_info_t)) {
			return -1;
		}
		memcpy(buf.data(), this, sizeof(frame_info_t));
		return sizeof(frame_info_t);
	}

	static std::optional<frame_info_t> unmarshal(const std::span<uint8_t> buf) {
		if (buf.size() < sizeof(frame_info_t)) {
			return std::nullopt;
		}
		frame_info_t info;
		memcpy(&info, buf.data(), sizeof(frame_info_t));
		return info;
	}
};

struct __attribute__((packed)) sync_message_t {
	uint32_t frame_count;
	frame_info_t info;
	// NOTE: I don't need the `name` field
	// as long as we don't share same IPC socket for different video sources.
	int marshal(std::span<uint8_t> buf) const {
		if (buf.size() < sizeof(sync_message_t)) {
			return -1;
		}
		memcpy(buf.data(), this, sizeof(sync_message_t));
		return sizeof(sync_message_t);
	}

	static std::optional<sync_message_t> unmarshal(const std::span<uint8_t> buf) {
		if (buf.size() < sizeof(sync_message_t)) {
			return std::nullopt;
		}
		sync_message_t msg;
		memcpy(&msg, buf.data(), sizeof(sync_message_t));
		return msg;
	}
};

// https://github.com/nodejs/node-addon-api/blob/main/doc/object_wrap.md
// https://github.com/nodejs/nan/blob/main/doc/object_wrappers.md
// https://github.com/nodejs/nan/blob/main/doc/node_misc.md
// https://nodesource.com/blog/NAN-to-Node-API-migration-a-short-story/
class FrameReceiver : public Nan::ObjectWrap {
	zmq::context_t ctx;
	zmq::socket_t sock;
	std::optional<int> shm_fd;
};
}

#endif // FRAMERECEIVER_HPP
