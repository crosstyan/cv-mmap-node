//
// Created by crosstyan on 24-9-22.
//

#ifndef FRAMERECEIVER_HPP
#define FRAMERECEIVER_HPP
#include "FrameReceiverImpl.hpp"


#include <cstddef>
#include <format>
#include <print>
#include <napi.h>
#include <zmq_addon.hpp>
#include <opencv2/opencv.hpp>
#include <optional>

namespace app {
using namespace Napi;
using Napi::Error;
using Napi::Object;
using Napi::RangeError;
using Napi::TypeError;
using Napi::Value;

// https://github.com/nodejs/node-addon-api/blob/main/doc/object_wrap.md
// https://gitlab.ifi.uzh.ch/scheid/bcoln/-/blob/619c184df737026b2c7ae222d76da7435f101d9a/Web3/web3js/node_modules/node-addon-api/doc/object_wrap.md
// https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe.md
// https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe_function.md
// https://github.com/nodejs/node-addon-api/blob/main/doc/typed_threadsafe_function.md
// https://github.com/nodejs/node-addon-api/blob/main/doc/error_handling.md

inline size_t BgrToRgbaSize(size_t bgr_size) {
	return (bgr_size / 3) * 4;
}

void BgrToRgba(const std::span<const uint8_t> &src, Napi::Buffer<uint8_t> &dst) {
	if (const auto expected = BgrToRgbaSize(src.size()); dst.Length() != expected) {
		throw RangeError::New(dst.Env(), std::format("dst_size({}) != expected({})", dst.Length(), expected));
	}
	size_t i = 0;
	size_t j = 0;
	while (i < src.size()) {
		dst[j]     = src[i + 2];
		dst[j + 1] = src[i + 1];
		dst[j + 2] = src[i];
		dst[j + 3] = 255;
		i += 3;
		j += 4;
	}
}

class FrameReceiver final : public Napi::ObjectWrap<FrameReceiver> {
	std::unique_ptr<FrameReceiverImpl> impl_ = nullptr;
	ThreadSafeFunction tsfn_                 = nullptr;
	std::atomic_bool is_being_blocking_call_ = false;
	float scale_factor_                      = 1.0f;
	void onFrame(const sync_message_t &msg, std::span<const uint8_t> data);

public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	explicit FrameReceiver(const Napi::CallbackInfo &info);

