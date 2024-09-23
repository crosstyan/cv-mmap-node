#include <napi.h>
#include "FrameReceiver.hpp"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	app::FrameReceiver::Init(env, exports);
	return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
