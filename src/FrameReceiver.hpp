//
// Created by crosstyan on 24-9-22.
//

#ifndef FRAMERECEIVER_HPP
#define FRAMERECEIVER_HPP
#include <napi.h>
#include <zmq_addon.hpp>
#include <optional>

namespace app {
// https://github.com/nodejs/node-addon-api/blob/main/doc/object_wrap.md
// https://github.com/nodejs/nan/blob/main/doc/object_wrappers.md
// https://github.com/nodejs/nan/blob/main/doc/node_misc.md
// https://nodesource.com/blog/NAN-to-Node-API-migration-a-short-story/
class FrameReceiver : public Napi::ObjectWrap<> {
};
}

#endif // FRAMERECEIVER_HPP