	Napi::Value Start(const Napi::CallbackInfo &info);
	Napi::Value Stop(const Napi::CallbackInfo &info);
	Napi::Value SetOnFrame(const Napi::CallbackInfo &info);
	Napi::Value SetScaleFactor(const Napi::CallbackInfo &info);
};

inline void FrameReceiver::onFrame(const sync_message_t &msg, std::span<const uint8_t> data) {
	if (tsfn_ == nullptr) {
		return;
	}
	// only one callback is allowed to run at a time
	if (is_being_blocking_call_) {
		return;
	}
	struct frame_args_t {
		sync_message_t msg;
		std::span<const uint8_t> data;
	};
	static frame_args_t args;
	auto callback = [this](Napi::Env env, Napi::Function jsCallback, frame_args_t *args_ptr) {
		const auto &args = *args_ptr;
		const auto &msg  = args.msg;
		const auto &data = args.data;
		// Get the `Object.freeze` function
		auto object_constructor = env.Global().Get("Object").As<Napi::Function>();
		auto freeze             = object_constructor.Get("freeze").As<Napi::Function>();
		if (msg.magic != FRAME_TOPIC_MAGIC) {
			throw Error::New(env, "[from impl] invalid magic number");
		}
		if (msg.info.pixel_format != PixelFormat::BGR) {
			throw Error::New(env, std::format("[from impl] unsupported pixel format {}, expecting BGR", pixel_format_to_string(msg.info.pixel_format)));
		}
		if (msg.info.depth != Depth::U8) {
			throw Error::New(env, std::format("[from impl] unsupported depth {}, expecting U8", depth_to_string(msg.info.depth)));
		}

		// Create the object
		auto obj = Object::New(env);
		// https://github.com/nodejs/node-addon-api/blob/main/doc/buffer.md
		// Wraps the provided external data into a new Napi::Buffer object.
		// When the external buffer is not supported,
		// allocates a new Napi::Buffer object and copies the provided external data into it.
		if (scale_factor_ == 1.0f) {
			obj.Set("frame_count", msg.frame_count);
			obj.Set("width", msg.info.width);
			obj.Set("height", msg.info.height);
			obj.Set("channels", msg.info.channels);
			auto buffer = Napi::Buffer<uint8_t>::New(env, BgrToRgbaSize(data.size()));
			BgrToRgba(data, buffer);
			obj.Set("data", std::move(buffer));
		} else {
			// TODO: don't depend on OpenCV
			auto mat = cv::Mat(msg.info.height, msg.info.width, CV_8UC3, const_cast<uint8_t *>(data.data()));
			cv::Mat resized;
			cv::resize(mat, resized, cv::Size(), scale_factor_, scale_factor_);
			obj.Set("frame_count", msg.frame_count);
			obj.Set("width", resized.cols);
			obj.Set("height", resized.rows);
			obj.Set("channels", resized.channels());
			auto buffer = Napi::Buffer<uint8_t>::New(env, BgrToRgbaSize(resized.total() * resized.elemSize()));
			BgrToRgba({resized.data, resized.total() * resized.elemSize()}, buffer);
			obj.Set("data", std::move(buffer));
		}
		freeze.Call({obj});
		jsCallback.Call({obj});
	};

	is_being_blocking_call_ = true;
	args.msg                = msg;
	args.data               = data;
	tsfn_.BlockingCall(&args, std::move(callback));
	is_being_blocking_call_ = false;
}

inline Napi::Value FrameReceiver::Start(const Napi::CallbackInfo &info) {
	auto env = info.Env();
	if (impl_ == nullptr) {
		throw Error::New(env, "Not initialized");
	}
	if (const auto res = impl_->start()) {
		return Napi::Boolean::New(env, true);
	} else {
		throw Error::New(env, res.error());
	}
}

inline Napi::Value FrameReceiver::Stop(const Napi::CallbackInfo &info) {
	auto env = info.Env();
	if (impl_ == nullptr) {
		throw Error::New(env, "Not initialized");
	}
	impl_->stop();
	return env.Undefined();
}
inline Napi::Value FrameReceiver::SetScaleFactor(const Napi::CallbackInfo &info) {
	auto env = info.Env();
	if (info.Length() < 1) {
		throw TypeError::New(env, "Expected 1 argument (`scale_factor`)");
	}
	if (not info[0].IsNumber()) {
		throw TypeError::New(env, "`scale_factor` is expected to be number");
	}
	auto factor = info[0].As<Number>().FloatValue();
	if (factor <= 0.0f) {
		throw RangeError::New(env, "`scale_factor` is expected to be greater than 0");
	}
	if (factor > 1.0f) {
		throw RangeError::New(env, "`scale_factor` is expected to be less than or equal to 1");
	}
	scale_factor_ = factor;
	return env.Undefined();
}

inline Napi::Value FrameReceiver::SetOnFrame(const Napi::CallbackInfo &info) {
	auto env = info.Env();
	if (info.Length() < 1) {
		throw TypeError::New(env, "Expected 1 argument (`callback`)");
	}
	if (not info[0].IsFunction()) {
		throw TypeError::New(env, "`callback` is expected to be function");
	}
	auto callback = info[0].As<Function>();
	if (tsfn_ != nullptr) {
		tsfn_.Release();
	}
	tsfn_ = ThreadSafeFunction::New(env, callback, "onFrame", 16, 1);
	return env.Undefined();
}

inline FrameReceiver::FrameReceiver(const Napi::CallbackInfo &info) : Napi::ObjectWrap<FrameReceiver>(info) {
	if (info.Length() < 2) {
		throw TypeError::New(info.Env(), "Expected 2 arguments (`shm_name` and `zmq_addr`)");
	}
	if (not info[0].IsString()) {
		throw TypeError::New(info.Env(), "`shm_name` is expected to be string");
	}
	if (not info[1].IsString()) {
		throw TypeError::New(info.Env(), "`zmq_addr` is expected to be string");
	}
	auto shm_name = info[0].As<Napi::String>().Utf8Value();
	auto zmq_addr = info[1].As<Napi::String>().Utf8Value();
	impl_         = std::make_unique<FrameReceiverImpl>(shm_name, zmq_addr);
	impl_->setOnFrame([this](const sync_message_t &msg, std::span<const uint8_t> data) {
		onFrame(msg, data);
	});
}

inline Napi::Object FrameReceiver::Init(Napi::Env env, Napi::Object exports) {
	auto func = DefineClass(env, "FrameReceiver", {
													  InstanceMethod("start", &FrameReceiver::Start),
													  InstanceMethod("stop", &FrameReceiver::Stop),
													  InstanceMethod("setOnFrame", &FrameReceiver::SetOnFrame),
													  InstanceMethod("setScaleFactor", &FrameReceiver::SetScaleFactor),
												  });
	exports.Set("FrameReceiver", func);
	return exports;
}
}

#endif // FRAMERECEIVER_HPP
