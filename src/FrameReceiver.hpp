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
using Napi::TypeError;
using Napi::Value;
using Napi::Object;
using Context = Reference<Value>;

// https://github.com/nodejs/node-addon-api/blob/main/doc/object_wrap.md
// https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe_function.md
// https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe.md
// https://github.com/nodejs/node-addon-api/blob/main/doc/typed_threadsafe_function.md

class FrameReceiver : public Napi::ObjectWrap<FrameReceiver> {
	std::unique_ptr<FrameReceiverImpl> impl_ = nullptr;

public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	explicit FrameReceiver(const Napi::CallbackInfo &info);

	Napi::Value Start(const Napi::CallbackInfo &info);
	Napi::Value Stop(const Napi::CallbackInfo &info);
	// might need event emitter or thread safe function
	// no idea how to implement it
	Napi::Value SetOnFrame(const Napi::CallbackInfo &info);
};
// https://gitlab.ifi.uzh.ch/scheid/bcoln/-/blob/619c184df737026b2c7ae222d76da7435f101d9a/Web3/web3js/node_modules/node-addon-api/doc/object_wrap.md

inline Napi::Value FrameReceiver::Start(const Napi::CallbackInfo &info) {
	auto env = info.Env();
	if (impl_ == nullptr) {
		throw TypeError::New(env, "Not initialized");
	}
	auto res = impl_->start();
	if (res) {
		return Napi::Boolean::New(env, true);
	} else {
		throw TypeError::New(env, res.error());
	}
}

inline Napi::Value FrameReceiver::Stop(const Napi::CallbackInfo &info) {
	auto env = info.Env();
	if (impl_ == nullptr) {
		throw TypeError::New(env, "Not initialized");
	}
	impl_->stop();
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
}

inline Napi::Object FrameReceiver::Init(Napi::Env env, Napi::Object exports) {
	Napi::Function func = DefineClass(env, "FrameReceiver", {
																InstanceMethod("start", &FrameReceiver::Start),
																InstanceMethod("stop", &FrameReceiver::Stop),
															});
	exports.Set("FrameReceiver", func);
	return exports;
}
}

#endif // FRAMERECEIVER_HPP
