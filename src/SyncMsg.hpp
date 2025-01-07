//
// Created by crosstyan on 24-9-23.
//

#ifndef SYNCMSG_HPP
#define SYNCMSG_HPP
#include <cstdint>
#include <optional>
#include <span>

namespace app {
/// @note use with `pixel_format` field in `frame_info_t`
enum class PixelFormat : uint8_t {
	/// usually 24bit RGB (8bit per channel, depth=U8)
	RGB = 0,
	BGR,
	RGBA,
	BGRA,
	/// channel=1
	GRAY,
	YUV,
	YUYV,
};

/// @note use with `depth` field in `frame_info_t`
enum class Depth : uint8_t {
	U8 = 0,
	S8,
	U16,
	S16,
	S32,
	F32,
	F64,
	F16,
};

constexpr auto FRAME_TOPIC_MAGIC = 0x7d;

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
	Depth depth;
	uint32_t buffer_size;
	PixelFormat pixel_format = PixelFormat::BGR;

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
	/// this field SHOULD NOT be modified
	uint8_t magic = FRAME_TOPIC_MAGIC;
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
}

#endif // SYNCMSG_HPP
