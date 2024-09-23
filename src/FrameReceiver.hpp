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
// https://github.com/nodejs/node-addon-api/blob/main/doc/object_wrap.md
// https://nodesource.com/blog/NAN-to-Node-API-migration-a-short-story/
// https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe_function.md
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
		Napi::TypeError::New(env, "Not initialized").ThrowAsJavaScriptException();
		return env.Undefined();
	}
	auto res = impl_->start();
	if (res) {
		return Napi::Boolean::New(env, true);
	} else {
		Napi::TypeError::New(env, res.error()).ThrowAsJavaScriptException();
		return env.Undefined();
	}
}

inline Napi::Value FrameReceiver::Stop(const Napi::CallbackInfo &info) {
	auto env = info.Env();
	if (impl_ == nullptr) {
		Napi::TypeError::New(env, "Not initialized").ThrowAsJavaScriptException();
		return env.Undefined();
	}
	impl_->stop();
	return env.Undefined();
}

inline FrameReceiver::FrameReceiver(const Napi::CallbackInfo &info) : Napi::ObjectWrap<FrameReceiver>(info) {
	if (info.Length() < 2) {
		Napi::TypeError::New(info.Env(), "Expected 2 arguments (`shm_name` and `zmq_addr`)").ThrowAsJavaScriptException();
		return;
	}
	if (not info[0].IsString()){
		Napi::TypeError::New(info.Env(), "`shm_name` is expected to be string").ThrowAsJavaScriptException();
		return;
	}
	if (not info[1].IsString()){
		Napi::TypeError::New(info.Env(), "`zmq_addr` is expected to be string").ThrowAsJavaScriptException();
		return;
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
