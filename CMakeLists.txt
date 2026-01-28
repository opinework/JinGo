# CMakeLists.txt - JinGo VPN Client
cmake_minimum_required(VERSION 3.21)

include(cmake/Policies.cmake)
include(cmake/Settings.cmake)

if(ANDROID)
    include(cmake/Platform-Android.cmake)
endif()

project(JinGo
    VERSION 1.0.0
    DESCRIPTION "Cross-platform Xray VPN Client - JinGo"
    LANGUAGES C CXX
)

if(APPLE AND NOT ANDROID)
    enable_language(OBJCXX)
endif()

# Platform detection
set(TARGET_MACOS OFF)
set(TARGET_IOS OFF)
set(TARGET_ANDROID OFF)
set(TARGET_WINDOWS OFF)
set(TARGET_LINUX OFF)

if(ANDROID)
    set(TARGET_ANDROID ON)
    message(STATUS "Target platform: Android (${ANDROID_ABI})")
elseif(IOS)
    set(TARGET_IOS ON)
    message(STATUS "Target platform: iOS")
elseif(APPLE)
    set(TARGET_MACOS ON)
    message(STATUS "Target platform: macOS")
elseif(WIN32)
    set(TARGET_WINDOWS ON)
    message(STATUS "Target platform: Windows")
elseif(UNIX)
    set(TARGET_LINUX ON)
    message(STATUS "Target platform: Linux")
endif()

# Bundle ID configuration
if(NOT APP_BUNDLE_ID)
    set(APP_BUNDLE_ID "work.opine.jingo" CACHE STRING "Application Bundle Identifier")
endif()
message(STATUS "App Bundle ID: ${APP_BUNDLE_ID}")
set(PACKET_TUNNEL_BUNDLE_ID "${APP_BUNDLE_ID}.PacketTunnelProvider")

# Code signing configuration (Apple platforms)
if(NOT APPLE_CODE_SIGN_IDENTITY)
    set(APPLE_CODE_SIGN_IDENTITY "Apple Development" CACHE STRING "Code signing identity")
endif()
if(NOT APPLE_DEVELOPMENT_TEAM)
    set(APPLE_DEVELOPMENT_TEAM "6HP2RFA5AK" CACHE STRING "Apple Development Team ID")
endif()
message(STATUS "Code Sign Identity: ${APPLE_CODE_SIGN_IDENTITY}")
message(STATUS "Development Team: ${APPLE_DEVELOPMENT_TEAM}")

if(BUILD_KEYCHAIN_PATH)
    message(STATUS "Build Keychain: ${BUILD_KEYCHAIN_PATH}")
endif()

# iOS Provisioning Profiles
if(NOT IOS_PROFILE_MAIN)
    set(IOS_PROFILE_MAIN "JinGo Accelerator iOS" CACHE STRING "iOS Main App Provisioning Profile")
endif()
if(NOT IOS_PROFILE_PACKET_TUNNEL)
    set(IOS_PROFILE_PACKET_TUNNEL "JinGo PacketTunnel iOS" CACHE STRING "iOS PacketTunnel Provisioning Profile")
endif()

# macOS Provisioning Profiles
if(NOT MACOS_PROFILE_MAIN)
    set(MACOS_PROFILE_MAIN "JinGo_Accelerator_macOS" CACHE STRING "macOS Main App Provisioning Profile")
endif()
if(NOT MACOS_PROFILE_PACKET_TUNNEL)
    set(MACOS_PROFILE_PACKET_TUNNEL "JinGo_PacketTunnel_macOS" CACHE STRING "macOS PacketTunnel Provisioning Profile")
endif()

# Dependencies
set(QT_NO_DISABLE_WARN_DUPLICATE_LIBRARIES ON)
include(cmake/Dependencies-Qt.cmake)
include(cmake/Dependencies-Xray.cmake)

