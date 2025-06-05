cmake_minimum_required(VERSION 3.20)
project(WebGPUTest)

### Build settings
set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    webgpu-utils.cpp
    webgpu-cpp-implementations.cpp
    main.cpp)

set(HEADERS
    webgpu-utils.h
    application.h
)

add_executable(wgputest
    ${SOURCES} ${HEADERS})

set_target_properties(wgputest PROPERTIES
    COMPILE_WARNING_AS_ERROR ON
)

# enable FetchContent
include(FetchContent)

# glfw
FetchContent_Declare(glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(glfw3)

#webgpu
set(WEBGPU_BACKEND "DAWN")

# Fetch from https://github.com/eliemichel/WebGPU-distribution
FetchContent_Declare(
    webgpu
    GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
    GIT_TAG v0.3.0-gamma
)
FetchContent_MakeAvailable(webgpu)

FetchContent_Declare(
    glfw3webgpu
    GIT_REPOSITORY https://github.com/eliemichel/glfw3webgpu
    GIT_TAG v1.3.0-alpha
)
FetchContent_MakeAvailable(glfw3webgpu)

# Add the WebGPU, glfw and glfw3webgpu library
target_link_libraries(wgputest
    PRIVATE webgpu glfw glfw3webgpu
)

# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(wgputest)
