cmake_minimum_required(VERSION 3.20)

# Name of the project (will be the name of the plugin)
project(addon)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build a shared library named after the project from the files in `src/`
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
target_sources(${PROJECT_NAME} PRIVATE 
    src/addon.cc
    src/async.cc
    src/sync.cc
    src/pi_est.cc
)
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# use `--print-configurations` to see the exact value of `CMAKE_JS_INC` and `CMAKE_JS_LIB`
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
