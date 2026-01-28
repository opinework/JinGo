# ============================================================================
# Dependencies-Xray.cmake - SuperRay Library Configuration
# ============================================================================
# This module handles:
# - SuperRay library path configuration for all platforms
# - Platform-specific library selection (Windows .dll, Apple XCFramework, Linux .so)
# - XCFramework selection for iOS/macOS
# - Include directories configuration
#
# Directory Structure (third_party/superray/):
#   apple/
#     SuperRay.xcframework/          # iOS + macOS 通用框架
#       ios-arm64/
#       ios-arm64_x86_64-simulator/
#       macos-arm64_x86_64/
#   darwin/                          # macOS 动态库（备用）
#     include/superray.h
#     amd64/libsuperray.dylib
#     arm64/libsuperray.dylib
#     universal/libsuperray.dylib
#   linux/
#     include/superray.h
#     amd64/libsuperray.so
#     arm64/libsuperray.so
#   windows/
#     include/superray.h
#     amd64/superray.dll
#
# Variables set by this module:
#   SUPERRAY_ROOT - Root directory of superray
#   SUPERRAY_INCLUDE_DIR - Include directory for superray headers
#   SUPERRAY_LIB - Library file path or imported target name
#   SUPERRAY_DLL - DLL file path (Windows only)
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Configuring SuperRay Dependencies")
message(STATUS "========================================")

# ============================================================================
# SuperRay Root Directory
# ============================================================================

set(SUPERRAY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray"
    CACHE PATH "SuperRay root directory")

message(STATUS "SuperRay root: ${SUPERRAY_ROOT}")

# ============================================================================
# Platform-Specific Library Configuration
# ============================================================================

if(WIN32)
    # ========================================================================
    # Windows: .dll (WinTun driver is embedded)
    # ========================================================================
    set(SUPERRAY_LIBRARY_DIR "${SUPERRAY_ROOT}/windows/amd64")
    set(SUPERRAY_LIB "${SUPERRAY_LIBRARY_DIR}/superray.dll")
    set(SUPERRAY_DLL "${SUPERRAY_LIBRARY_DIR}/superray.dll")
    set(SUPERRAY_INCLUDE_DIR "${SUPERRAY_ROOT}/windows/include"
        CACHE PATH "SuperRay include directory" FORCE)

    message(STATUS "Windows SuperRay configuration:")
    message(STATUS "  DLL: ${SUPERRAY_DLL}")
    message(STATUS "  Headers: ${SUPERRAY_INCLUDE_DIR}")

elseif(TARGET_MACOS OR TARGET_IOS OR APPLE)
    # ========================================================================
    # macOS and iOS: Static Library (.a)
    # iOS: uses SuperRay.xcframework
    # macOS: uses standalone static library (not in xcframework)
    # ========================================================================
    set(SUPERRAY_XCFRAMEWORK_DIR "${SUPERRAY_ROOT}/apple/SuperRay.xcframework")

    if(TARGET_IOS OR IOS)
        # iOS: Select appropriate library from XCFramework
        if(CMAKE_OSX_SYSROOT MATCHES ".*simulator.*" OR PLATFORM MATCHES ".*SIMULATOR.*")
            # iOS Simulator
            set(SUPERRAY_LIBRARY_DIR "${SUPERRAY_XCFRAMEWORK_DIR}/ios-arm64-simulator")
            message(STATUS "iOS Simulator detected")
        else()
            # iOS Device
            set(SUPERRAY_LIBRARY_DIR "${SUPERRAY_XCFRAMEWORK_DIR}/ios-arm64")
            message(STATUS "iOS Device detected")
        endif()
        # Use static library from XCFramework
        set(SUPERRAY_LIB "${SUPERRAY_LIBRARY_DIR}/libsuperray.a")
        # Get Headers from XCFramework
        set(SUPERRAY_INCLUDE_DIR "${SUPERRAY_LIBRARY_DIR}/Headers"
            CACHE PATH "SuperRay include directory for iOS" FORCE)

        message(STATUS "iOS SuperRay XCFramework configuration:")
        message(STATUS "  XCFramework: ${SUPERRAY_XCFRAMEWORK_DIR}")
        message(STATUS "  Selected Library: ${SUPERRAY_LIBRARY_DIR}")
        message(STATUS "  Library File: ${SUPERRAY_LIB}")
        message(STATUS "  Headers: ${SUPERRAY_INCLUDE_DIR}")
    else()
        # macOS: Use static library from xcframework
        set(SUPERRAY_LIBRARY_DIR "${SUPERRAY_XCFRAMEWORK_DIR}/macos-arm64_x86_64")
        set(SUPERRAY_LIB "${SUPERRAY_LIBRARY_DIR}/libsuperray.a")
        set(SUPERRAY_INCLUDE_DIR "${SUPERRAY_LIBRARY_DIR}/Headers"
            CACHE PATH "SuperRay include directory for macOS" FORCE)

        message(STATUS "macOS SuperRay XCFramework configuration:")
        message(STATUS "  Library Dir: ${SUPERRAY_LIBRARY_DIR}")
        message(STATUS "  Library File: ${SUPERRAY_LIB}")
        message(STATUS "  Headers: ${SUPERRAY_INCLUDE_DIR}")
    endif()

