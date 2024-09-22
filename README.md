# cmake-js example

This is an example project that uses [cmake-js](https://github.com/cmake-js/cmake-js)

## IDE

use `cmake-js build` to build.

use `cmake-js --print-configuration` to print the cmake configuration parameters, which could be used in your IDE.

```txt
# cmake ..
--no-warn-unused-cli,
-GNinja,
-DCMAKE_JS_VERSION=7.3.0,
-DCMAKE_BUILD_TYPE=Release,
-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=/home/crosstyan/Code/cv-mmap-node/build/Release,
-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>,
-DCMAKE_JS_INC=/home/crosstyan/.cmake-js/node-x64/v22.7.0/include/node;/home/crosstyan/Code/cv-mmap-node/node_modules/nan,
-DCMAKE_JS_SRC=,
-DNODE_RUNTIME=node,
-DNODE_RUNTIMEVERSION=22.7.0,
-DNODE_ARCH=x64,
-DCMAKE_JS_LIB=,
-DCMAKE_CXX_FLAGS=-DBUILDING_NODE_EXTENSION
```

## Reference

- [Creating a native module by using CMake.js and NAN](https://github.com/cmake-js/cmake-js/wiki/TUTORIAL-01-Creating-a-native-module-by-using-CMake.js-and-NAN)
- [Developing with CLion](https://github.com/cmake-js/cmake-js/issues/23)
