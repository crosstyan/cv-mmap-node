import {createRequire} from 'module'

const IS_DEBUG = false
const require = createRequire(import.meta.url)
const addon = IS_DEBUG ? require("../build/Debug/addon.node") : require("../build/Release/addon.node")
const SHM_NAME = "/psm_default"
const ZMQ_ADDR = "ipc:///tmp/0"

const r = new addon.FrameReceiver(SHM_NAME, ZMQ_ADDR)
// frame_count
const ret = r.start()
console.log(ret)
r.setOnFrame((info) => {
    // frame count
    // width
    // height
    // channels
    // data

    // note that the consumer should be fast enough to keep up with the producer
    // otherwise the shared memory will be overwritten
    // Well, you can just copy the data to another buffer
    // but why don't you just put it to display/canvas directly?
    // console.log(info)
    const i = {
        frame_count: info.frame_count,
        width: info.width,
        height: info.height,
        channels: info.channels,
        buffer_size: info.data.length
    }
    // only takes in Buffer in Node.js
    const take_last_n = (arr, n) => {
        if (arr.length <= n) {
            return arr
        }
        return arr.subarray(arr.length - n, arr.length)
    }
    console.log(i)
    // put Node.js Buffer to it directly seems to be problematic
    const to_hex = (arr) => {
        return arr.map((x) => x.toString(16).padStart(2, '0')).join('')
    }
    // log last 40
    // https://nodejs.org/api/buffer.html#bufsubarraystart-end
    // console.info(info.data)
    const last_n = take_last_n(info.data, 40)
    console.info(to_hex([...last_n]))
})

// don't busy polling (use a promise)
// but don't exit either
while (true) {
    await new Promise(resolve => setTimeout(resolve, 1000));
}