elseif(TARGET_ANDROID OR ANDROID)
    # ========================================================================
    # Android: Currently not pre-built
    # Android .so files need to be built separately
    # ========================================================================
    message(STATUS "Android platform detected")

    # Try to find Android library in standard locations
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(SUPERRAY_ANDROID_ABI_DIR "arm64")
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(SUPERRAY_ANDROID_ABI_DIR "arm")
    elseif(ANDROID_ABI STREQUAL "x86_64")
        set(SUPERRAY_ANDROID_ABI_DIR "x86_64")
    elseif(ANDROID_ABI STREQUAL "x86")
        set(SUPERRAY_ANDROID_ABI_DIR "x86")
    else()
        set(SUPERRAY_ANDROID_ABI_DIR "${ANDROID_ABI}")
    endif()

    # Check for library in android/ directory
    set(SUPERRAY_ANDROID_SO "${SUPERRAY_ROOT}/android/${SUPERRAY_ANDROID_ABI_DIR}/libsuperray.so")

    if(EXISTS "${SUPERRAY_ANDROID_SO}")
        set(SUPERRAY_LIB "${SUPERRAY_ANDROID_SO}")
        set(SUPERRAY_INCLUDE_DIR "${SUPERRAY_ROOT}/android/include"
            CACHE PATH "SuperRay include directory for Android" FORCE)

        # Create imported target
        # Note: IMPORTED_NO_SONAME=TRUE is crucial to avoid embedding the full build path
        add_library(superray_imported SHARED IMPORTED)
        set_target_properties(superray_imported PROPERTIES
            IMPORTED_LOCATION "${SUPERRAY_LIB}"
            IMPORTED_NO_SONAME TRUE
        )

        message(STATUS "Android SuperRay configuration:")
        message(STATUS "  Library: ${SUPERRAY_LIB}")
        message(STATUS "  Headers: ${SUPERRAY_INCLUDE_DIR}")
    else()
        # Android library not found - need to build separately
        message(WARNING "SuperRay Android library not found at: ${SUPERRAY_ANDROID_SO}")
        message(WARNING "Android VPN functionality requires building SuperRay for Android")
        message(WARNING "Expected path: ${SUPERRAY_ROOT}/android/${SUPERRAY_ANDROID_ABI_DIR}/libsuperray.so")

        set(SUPERRAY_LIB "")
        set(SUPERRAY_INCLUDE_DIR "${SUPERRAY_ROOT}/linux/include"
            CACHE PATH "SuperRay include directory (fallback)" FORCE)
    endif()

else()
    # ========================================================================
    # Linux: .so shared library
    # ========================================================================
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
        set(SUPERRAY_ARCH "arm64")
    else()
        set(SUPERRAY_ARCH "amd64")
    endif()

    set(SUPERRAY_LIBRARY_DIR "${SUPERRAY_ROOT}/linux/${SUPERRAY_ARCH}")
    set(SUPERRAY_LIB "${SUPERRAY_LIBRARY_DIR}/libsuperray.so")
    set(SUPERRAY_INCLUDE_DIR "${SUPERRAY_ROOT}/linux/include"
        CACHE PATH "SuperRay include directory for Linux" FORCE)

    message(STATUS "Linux SuperRay configuration:")
    message(STATUS "  Architecture: ${SUPERRAY_ARCH}")
    message(STATUS "  Library: ${SUPERRAY_LIB}")
    message(STATUS "  Headers: ${SUPERRAY_INCLUDE_DIR}")
endif()

# ============================================================================
# Verification
# ============================================================================

if(TARGET superray_imported)
    message(STATUS "SuperRay configured: Imported target (Android)")
elseif(SUPERRAY_LIB AND EXISTS "${SUPERRAY_LIB}")
    message(STATUS "SuperRay library found: ${SUPERRAY_LIB}")
else()
    if(TARGET_ANDROID OR ANDROID)
        message(WARNING "SuperRay Android library NOT found")
        message(WARNING "  Please build SuperRay for Android or place library at expected path")
    else()
        message(WARNING "SuperRay library NOT found at: ${SUPERRAY_LIB}")
        message(WARNING "  XrayCore functionality may not work")
    endif()
endif()

# Verify header exists
if(EXISTS "${SUPERRAY_INCLUDE_DIR}/superray.h")
    message(STATUS "SuperRay header found: ${SUPERRAY_INCLUDE_DIR}/superray.h")
else()
    message(WARNING "SuperRay header NOT found at: ${SUPERRAY_INCLUDE_DIR}/superray.h")
endif()

message(STATUS "SuperRay dependencies configured")
message(STATUS "========================================")
message(STATUS "")
