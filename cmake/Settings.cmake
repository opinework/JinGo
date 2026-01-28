# ============================================================================
# CMake Settings - 全局编译配置
# ============================================================================

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: Enable correct __cplusplus macro value
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

# Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Android SDK 版本
set(ANDROID_MIN_SDK_VERSION 28)
set(ANDROID_TARGET_SDK_VERSION 35)
set(ANDROID_COMPILE_SDK_VERSION 35)
set(ANDROID_BUILD_TOOLS_VERSION "35.0.0")

# 默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
