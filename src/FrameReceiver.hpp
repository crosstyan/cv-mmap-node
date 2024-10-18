//
// Created by crosstyan on 24-9-22.
//

#ifndef FRAMERECEIVER_HPP
#define FRAMERECEIVER_HPP
#include "FrameReceiverImpl.hpp"


#include <napi.h>
#include <zmq_addon.hpp>
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

void BgrToRgba(const std::span<const uint8_t> &src, Napi::Buffer<uint8_t> &dst) {
	const auto expected_dst_size = src.size() * 3 / 4;
	if (dst.Length() != expected_dst_size) {
		throw RangeError::New(dst.Env(), "src and dst size mismatch");
	}
	for (size_t i = 0; i < src.size(); i += 3) {
		dst[i + 0] = src[i + 2];
		dst[i + 1] = src[i + 1];
		dst[i + 2] = src[i + 0];
		dst[i + 3] = 0xff;
	}
}

class FrameReceiver final : public Napi::ObjectWrap<FrameReceiver> {
	std::unique_ptr<FrameReceiverImpl> impl_       = nullptr;
	std::unique_ptr<Napi::Buffer<uint8_t>> buffer_ = nullptr;
	ThreadSafeFunction tsfn_                       = nullptr;

	void onFrame(const sync_message_t &msg, std::span<const uint8_t> data) const;

public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	explicit FrameReceiver(const Napi::CallbackInfo &info);

	Napi::Value Start(const Napi::CallbackInfo &info);
	Napi::Value Stop(const Napi::CallbackInfo &info);
	// might need event emitter or thread safe function
	// no idea how to implement it
	Napi::Value SetOnFrame(const Napi::CallbackInfo &info);
};

inline void FrameReceiver::onFrame(const sync_message_t &msg, std::span<const uint8_t> data) const {
	if (tsfn_ == nullptr) {
		return;
	}
	struct frame_args_t {
		sync_message_t msg;
		std::span<const uint8_t> data;
	};
	auto callback = [this](Napi::Env env, Napi::Function jsCallback, frame_args_t *args_ptr) {
		const auto &args = *args_ptr;
		const auto &msg  = args.msg;
		const auto &data = args.data;
		// Get the `Object.freeze` function
		auto object_constructor = env.Global().Get("Object").As<Napi::Function>();
		auto freeze             = object_constructor.Get("freeze").As<Napi::Function>();

		// Create the object
		auto obj = Object::New(env);
		obj.Set("frame_count", msg.frame_count);
		obj.Set("width", msg.info.width);
		obj.Set("height", msg.info.height);
		obj.Set("channels", msg.info.channels);
		// https://github.com/nodejs/node-addon-api/blob/main/doc/buffer.md
		// Wraps the provided external data into a new Napi::Buffer object.
		// When the external buffer is not supported,
		// allocates a new Napi::Buffer object and copies the provided external data into it.
		const auto size = data.size() * 4 / 3;
		auto buffer     = Napi::Buffer<uint8_t>::New(env, size);
		BgrToRgba(data, buffer);
		obj.Set("data", std::move(buffer));
		freeze.Call({obj});
		jsCallback.Call({obj});
		delete args_ptr;
	};
	auto args = new frame_args_t{msg, data};
	tsfn_.BlockingCall(args, std::move(callback));
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
												  });
	exports.Set("FrameReceiver", func);
	return exports;
}
}

#endif // FRAMERECEIVER_HPP
