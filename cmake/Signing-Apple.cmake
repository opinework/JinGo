# ============================================================================
# Signing-Apple.cmake - macOS Code Signing and Deployment
# ============================================================================
# This module handles:
# - macdeployqt automatic execution after build
# - Qt dependencies bundling
# - Qt plugins rpath fix (CRITICAL for app to run)
# - Framework code signing (if SIGNING_IDENTITY is set)
# - Extension code signing (PacketTunnelProvider)
# - Main app code signing
#
# IMPORTANT: All steps run in correct order via a single script:
# 1. macdeployqt bundles Qt frameworks
# 2. Remove SuperRay.framework
# 3. Fix Qt plugins rpath (MUST happen BEFORE signing)
# 4. Sign frameworks and app (only if SIGNING_IDENTITY is set)
#
# Usage:
#   if(TARGET_MACOS)
#       include(cmake/Signing-Apple.cmake)
#   endif()
#
# Requirements:
#   - TARGET_MACOS must be set
#   - JinGo target must exist
# Optional:
#   - SIGNING_IDENTITY for code signing
#   - APP_ENTITLEMENTS_SIGNED and EXT_ENTITLEMENTS_SIGNED for entitlements
# ============================================================================

if(NOT TARGET_MACOS)
    message(WARNING "Signing-Apple.cmake included but TARGET_MACOS is not set")
    return()
endif()

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Configuring macOS Automatic Deployment")
message(STATUS "========================================")

# ============================================================================
# Find macdeployqt Tool
# ============================================================================

find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${Qt6_DIR}/../../../bin)

if(NOT MACDEPLOYQT_EXECUTABLE)
    message(WARNING "macdeployqt not found. Qt dependencies will not be bundled automatically.")
    message(WARNING "App may not run on systems without Qt installed.")
    return()
endif()

message(STATUS "macdeployqt found: ${MACDEPLOYQT_EXECUTABLE}")

# ============================================================================
# Post-Build Script Path
# ============================================================================
set(POST_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/signing/post_build_macos.sh")

if(NOT EXISTS ${POST_BUILD_SCRIPT})
    message(WARNING "Post-build script not found: ${POST_BUILD_SCRIPT}")
    message(WARNING "Qt plugins rpath fix will not be applied!")
endif()

# ============================================================================
# All Post-Build Steps (Single Command for Correct Ordering)
# ============================================================================
# Order is critical:
# 1. macdeployqt bundles Qt frameworks
# 2. post_build_macos.sh:
#    a. Remove SuperRay.framework
#    b. Fix Qt plugins rpath (BEFORE signing!)
#    c. Sign everything (only if SIGNING_IDENTITY is set)

add_custom_command(TARGET JinGo POST_BUILD
    # Step 1: Run macdeployqt
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Running macdeployqt to bundle Qt dependencies..."
    COMMAND "${MACDEPLOYQT_EXECUTABLE}"
        "$<TARGET_BUNDLE_DIR:JinGo>"
        -verbose=1
        -qmldir=${CMAKE_SOURCE_DIR}/resources/qml

    # Step 2: Run unified post-build script (rpath fix THEN signing)
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND bash "${POST_BUILD_SCRIPT}"
        "$<TARGET_BUNDLE_DIR:JinGo>"
        "${SIGNING_IDENTITY}"
        "${APP_ENTITLEMENTS_SIGNED}"
        "${EXT_ENTITLEMENTS_SIGNED}"

    COMMENT "Building and deploying JinGo.app"
    VERBATIM
)

# ============================================================================
# Status Messages
# ============================================================================
message(STATUS "")
message(STATUS "Post-build workflow:")
message(STATUS "  1. macdeployqt bundles Qt frameworks")
message(STATUS "  2. Remove SuperRay.framework (using static library)")
message(STATUS "  3. Fix Qt plugins rpath (CRITICAL for app to run)")

if(SIGNING_IDENTITY)
    message(STATUS "  4. Sign all frameworks")
    message(STATUS "  5. Sign PacketTunnelProvider.appex")
    message(STATUS "  6. Sign main executable")
    message(STATUS "")
    message(STATUS "Code signing enabled:")
    message(STATUS "  Identity: ${SIGNING_IDENTITY}")
    if(MACOS_TEAM_ID)
        message(STATUS "  Team ID: ${MACOS_TEAM_ID}")
    endif()
    if(APP_ENTITLEMENTS_SIGNED)
        message(STATUS "  App entitlements: ${APP_ENTITLEMENTS_SIGNED}")
    endif()
    if(EXT_ENTITLEMENTS_SIGNED)
        message(STATUS "  Extension entitlements: ${EXT_ENTITLEMENTS_SIGNED}")
    endif()
else()
    message(STATUS "")
    message(STATUS "Code signing DISABLED (SIGNING_IDENTITY not set)")
    message(STATUS "  App will use ad-hoc signature for local development")
    message(STATUS "  To enable signing, set SIGNING_IDENTITY CMake variable")
endif()

message(STATUS "========================================")
message(STATUS "")