# JinDoCore static library configuration
set(JINDO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../JinDo" CACHE PATH "Path to JinDo project")
set(JINDO_THIRD_PARTY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/jindo")

if(TARGET_ANDROID)
    set(JINDO_LIB_DIR "${JINDO_THIRD_PARTY}/android/${CMAKE_ANDROID_ARCH_ABI}")
elseif(TARGET_IOS OR TARGET_MACOS)
    set(JINDO_XCFRAMEWORK "${JINDO_THIRD_PARTY}/apple/JinDoCore.xcframework")
    if(TARGET_IOS)
        if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
            set(JINDO_LIB_DIR "${JINDO_XCFRAMEWORK}/ios-arm64_x86_64-simulator")
        else()
            set(JINDO_LIB_DIR "${JINDO_XCFRAMEWORK}/ios-arm64")
        endif()
    else()
        set(JINDO_LIB_DIR "${JINDO_XCFRAMEWORK}/macos-arm64_x86_64")
    endif()
    set(JINDO_INCLUDE_DIR "${JINDO_LIB_DIR}/Headers")
    set(JINDO_LIB_FILE "${JINDO_LIB_DIR}/libJinDoCore.a")
elseif(TARGET_WINDOWS)
    set(JINDO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/jindo/windows/mingw64")
elseif(TARGET_LINUX)
    set(JINDO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/jindo/linux/x64")
endif()

if(NOT JINDO_INCLUDE_DIR)
    set(JINDO_INCLUDE_DIR "${JINDO_LIB_DIR}/include")
endif()
if(NOT JINDO_LIB_FILE)
    set(JINDO_LIB_FILE "${JINDO_LIB_DIR}/libJinDoCore.a")
endif()

if(EXISTS "${JINDO_LIB_FILE}")
    message(STATUS "JinDoCore library: ${JINDO_LIB_FILE}")
    message(STATUS "JinDoCore headers: ${JINDO_INCLUDE_DIR}")
    add_library(JinDoCore STATIC IMPORTED)
    set_target_properties(JinDoCore PROPERTIES IMPORTED_LOCATION "${JINDO_LIB_FILE}")
else()
    message(FATAL_ERROR "JinDoCore library not found: ${JINDO_LIB_FILE}")
endif()

# Platform-specific configuration
if(TARGET_ANDROID)
    include(cmake/Platform-Android.cmake)
elseif(TARGET_IOS)
    include(cmake/Platform-iOS.cmake)
elseif(TARGET_MACOS)
    include(cmake/Platform-macOS.cmake)
elseif(TARGET_WINDOWS)
    include(cmake/Platform-Windows.cmake)
elseif(TARGET_LINUX)
    include(cmake/Platform-Linux.cmake)
endif()

# Source files (using JinDoCore library)
set(CORE_SOURCES "")
set(STORAGE_SOURCES "")
set(UTIL_SOURCES "")
set(NETWORK_SOURCES "")

set(PANEL_SOURCES
    src/panel/V2BoardProvider.cpp
    src/panel/V2BoardProvider.h
)

# JinGo 独有模型（QML 绑定）
set(MODEL_SOURCES
    src/models/SubscriptionListModel.cpp
    src/models/SubscriptionListModel.h
)

# ViewModels（UI 层）- 必须包含头文件以便 AUTOMOC 处理
set(VIEWMODEL_SOURCES
    src/viewmodels/LoginViewModel.cpp
    src/viewmodels/LoginViewModel.h
    src/viewmodels/RegisterViewModel.cpp
    src/viewmodels/RegisterViewModel.h
    src/viewmodels/ServerListViewModel.cpp
    src/viewmodels/ServerListViewModel.h
    src/viewmodels/ConnectionViewModel.cpp
    src/viewmodels/ConnectionViewModel.h
    src/viewmodels/SettingsViewModel.cpp
    src/viewmodels/SettingsViewModel.h
)

# UI 模块（仅桌面平台）
if(NOT TARGET_ANDROID AND NOT TARGET_IOS)
    set(UI_SOURCES
        src/ui/SystemTrayManager.cpp
        src/ui/SystemTrayManager.h
    )
else()
    set(UI_SOURCES "")
endif()

# Platform resources
if(TARGET_MACOS)
    set(MACOS_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.icns")
    set(PLATFORM_RESOURCES ${MACOS_ICON_FILE})
    set_source_files_properties(${MACOS_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
elseif(TARGET_IOS)
    set(PLATFORM_RESOURCES "")
endif()

# Collect all sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
    ${PANEL_SOURCES}
    ${STORAGE_SOURCES}
    ${MODEL_SOURCES}
    ${VIEWMODEL_SOURCES}
    ${UTIL_SOURCES}
    ${UI_SOURCES}
)

set(AVAILABLE_SOURCES)
foreach(source ${ALL_SOURCES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${source})
        list(APPEND AVAILABLE_SOURCES ${source})
    endif()
endforeach()

set(PROJECT_SOURCES
    src/main.cpp
    ${AVAILABLE_SOURCES}
    ${PLATFORM_SOURCES}
)

# GeoIP data files
set(GEOIP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/geoip")
set(GEOIP_DAT_FILE "${GEOIP_SOURCE_DIR}/geoip.dat")
set(GEOSITE_DAT_FILE "${GEOIP_SOURCE_DIR}/geosite.dat")

if(EXISTS ${GEOIP_DAT_FILE})
    message(STATUS "Found geoip.dat")
endif()
if(EXISTS ${GEOSITE_DAT_FILE})
    message(STATUS "Found geosite.dat")
endif()

# Translation files
set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/jingo_zh_CN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/jingo_zh_TW.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/jingo_en_US.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/jingo_fa_IR.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/jingo_ru_RU.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/jingo_vi_VN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/jingo_km_KH.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/jingo_my_MM.ts
)

# QML and resource files
file(GLOB_RECURSE QML_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/qml/*.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/qml/themes/*.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/qml/pages/*.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/qml/components/*.qml
)

set(AVAILABLE_QML_FILES)
foreach(qml_file ${QML_FILES})
    if(EXISTS ${qml_file})
        file(RELATIVE_PATH file_alias ${CMAKE_CURRENT_SOURCE_DIR}/resources ${qml_file})
        string(REPLACE "qml/components/" "qml/JinGo/" file_alias "${file_alias}")
        list(APPEND AVAILABLE_QML_FILES ${qml_file})
        set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${file_alias})
    endif()
endforeach()

set(RESOURCE_FILES resources/resources.qrc)

# Create executable
if(TARGET_ANDROID)
    qt_add_executable(JinGo MANUAL_FINALIZATION ${PROJECT_SOURCES})
elseif(TARGET_MACOS OR TARGET_IOS)
    qt_add_executable(JinGo MACOSX_BUNDLE ${PROJECT_SOURCES} ${PLATFORM_RESOURCES})

    if(TARGET_IOS)
        set(INFO_PLIST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/Info.plist)
        set(IOS_ASSET_CATALOG "${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/Assets.xcassets")
    else()
        set(INFO_PLIST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/Info.plist)
    endif()

    set_target_properties(JinGo PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${INFO_PLIST_PATH}
        MACOSX_BUNDLE_BUNDLE_NAME "JinGoVPN"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_GUI_IDENTIFIER "${APP_BUNDLE_ID}"
        QT_NO_DISABLE_WARN_DUPLICATE_LIBRARIES ON
    )

    if(TARGET_IOS)
        set_target_properties(JinGo PROPERTIES
            RESOURCE "${IOS_ASSET_CATALOG}"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${APP_BUNDLE_ID}"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        )

        if(APPLE_DEVELOPMENT_TEAM)
            if(BUILD_KEYCHAIN_PATH)
                set_target_properties(JinGo PROPERTIES
                    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
                    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
                    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
                    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APPLE_DEVELOPMENT_TEAM}"
                )
            else()
                set_target_properties(JinGo PROPERTIES
                    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${APPLE_CODE_SIGN_IDENTITY}"
                    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/platform/ios/JinGo.entitlements"
                    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APPLE_DEVELOPMENT_TEAM}"
                    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
                    XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${IOS_PROFILE_MAIN}"
                )
            endif()
        else()
            set_target_properties(JinGo PROPERTIES
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
                XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
            )
        endif()

        # Compile Assets.xcassets
        find_program(ACTOOL actool HINTS /usr/bin)
        if(ACTOOL)
            add_custom_command(TARGET JinGo POST_BUILD
                COMMAND ${ACTOOL}
                    --output-format human-readable-text --notices --warnings
                    --export-dependency-info "$<TARGET_BUNDLE_DIR:JinGo>/assetcatalog_dependencies"
                    --output-partial-info-plist "$<TARGET_BUNDLE_DIR:JinGo>/assetcatalog_generated_info.plist"
                    --app-icon AppIcon --compress-pngs --enable-on-demand-resources NO
                    --platform iphoneos --minimum-deployment-target ${CMAKE_OSX_DEPLOYMENT_TARGET}
                    --target-device iphone --compile "$<TARGET_BUNDLE_DIR:JinGo>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/Assets.xcassets"
                COMMENT "Compiling iOS Assets.xcassets"
                VERBATIM
            )
        endif()
    else()
        # macOS
        set_target_properties(JinGo PROPERTIES
            RESOURCE "${PLATFORM_RESOURCES}"
            MACOSX_BUNDLE_ICON_FILE "app.icns"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${APP_BUNDLE_ID}"
        )

        if(SKIP_CODE_SIGNING)
            set_target_properties(JinGo PROPERTIES
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
                XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
                XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
            )
        elseif(APPLE_DEVELOPMENT_TEAM)
            if(BUILD_KEYCHAIN_PATH)
                set_target_properties(JinGo PROPERTIES
                    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
                    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
                    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
                    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APPLE_DEVELOPMENT_TEAM}"
                    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                )
            else()
                set_target_properties(JinGo PROPERTIES
                    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${APPLE_CODE_SIGN_IDENTITY}"
                    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APPLE_DEVELOPMENT_TEAM}"
                    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
                    XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${MACOS_PROFILE_MAIN}"
                    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/platform/macos/JinGo.entitlements"
                    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                    XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS NO
                )
            endif()
        endif()
    endif()
elseif(TARGET_WINDOWS)
    if(PLATFORM_RC)
        set(RC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/JinGo.rc.obj")
        add_custom_command(
            OUTPUT ${RC_OUTPUT}
            COMMAND windres -O coff -I "${CMAKE_CURRENT_SOURCE_DIR}/platform/windows"
                    "${PLATFORM_RC}" "${RC_OUTPUT}"
            DEPENDS ${PLATFORM_RC} "${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/JinGo.manifest"
            COMMENT "Compiling RC file"
            VERBATIM
        )
        qt_add_executable(JinGo WIN32 ${PROJECT_SOURCES} ${RC_OUTPUT})
    else()
        qt_add_executable(JinGo WIN32 ${PROJECT_SOURCES})
    endif()
else()
    qt_add_executable(JinGo ${PROJECT_SOURCES})
endif()

# macOS Helper executables
if(TARGET_MACOS)
    # JinGoHelper - setuid helper for routing/DNS
    set(JINGO_HELPER_SOURCE "${JINDO_ROOT}/src/platform/apple/JinGoHelper.cpp")
    add_executable(JinGoHelper ${JINGO_HELPER_SOURCE})
    set_target_properties(JinGoHelper PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
    )
    add_custom_command(TARGET JinGo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:JinGoHelper>"
            "$<TARGET_BUNDLE_DIR:JinGo>/Contents/MacOS/JinGoHelper"
        COMMENT "Copying JinGoHelper to app bundle"
    )
    add_dependencies(JinGo JinGoHelper)

    # JinGoCore - core service with Xray
    set(JINGO_CORE_SOURCE "${JINDO_ROOT}/src/platform/apple/JinGoCore.mm")
    add_executable(JinGoCore ${JINGO_CORE_SOURCE})
    set_target_properties(JinGoCore PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
    )

    set(SUPERRAY_MACOS_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray/apple/SuperRay.xcframework/macos-arm64_x86_64/libsuperray.a")
    if(EXISTS "${SUPERRAY_MACOS_LIB}")
        target_link_libraries(JinGoCore PRIVATE
            "${SUPERRAY_MACOS_LIB}"
            "-framework Foundation" "-framework CoreFoundation" "-framework Security"
            "-framework SystemConfiguration" "-framework Network"
            resolv
        )
        target_include_directories(JinGoCore PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray/apple/SuperRay.xcframework/macos-arm64_x86_64/Headers"
        )
    endif()

    add_custom_command(TARGET JinGo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:JinGoCore>"
            "$<TARGET_BUNDLE_DIR:JinGo>/Contents/MacOS/JinGoCore"
        COMMENT "Copying JinGoCore to app bundle"
    )
    add_dependencies(JinGo JinGoCore)
endif()

# Exclude FFmpeg multimedia plugins on Apple
if(TARGET_IOS OR TARGET_MACOS)
    qt_import_plugins(JinGo EXCLUDE_BY_TYPE multimedia)
endif()

# QML module
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

if(AVAILABLE_QML_FILES OR RESOURCE_FILES)
    set(QMLDIR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/qml/components/qmldir)
    if(EXISTS ${QMLDIR_FILE})
        set_source_files_properties(${QMLDIR_FILE} PROPERTIES QT_RESOURCE_ALIAS qml/JinGo/qmldir)
        list(APPEND RESOURCE_FILES ${QMLDIR_FILE})
    endif()

    qt_add_qml_module(JinGo
        URI JinGo
        VERSION 1.0
        QML_FILES ${AVAILABLE_QML_FILES}
        RESOURCES ${RESOURCE_FILES}
        NO_RESOURCE_TARGET_PATH
        NO_CACHEGEN
        NO_GENERATE_QMLDIR
        OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml
        RESOURCE_PREFIX "/"
    )
endif()

# Translations
qt_add_translations(JinGo
    TS_FILES ${TS_FILES}
    SOURCES ${PROJECT_SOURCES} ${AVAILABLE_QML_FILES}
    RESOURCE_PREFIX "/translations"
)

if(TARGET_ANDROID)
    set_property(TARGET JinGo APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/android")
    if(DEFINED QT_ANDROID_ABIS)
        set_property(TARGET JinGo PROPERTY QT_ANDROID_ABIS "${QT_ANDROID_ABIS}")
    endif()
endif()

# GeoIP data installation
set(GEOIP_FILES)
if(EXISTS ${GEOIP_DAT_FILE})
    list(APPEND GEOIP_FILES ${GEOIP_DAT_FILE})
endif()
if(EXISTS ${GEOSITE_DAT_FILE})
    list(APPEND GEOIP_FILES ${GEOSITE_DAT_FILE})
endif()

if(GEOIP_FILES)
    if(TARGET_MACOS)
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Resources/dat"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOIP_DAT_FILE}
                "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Resources/dat/geoip.dat"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOSITE_DAT_FILE}
                "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Resources/dat/geosite.dat"
            COMMENT "Copying GeoIP data files"
        )
    elseif(TARGET_IOS)
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOIP_DAT_FILE} "$<TARGET_BUNDLE_DIR:JinGo>/geoip.dat"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOSITE_DAT_FILE} "$<TARGET_BUNDLE_DIR:JinGo>/geosite.dat"
            COMMENT "Copying GeoIP data files"
        )
    elseif(TARGET_ANDROID)
        set(ANDROID_GEOIP_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platform/android/assets/dat")
        file(MAKE_DIRECTORY "${ANDROID_GEOIP_ASSETS_DIR}")
    elseif(TARGET_WINDOWS)
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:JinGo>/dat"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOIP_DAT_FILE} "$<TARGET_FILE_DIR:JinGo>/dat/geoip.dat"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOSITE_DAT_FILE} "$<TARGET_FILE_DIR:JinGo>/dat/geosite.dat"
            COMMENT "Copying GeoIP data files"
        )
    else()
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOIP_DAT_FILE} "${CMAKE_BINARY_DIR}/geoip.dat"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOSITE_DAT_FILE} "${CMAKE_BINARY_DIR}/geosite.dat"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOIP_DAT_FILE} "${CMAKE_BINARY_DIR}/bin/geoip.dat"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEOSITE_DAT_FILE} "${CMAKE_BINARY_DIR}/bin/geosite.dat"
            COMMENT "Copying GeoIP data files"
        )
    endif()
endif()

# Bundle config deployment
set(BUNDLE_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/bundle_config.json")
if(EXISTS ${BUNDLE_CONFIG_FILE})
    if(TARGET_MACOS)
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE_CONFIG_FILE}
                "$<TARGET_BUNDLE_DIR:JinGo>/Contents/Resources/bundle_config.json"
            COMMENT "Copying bundle_config.json"
        )
    elseif(TARGET_IOS)
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE_CONFIG_FILE}
                "$<TARGET_BUNDLE_DIR:JinGo>/bundle_config.json"
            COMMENT "Copying bundle_config.json"
        )
    elseif(TARGET_ANDROID)
        set(ANDROID_CONFIG_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platform/android/assets")
        file(MAKE_DIRECTORY "${ANDROID_CONFIG_ASSETS_DIR}")
        file(COPY ${BUNDLE_CONFIG_FILE} DESTINATION "${ANDROID_CONFIG_ASSETS_DIR}")
    elseif(TARGET_WINDOWS OR TARGET_LINUX)
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE_CONFIG_FILE}
                "$<TARGET_FILE_DIR:JinGo>/bundle_config.json"
            COMMENT "Copying bundle_config.json"
        )
    endif()
endif()

# Include directories
target_include_directories(JinGo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewmodels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_BINARY_DIR}/JinGo_autogen/include
)

if(WIN32)
    target_include_directories(JinGo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wintun/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray/windows/include
    )
elseif(TARGET_LINUX)
    target_include_directories(JinGo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray/linux/include
    )
endif()

if(EXISTS ${SUPERRAY_INCLUDE_DIR})
    target_include_directories(JinGo PRIVATE ${SUPERRAY_INCLUDE_DIR})
endif()

if(TARGET_ANDROID)
    target_include_directories(JinGo PRIVATE ${Qt6Core_PRIVATE_INCLUDE_DIRS})
endif()

# Link libraries
target_link_libraries(JinGo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    ${PLATFORM_LIBS}
)

# Link JinDoCore
if(TARGET JinDoCore)
    target_link_libraries(JinGo PRIVATE JinDoCore)
    target_include_directories(JinGo PRIVATE
        ${JINDO_INCLUDE_DIR}
        ${JINDO_INCLUDE_DIR}/core
        ${JINDO_INCLUDE_DIR}/platform
        ${JINDO_INCLUDE_DIR}/platform/apple
        ${JINDO_INCLUDE_DIR}/storage
        ${JINDO_INCLUDE_DIR}/utils
        ${JINDO_INCLUDE_DIR}/extensions
        ${JINDO_INCLUDE_DIR}/extensions/PacketTunnelProvider
    )

    if(TARGET_WINDOWS)
        set(WINPTHREAD_STATIC_LIB "D:/Qt/Tools/mingw1310_64/x86_64-w64-mingw32/lib/libwinpthread.a")
        if(EXISTS "${WINPTHREAD_STATIC_LIB}")
            target_link_libraries(JinGo PRIVATE
                Qt6::Sql Qt6::Gui Qt6::Widgets
                wininet crypt32 ${WINPTHREAD_STATIC_LIB}
            )
        endif()
    endif()

    if(TARGET_LINUX)
        target_link_libraries(JinGo PRIVATE
            Qt6::Sql Qt6::Gui Qt6::Widgets ${PLATFORM_LIBS}
        )
    endif()
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID AND TARGET Qt6::DBus)
    target_link_libraries(JinGo PRIVATE Qt6::DBus)
endif()

# OpenSSL / Platform crypto
if(TARGET_ANDROID OR ANDROID)
    set(ANDROID_OPENSSL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/android_openssl")
    if(EXISTS "${ANDROID_OPENSSL_ROOT}/${ANDROID_ABI}")
        target_include_directories(JinGo PRIVATE "${ANDROID_OPENSSL_ROOT}/include")
        target_link_libraries(JinGo PRIVATE
            "${ANDROID_OPENSSL_ROOT}/${ANDROID_ABI}/libssl_3.so"
            "${ANDROID_OPENSSL_ROOT}/${ANDROID_ABI}/libcrypto_3.so"
        )
    endif()
elseif(TARGET_IOS OR TARGET_MACOS)
    message(STATUS "Using Security.framework + CommonCrypto")
elseif(WIN32)
    message(STATUS "Using BCrypt/NCrypt (CNG)")
else()
    set(LINUX_OPENSSL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/linux_openssl")
    if(EXISTS "${LINUX_OPENSSL_ROOT}/x86_64/libssl.so")
        target_include_directories(JinGo PRIVATE "${LINUX_OPENSSL_ROOT}/include")
        target_link_libraries(JinGo PRIVATE
            "${LINUX_OPENSSL_ROOT}/x86_64/libssl.so"
            "${LINUX_OPENSSL_ROOT}/x86_64/libcrypto.so"
        )
    else()
        find_package(OpenSSL REQUIRED)
        target_link_libraries(JinGo PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
endif()

# Compile definitions
target_compile_definitions(JinGo PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x060500
    APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="JinGoVPN"
    APP_ORGANIZATION="JinGo"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(JinGo PRIVATE QT_QML_DEBUG DEBUG_MODE QT_MESSAGELOGCONTEXT ENABLE_VERBOSE_LOGGING)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(JinGo PRIVATE QT_NO_DEBUG_OUTPUT NDEBUG QT_NO_WARNING_OUTPUT)
endif()

# SuperRay integration
option(USE_SUPERRAY "Enable SuperRay support" ON)
if(USE_SUPERRAY AND EXISTS ${SUPERRAY_INCLUDE_DIR})
    target_include_directories(JinGo PRIVATE ${SUPERRAY_INCLUDE_DIR})

    if(TARGET_MACOS OR TARGET_IOS)
        target_link_directories(JinGo PRIVATE "${SUPERRAY_LIBRARY_DIR}")
        target_link_options(JinGo PRIVATE "-F${SUPERRAY_LIBRARY_DIR}")
        target_link_libraries(JinGo PRIVATE "${SUPERRAY_LIB}" "-lz" "-lresolv")
    elseif(TARGET_ANDROID)
        if(TARGET superray_imported)
            target_link_libraries(JinGo PRIVATE superray_imported)
        elseif(SUPERRAY_LIB AND EXISTS "${SUPERRAY_LIB}")
            get_filename_component(SUPERRAY_SO_DIR "${SUPERRAY_LIB}" DIRECTORY)
            target_link_directories(JinGo PRIVATE "${SUPERRAY_SO_DIR}")
            target_link_libraries(JinGo PRIVATE superray)
        endif()
    elseif(TARGET_WINDOWS)
        set(SUPERRAY_WIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray/windows")
        if(EXISTS "${SUPERRAY_WIN_DIR}/amd64/superray.dll")
            target_link_directories(JinGo PRIVATE "${SUPERRAY_WIN_DIR}/amd64")
            target_link_libraries(JinGo PRIVATE superray)
        endif()
    elseif(TARGET_LINUX)
        set(SUPERRAY_LINUX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray/linux")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(SUPERRAY_LINUX_LIB_DIR "${SUPERRAY_LINUX_DIR}/arm64")
        else()
            set(SUPERRAY_LINUX_LIB_DIR "${SUPERRAY_LINUX_DIR}/amd64")
        endif()
        if(EXISTS "${SUPERRAY_LINUX_LIB_DIR}/libsuperray.so")
            target_link_directories(JinGo PRIVATE "${SUPERRAY_LINUX_LIB_DIR}")
            target_link_libraries(JinGo PRIVATE superray)
        endif()
    endif()

    target_compile_definitions(JinGo PRIVATE HAVE_SUPERRAY)

    # RPATH configuration
    if(TARGET_LINUX)
        set_target_properties(JinGo PROPERTIES
            INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
            BUILD_WITH_INSTALL_RPATH OFF
        )
    elseif(TARGET_MACOS)
        set_target_properties(JinGo PROPERTIES
            INSTALL_RPATH "@executable_path/../Frameworks"
            BUILD_WITH_INSTALL_RPATH OFF
        )
    endif()
else()
    target_compile_definitions(JinGo PRIVATE NO_SUPERRAY)
endif()

# Windows: WinTun and deployment
if(WIN32)
    set(WINTUN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/wintun")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(WINTUN_DLL "${WINTUN_ROOT}/bin/amd64/wintun.dll")
        set(SUPERRAY_DLL "${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray/windows/amd64/superray.dll")
    else()
        set(WINTUN_DLL "${WINTUN_ROOT}/bin/x86/wintun.dll")
        set(SUPERRAY_DLL "${CMAKE_CURRENT_SOURCE_DIR}/third_party/superray/windows/x86/superray.dll")
    endif()

    if(EXISTS ${WINTUN_DLL})
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WINTUN_DLL}" "$<TARGET_FILE_DIR:JinGo>/wintun.dll"
            COMMENT "Copying wintun.dll"
        )
    endif()

    if(EXISTS ${SUPERRAY_DLL})
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SUPERRAY_DLL}" "$<TARGET_FILE_DIR:JinGo>/superray.dll"
            COMMENT "Copying superray.dll"
        )
    endif()

    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${CMAKE_PREFIX_PATH}/bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET JinGo POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --qmldir "${CMAKE_SOURCE_DIR}/resources/qml"
                --no-compiler-runtime --no-translations
                "$<TARGET_FILE:JinGo>"
            COMMENT "Running windeployqt"
        )
    endif()

    # Copy MinGW runtime DLLs
    get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    foreach(dll "libgcc_s_seh-1.dll" "libstdc++-6.dll" "libwinpthread-1.dll")
        if(EXISTS "${MINGW_BIN_DIR}/${dll}")
            add_custom_command(TARGET JinGo POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MINGW_BIN_DIR}/${dll}" "$<TARGET_FILE_DIR:JinGo>/${dll}"
            )
        endif()
    endforeach()
endif()

# Linux: TUN permissions
if(TARGET_LINUX)
    add_custom_command(TARGET JinGo POST_BUILD
        COMMAND bash -c "if command -v sudo >/dev/null 2>&1; then \
            sudo setcap cap_net_admin+eip $<TARGET_FILE:JinGo> 2>/dev/null || true; fi"
        COMMENT "Setting Linux TUN permissions"
        VERBATIM
    )
endif()

# Compile options
if(MSVC)
    target_compile_options(JinGo PRIVATE /W4 /utf-8 /MP /Zc:__cplusplus /permissive-)
else()
    target_compile_options(JinGo PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3>
        -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-variable
        -Wno-missing-field-initializers -Wno-sign-compare
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(JinGo PRIVATE
            -Wno-cast-function-type
            $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>
            -Wno-stringop-overread
        )
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(JinGo PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder-ctor>)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(JinGo PRIVATE -Wno-unused-result)
    endif()
endif()

if(TARGET_MACOS OR TARGET_IOS)
    target_compile_options(JinGo PRIVATE -fmodules -fcxx-modules)
    set_target_properties(JinGo PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
    target_compile_options(JinGo PRIVATE -O2)
endif()

# Platform finalization
if(TARGET_ANDROID)
    include(cmake/Platform-Android-Finalize.cmake)
endif()

# iOS Network Extension
if(TARGET_IOS)
    include(cmake/Extension-PacketTunnel.cmake)
endif()

# Apple code signing
if(APPLE AND NOT ANDROID)
    include(cmake/Signing-Apple.cmake)
endif()

# Linux installation
if(LINUX AND NOT TARGET_ANDROID)
    install(TARGETS JinGo RUNTIME DESTINATION bin COMPONENT applications)
    install(FILES platform/linux/jingo.desktop DESTINATION share/applications COMPONENT applications)
    install(FILES resources/icons/app.png DESTINATION share/icons/hicolor/512x512/apps RENAME jingo.png COMPONENT applications)
    install(FILES resources/icons/app.png DESTINATION share/pixmaps RENAME jingo.png COMPONENT applications)
endif()

# Finalize Qt target for mobile
if(TARGET_IOS OR TARGET_ANDROID)
    qt_finalize_target(JinGo)
endif()

# Configuration summary
message(STATUS "")
message(STATUS "========================================")
message(STATUS "JinGoVPN Configuration Summary")
message(STATUS "========================================")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Platform:       ${PLATFORM_NAME}")
message(STATUS "  Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:   C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt Version:     ${Qt6_VERSION}")
message(STATUS "  JinDoCore:      ${JINDO_LIB_FILE}")
if(TARGET_ANDROID)
    message(STATUS "  Android API:    ${ANDROID_MIN_SDK_VERSION} - ${ANDROID_TARGET_SDK_VERSION}")
endif()
message(STATUS "========================================")
message(STATUS "")
