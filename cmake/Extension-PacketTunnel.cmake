# ============================================================================
# Extension-PacketTunnel.cmake - PacketTunnelProvider Network Extension
# ============================================================================
# This module configures the PacketTunnelProvider Network Extension for iOS/macOS.
#
# Architecture:
#   - NEPacketTunnelProvider implementation (TUN mode)
#   - SuperRay - 统一的Xray-core和TUN处理库
#   - XrayCBridge - Extension-internal Xray management (使用SuperRay SuperRay兼容API)
#   - Xray runs inside Extension process via SuperRay
#
# Usage:
#   if(TARGET_MACOS OR TARGET_IOS)
#       include(cmake/Extension-PacketTunnel.cmake)
#   endif()
#
# Requirements:
#   - TARGET_MACOS or TARGET_IOS must be set
#   - MACOS_TEAM_ID or IOS_TEAM_ID must be defined for code signing
#   - SUPERRAY_LIB (SuperRay.xcframework) should be available
#
# Targets created:
#   - PacketTunnelProvider - Network Extension target
# ============================================================================

if(NOT (TARGET_MACOS OR TARGET_IOS))
    message(WARNING "Extension-PacketTunnel.cmake included but neither TARGET_MACOS nor TARGET_IOS is set")
    return()
endif()

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Building PacketTunnelProvider Extension (TUN Mode)")
message(STATUS "========================================")

# ============================================================================
# Extension Source Files
# ============================================================================
# Note: XrayCore runs inside Extension, not in main process
# Extension includes:
#   1. PacketTunnelProvider - NEPacketTunnelProvider implementation (TUN mode)
#   2. XrayCBridge - Xray management inside Extension (uses SuperRay SuperRay API)
#
# IMPORTANT: iOS App Extension uses _NSExtensionMain as entry point (no main.m needed)
#            macOS System Extension uses startSystemExtensionMode (needs main.m)
#
# Note: TUN processing is now handled by SuperRay library directly

# 当使用 JinDoCore 库时，从 JinDo 目录获取源文件
if(USE_JINDO_LIB AND JINDO_ROOT)
    set(EXT_SRC_DIR "${JINDO_ROOT}/src")
