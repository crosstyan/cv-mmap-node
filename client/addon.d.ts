import {Buffer} from "node:buffer"

declare module "addon" {
    interface FrameInfo {
        frame_count: number;
        width: number;
        height: number;
        channels: number;
        data: Buffer;
    }

    type OnFrameCallback = (frame: FrameInfo) => void;

    export class FrameReceiver {
        constructor(shm_name: string, zmq_addr: string);

        start(): void;

        stop(): void;

        setOnFrame(onFrame: OnFrameCallback): void;
    }
}