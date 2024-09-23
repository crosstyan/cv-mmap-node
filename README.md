# cmake-js example

This is an example project that uses [cmake-js](https://github.com/cmake-js/cmake-js)

## IDE

use `cmake-js build` to build.

use `cmake-js --print-configuration` to print the cmake configuration parameters, which could be used in your IDE.

```log
info TOOL Using Ninja generator, because ninja is available.
info CMD CONFIGURE
info RUN [
info RUN   'cmake',
info RUN   '/Users/crosstyan/Code/cv-mmap-node',
info RUN   '--no-warn-unused-cli',
info RUN   '-G',
info RUN   'Ninja',
info RUN   '-DCMAKE_JS_VERSION=7.3.0',
info RUN   '-DCMAKE_BUILD_TYPE=Release',
info RUN   '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=/Users/crosstyan/Code/cv-mmap-node/build/Release',
info RUN   '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>',
info RUN   '-DCMAKE_JS_INC=/Users/crosstyan/Code/cv-mmap-node/node_modules/.pnpm/node-api-headers@1.3.0/node_modules/node-api-headers/include;/Users/crosstyan/Code/cv-mmap-node/node_modules/.pnpm/node-addon-api@8.1.0/node_modules/node-addon-api',
info RUN   '-DCMAKE_JS_SRC=',
info RUN   '-DNODE_RUNTIME=node',
info RUN   '-DNODE_RUNTIMEVERSION=22.9.0',
info RUN   '-DNODE_ARCH=arm64',
info RUN   '-DCMAKE_OSX_ARCHITECTURES=arm64',
info RUN   '-DCMAKE_JS_LIB=',
info RUN   '-DCMAKE_CXX_FLAGS=-D_DARWIN_USE_64_BIT_INODE=1 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DBUILDING_NODE_EXTENSION',
info RUN   '-DCMAKE_SHARED_LINKER_FLAGS=-undefined dynamic_lookup'
info RUN ]
```

In practice, I just set `CMAKE_JS_INC` to get the correct include path. I still use `cmake-js build` to build the project, instead of
the IDE's build command.

## Reference

- [`napi-rs`](https://github.com/napi-rs/napi-rs)
- [N-API documentation on CMake.js](https://github.com/nodejs/node-addon-api/blob/main/doc/cmake-js.md)
- [~~Creating a native module by using CMake.js and NAN~~](https://github.com/cmake-js/cmake-js/wiki/TUTORIAL-01-Creating-a-native-module-by-using-CMake.js-and-NAN) don't use Nan
- [examples/async_pi_estimate](https://github.com/nodejs/nan/tree/main/examples/async_pi_estimate)
- [khomin/electron_ffmpeg_addon_camera](https://github.com/khomin/electron_ffmpeg_addon_camera)
- [Developing with CLion](https://github.com/cmake-js/cmake-js/issues/23)
- [How to Import Native Modules using the new ES6 Module Syntax](https://medium.com/the-node-js-collection/how-to-import-native-modules-using-the-new-es6-module-syntax-426ca3c44bed)