else()
    set(EXT_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
endif()

if(TARGET_IOS)
    # iOS: No main.m needed - system uses _NSExtensionMain entry point
    set(EXTENSION_SOURCES
        ${EXT_SRC_DIR}/extensions/PacketTunnelProvider/PacketTunnelProvider.mm
        ${EXT_SRC_DIR}/extensions/PacketTunnelProvider/XrayExtensionBridge.mm
        ${EXT_SRC_DIR}/core/XrayCBridge_Apple.mm
    )
else()
    # macOS: Needs main.m for System Extension mode
    set(EXTENSION_SOURCES
        ${EXT_SRC_DIR}/extensions/PacketTunnelProvider/main.m
        ${EXT_SRC_DIR}/extensions/PacketTunnelProvider/PacketTunnelProvider.mm
        ${EXT_SRC_DIR}/extensions/PacketTunnelProvider/XrayExtensionBridge.mm
        ${EXT_SRC_DIR}/core/XrayCBridge_Apple.mm
    )
endif()

message(STATUS "Extension source files:")
foreach(source ${EXTENSION_SOURCES})
    message(STATUS "  - ${source}")
endforeach()

# ============================================================================
# Create Network Extension Target
# ============================================================================
# IMPORTANT: NetworkExtension needs EXECUTABLE type (MH_EXECUTE), not MODULE (MH_BUNDLE)
# Reference: Shadowrocket and other working VPN extensions use executable type

add_executable(PacketTunnelProvider MACOSX_BUNDLE ${EXTENSION_SOURCES})

# Workaround for Qt 6.8+ adding -no_warn_duplicate_libraries which Xcode clang doesn't understand
set_target_properties(PacketTunnelProvider PROPERTIES
    QT_NO_DISABLE_WARN_DUPLICATE_LIBRARIES ON
)

# ============================================================================
# Set Bundle Properties
# ============================================================================

# Note: BUNDLE_EXTENSION is set in platform-specific properties below

# ============================================================================
# Platform-Specific Bundle Properties
# ============================================================================

if(TARGET_IOS)
    # ========================================================================
    # iOS Configuration
    # ========================================================================
    # CI 环境：禁用 Xcode 内置签名，由构建脚本手动签名
    if(BUILD_KEYCHAIN_PATH)
        set_target_properties(PacketTunnelProvider PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/extensions/PacketTunnelProvider/Info-iOS.plist
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${PACKET_TUNNEL_BUNDLE_ID}"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}"
            XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_EXTENSION "appex"
        )
    else()
        # 本地开发环境
        set_target_properties(PacketTunnelProvider PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/extensions/PacketTunnelProvider/Info-iOS.plist
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${PACKET_TUNNEL_BUNDLE_ID}"
            XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/PacketTunnelProvider.entitlements"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${APPLE_CODE_SIGN_IDENTITY}"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
            XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${IOS_PROFILE_PACKET_TUNNEL}"
            XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_EXTENSION "appex"
        )
    endif()
    message(STATUS "iOS PacketTunnelProvider configuration:")
    message(STATUS "  Bundle ID: ${PACKET_TUNNEL_BUNDLE_ID}")
    message(STATUS "  Team ID: ${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}")
    message(STATUS "  Deployment Target: iOS 14.0+")
    message(STATUS "  Devices: iPhone & iPad")

else()
    # ========================================================================
    # macOS Configuration
    # ========================================================================
    # CI 环境：禁用 Xcode 内置签名，由构建脚本手动签名
    if(BUILD_KEYCHAIN_PATH)
        set_target_properties(PacketTunnelProvider PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/extensions/PacketTunnelProvider/Info.plist
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${PACKET_TUNNEL_BUNDLE_ID}"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${MACOS_TEAM_ID}"
            XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
            BUNDLE_EXTENSION "systemextension"
        )
    else()
        # 本地开发环境
        set_target_properties(PacketTunnelProvider PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/extensions/PacketTunnelProvider/Info.plist
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${PACKET_TUNNEL_BUNDLE_ID}"
            XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/PacketTunnelProvider.entitlements"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${MACOS_TEAM_ID}"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
            XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${MACOS_PROFILE_PACKET_TUNNEL}"
            XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
            BUNDLE_EXTENSION "systemextension"
        )
    endif()
    message(STATUS "macOS PacketTunnelProvider configuration:")
    message(STATUS "  Bundle ID: ${PACKET_TUNNEL_BUNDLE_ID}")
    message(STATUS "  Team ID: ${MACOS_TEAM_ID}")
    message(STATUS "  Hardened Runtime: YES")
endif()

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(PacketTunnelProvider PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/extensions/PacketTunnelProvider
)

# Add SuperRay include if available
if(EXISTS ${SUPERRAY_INCLUDE_DIR})
    target_include_directories(PacketTunnelProvider PRIVATE ${SUPERRAY_INCLUDE_DIR})
endif()

# ============================================================================
# Compiler Options
# ============================================================================

# Enable ARC for Objective-C/C++ files
target_compile_options(PacketTunnelProvider PRIVATE
    $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
    $<$<COMPILE_LANGUAGE:OBJC>:-fobjc-arc>
)

# iOS/macOS requires -fmodules for @import syntax in SuperRay headers
if(APPLE AND NOT ANDROID)
    # Directly set fmodules and fcxx-modules for all source files
    target_compile_options(PacketTunnelProvider PRIVATE -fmodules -fcxx-modules)
    # Also set Xcode attributes for modules and ARC
    set_target_properties(PacketTunnelProvider PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )
endif()

# ============================================================================
# Link Frameworks
# ============================================================================

if(TARGET_IOS)
    # iOS frameworks
    target_link_libraries(PacketTunnelProvider PRIVATE
        "-framework NetworkExtension"
        "-framework Foundation"
        "-framework Security"
        "-framework SystemConfiguration"
        "-framework UIKit"
        "-framework CoreTelephony"
    )
    # CRITICAL: iOS App Extension must use _NSExtensionMain as entry point
    # This is the standard entry point for iOS Network Extensions provided by NetworkExtension.framework
    # Using target_link_options is the proper CMake way to set this (instead of modifying pbxproj)
    target_link_options(PacketTunnelProvider PRIVATE "-e" "_NSExtensionMain")
    message(STATUS "iOS frameworks linked")
    message(STATUS "iOS entry point: _NSExtensionMain (set via target_link_options)")
else()
    # macOS frameworks
    target_link_libraries(PacketTunnelProvider PRIVATE
        "-framework NetworkExtension"
        "-framework Foundation"
        "-framework Security"
        "-framework SystemConfiguration"
    )
    message(STATUS "macOS frameworks linked")
endif()

# ============================================================================
# Link SuperRay Library (XrayCore + TUN)
# ============================================================================
# NOTE: New architecture - SuperRay runs inside Extension
# SuperRay provides both Xray-core and TUN processing functionality
# Extension contains complete Xray instance listening on 127.0.0.1:10808
# TUN mode: SuperRay handles TUN device creation and packet processing

if(USE_SUPERRAY AND EXISTS ${SUPERRAY_LIB})
    message(STATUS "Linking SuperRay to Extension: ${SUPERRAY_LIB}")
    target_link_directories(PacketTunnelProvider PRIVATE "${SUPERRAY_LIBRARY_DIR}")
    target_link_options(PacketTunnelProvider PRIVATE "-F${SUPERRAY_LIBRARY_DIR}")
    target_link_libraries(PacketTunnelProvider PRIVATE
        "${SUPERRAY_LIB}"
        "-lz"
        "-lresolv"
    )
    target_compile_definitions(PacketTunnelProvider PRIVATE
        HAVE_SUPERRAY
        NETWORK_EXTENSION_TARGET
        APPLE_XRAY_BRIDGE
    )
else()
    message(WARNING "SuperRay not available for Extension")
    target_compile_definitions(PacketTunnelProvider PRIVATE NO_SUPERRAY)
endif()

# ============================================================================
# Note: TUN processing is handled by SuperRay library
# ============================================================================
# SuperRay integrates both Xray-core and TUN functionality
# No separate hev-socks5-tunnel linking is required

# ============================================================================
# Info.plist 依赖配置
# ============================================================================
# 注意: Info.plist 的刷新由构建脚本 (build-macos.sh, build-ios.sh) 处理
# 脚本会在构建前将最新的 Info.plist 复制到 CMake 缓存位置
# 这里只添加文件依赖，确保 Info.plist 修改后触发重新构建

if(TARGET_IOS)
    set(EXTENSION_INFO_PLIST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/extensions/PacketTunnelProvider/Info-iOS.plist")
else()
    set(EXTENSION_INFO_PLIST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/extensions/PacketTunnelProvider/Info.plist")
endif()

# 添加 Info.plist 作为依赖
set_property(TARGET PacketTunnelProvider APPEND PROPERTY
    OBJECT_DEPENDS "${EXTENSION_INFO_PLIST_SOURCE}"
)

# ============================================================================
# Platform-Specific Installation and Post-Build Steps
# ============================================================================

if(TARGET_IOS)
    # ========================================================================
    # iOS Post-Build: Copy Extension to App Bundle
    # ========================================================================
    # CRITICAL: iOS extensions must be copied to JinGo.app/PlugIns/ during build
    # Using POST_BUILD ensures extensions are embedded before code signing

    add_custom_command(TARGET PacketTunnelProvider POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying PacketTunnelProvider Extension to iOS app bundle..."
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:JinGo>/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_DIR:JinGo>/PlugIns/PacketTunnelProvider.appex"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>"
            "$<TARGET_BUNDLE_DIR:JinGo>/PlugIns/PacketTunnelProvider.appex"
        COMMAND ${CMAKE_COMMAND} -E echo "✓ PacketTunnelProvider Extension copied to JinGo.app/PlugIns/PacketTunnelProvider.appex"
        COMMENT "Copying PacketTunnelProvider Extension to iOS app bundle"
        DEPENDS JinGo
        VERBATIM
    )

    # Also copy after JinGo target is built
    add_custom_command(TARGET JinGo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Ensuring PacketTunnelProvider Extension is in JinGo.app..."
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:JinGo>/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_DIR:JinGo>/PlugIns/PacketTunnelProvider.appex"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>"
            "$<TARGET_BUNDLE_DIR:JinGo>/PlugIns/PacketTunnelProvider.appex"
        COMMAND ${CMAKE_COMMAND} -E echo "✓ PacketTunnelProvider Extension embedded in JinGo.app"
        COMMENT "Embedding PacketTunnelProvider Extension in JinGo.app"
        DEPENDS PacketTunnelProvider
        VERBATIM
    )

    # Ensure Extension is built before main app
    add_dependencies(JinGo PacketTunnelProvider)

    # Install target (for cmake --install, not used by normal build)
    install(TARGETS PacketTunnelProvider
        BUNDLE DESTINATION JinGo.app/PlugIns
    )

    # ========================================================================
    # iOS Post-Build: Copy GeoIP Files to Extension
    # ========================================================================
    # IMPORTANT: iOS Extension DOES need GeoIP files because Xray runs inside
    # the extension process via SuperRay. Without these files, Xray routing
    # will not work properly.
    #
    # Note: This increases extension size by ~21MB but is required for proper
    # operation of the TUN-based VPN.

    add_custom_command(TARGET PacketTunnelProvider POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying GeoIP data files to iOS PacketTunnelProvider Extension..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/geoip/geoip.dat"
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>/geoip.dat"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/geoip/geosite.dat"
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>/geosite.dat"
        COMMAND ${CMAKE_COMMAND} -E echo "✓ GeoIP files copied to iOS Extension bundle root"
        COMMENT "Copying GeoIP data files to iOS PacketTunnelProvider Extension"
    )

    message(STATUS "iOS Network Extension configured")
    message(STATUS "  Build-time copy to: JinGo.app/PlugIns")
    message(STATUS "  GeoIP files: copied to extension bundle root")

else()
    # ========================================================================
    # macOS Post-Build: Copy GeoIP Files
    # ========================================================================
    # IMPORTANT: Copy GeoIP files during Extension's own build
    # This ensures files exist in Extension before it's copied to JinGo.app and signed

    add_custom_command(TARGET PacketTunnelProvider POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying GeoIP data files to PacketTunnelProvider Extension..."
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>/Contents/Resources/dat"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/geoip/geoip.dat"
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>/Contents/Resources/dat/geoip.dat"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/geoip/geosite.dat"
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>/Contents/Resources/dat/geosite.dat"
        COMMAND ${CMAKE_COMMAND} -E echo " GeoIP files copied to PacketTunnelProvider Extension"
        COMMENT "Copying GeoIP data files to PacketTunnelProvider Extension Resources"
    )

    # ========================================================================
    # macOS Post-Build: Copy System Extension to App Bundle
    # ========================================================================
    # System Extensions must be placed in Contents/Library/SystemExtensions/
    # and use full bundle ID as filename: cfd.jingo.acc.PacketTunnelProvider.systemextension

    add_custom_command(TARGET PacketTunnelProvider POST_BUILD
        # Copy System Extension to app bundle
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Library/SystemExtensions"
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Library/SystemExtensions/${PACKET_TUNNEL_BUNDLE_ID}.systemextension"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>"
            "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Library/SystemExtensions/${PACKET_TUNNEL_BUNDLE_ID}.systemextension"
        COMMENT "Copying System Extension to app bundle"
    )

    # Also copy after JinGo target is built
    add_custom_command(TARGET JinGo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Library/SystemExtensions"
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Library/SystemExtensions/${PACKET_TUNNEL_BUNDLE_ID}.systemextension"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:PacketTunnelProvider>"
            "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Library/SystemExtensions/${PACKET_TUNNEL_BUNDLE_ID}.systemextension"
        COMMAND ${CMAKE_COMMAND} -E echo "System Extension copied to JinGo.app/Contents/Library/SystemExtensions/${PACKET_TUNNEL_BUNDLE_ID}.systemextension"
        COMMENT "Copying System Extension to JinGo.app/Contents/Library/SystemExtensions"
        VERBATIM
    )

    message(STATUS "macOS System Extension configured")
    message(STATUS "  Install location: JinGo.app/Contents/Library/SystemExtensions")

    # Ensure Extension is built before main app
    add_dependencies(JinGo PacketTunnelProvider)

    # ========================================================================
    # Code Signing Note
    # ========================================================================
    if(MACOS_TEAM_ID)
        message(STATUS "")
        message(STATUS "Automatic Code Signing Configuration:")
        message(STATUS "  Identity: ${APPLE_CODE_SIGN_IDENTITY}")
        message(STATUS "  Team ID: ${MACOS_TEAM_ID}")
        message(STATUS "")
        message(STATUS "  NOTE: All code signing (frameworks, extensions, app) is done in")
        message(STATUS "        the post-macdeployqt phase to avoid signature invalidation.")
        message(STATUS "        See cmake/Signing-Apple.cmake for signing logic.")
    else()
        message(WARNING "")
        message(WARNING "MACOS_TEAM_ID not set - skipping automatic code signing")
        message(WARNING "  Extension entitlements will NOT be embedded automatically")
        message(WARNING "  To enable automatic signing, set:")
        message(WARNING "    cmake -DMACOS_TEAM_ID=YOUR_TEAM_ID ...")
        message(WARNING "  Or run scripts/sign_app.sh manually after build")
    endif()
endif()

message(STATUS "========================================")
message(STATUS "")
