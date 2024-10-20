cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

# Name of the project (will be the name of the plugin)
project(addon)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DNAPI_VERSION=8 -DNAPI_CPP_EXCEPTIONS)

find_package(cppzmq)
find_package(OpenCV REQUIRED)

# ----------- Node Addon Setup -----------
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
target_sources(${PROJECT_NAME} PRIVATE
        src/addon.cc
        src/FrameReceiver.hpp)
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# See also `cmake-js --print-configurations`
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
target_link_libraries(${PROJECT_NAME} cppzmq)

target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

if (MSVC
        AND CMAKE_JS_NODELIB_DEF
        AND CMAKE_JS_NODELIB_TARGET)
    # Generate node.lib
    execute_process(
            COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF}
            /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif ()
# ----------- End of Node Addon Setup -----------

# ----------- Tester Setup -----------
add_executable(tester src/tester.cpp src/FrameReceiverImpl.hpp)
target_include_directories(tester PRIVATE src)
target_link_libraries(tester cppzmq)
target_compile_definitions(tester PRIVATE -DTRACE_LOG_STDOUT)
# ----------- End of Tester Setup -----------
