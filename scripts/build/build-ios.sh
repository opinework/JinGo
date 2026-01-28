#!/bin/bash
# ============================================================================
# JinGo VPN - iOS 构建脚本
# ============================================================================
# 描述：编译 iOS 应用 (真机/模拟器)
#
# 功能：生成 Xcode 项目、编译、签名、安装到设备
# 依赖：macOS, Xcode 14.0+, CMake 3.21+, Qt 6.5+ for iOS
# 版本：1.2.0
# ============================================================================

set -e  # 遇到错误立即退出

# ============================================================================
# ██████╗  ██╗      █████╗ ████████╗███████╗ ██████╗ ██████╗ ███╗   ███╗
# ██╔══██╗ ██║     ██╔══██╗╚══██╔══╝██╔════╝██╔═══██╗██╔══██╗████╗ ████║
# ██████╔╝ ██║     ███████║   ██║   █████╗  ██║   ██║██████╔╝██╔████╔██║
# ██╔═══╝  ██║     ██╔══██║   ██║   ██╔══╝  ██║   ██║██╔══██╗██║╚██╔╝██║
# ██║      ███████╗██║  ██║   ██║   ██║     ╚██████╔╝██║  ██║██║ ╚═╝ ██║
# ╚═╝      ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝      ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝
#              用户配置 - 修改下面的路径以匹配您的环境
# ============================================================================

# --------------------- Qt 路径配置 ---------------------
# Qt iOS 安装路径
# 优先使用环境变量 QT_IOS_PATH 或 Qt6_DIR，否则使用默认值
# 本地开发请修改下面的默认路径，或设置环境变量
# 示例: "/Users/yourname/Qt/6.8.0/ios"
if [[ -n "${QT_IOS_PATH:-}" ]]; then
    : # 使用已设置的 QT_IOS_PATH
elif [[ -n "${Qt6_DIR:-}" ]]; then
    QT_IOS_PATH="$Qt6_DIR"
else
    QT_IOS_PATH="/Volumes/mindata/Applications/Qt/6.10.0/ios"
fi

# --------------------- Apple 开发者配置 (必需!) ---------------------
# ⚠️  iOS 构建必须签名，请在下面填入您的开发者信息
#
# 开发团队 ID (在 Apple Developer 账号中查看)
# 示例: "ABC123DEF4"
TEAM_ID="${APPLE_DEVELOPMENT_TEAM:-YOUR_TEAM_ID_HERE}"

# 签名身份:
#   - "Apple Development" (开发/测试)
#   - "Apple Distribution" (App Store/Ad Hoc 分发)
CODE_SIGN_IDENTITY="${APPLE_CODE_SIGN_IDENTITY:-Apple Development}"

# --------------------- Provisioning Profile 名称 ---------------------
# 在 Apple Developer 网站创建并下载对应的 Provisioning Profile
# 将 .mobileprovision 文件放到 platform/ios/cert/ 目录
PROFILE_MAIN="${IOS_PROFILE_MAIN:-JinGo Accelerator iOS}"
PROFILE_PACKET_TUNNEL="${IOS_PROFILE_PACKET_TUNNEL:-JinGo PacketTunnel iOS}"

# --------------------- 测试设备配置 ---------------------
# 默认测试设备 UDID (用于 --install 选项)
# 使用 Xcode > Window > Devices 查看设备 UDID
DEFAULT_DEVICE_UDID="${IOS_DEVICE_UDID:-}"

# --------------------- 脚本初始化 ---------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载白标资源复制脚本
if [[ -f "$SCRIPT_DIR/copy-brand-assets.sh" ]]; then
    source "$SCRIPT_DIR/copy-brand-assets.sh"
fi

# --------------------- iOS 配置 ---------------------
# 最低 iOS 版本
IOS_DEPLOYMENT_TARGET="14.0"

# --------------------- 应用信息 ---------------------
APP_NAME="JinGo"
APP_BUNDLE_ID="${APP_BUNDLE_ID:-work.opine.jingo}"

# ============================================================================
# 脚本内部变量 (一般不需要修改)
# ============================================================================
# SCRIPT_DIR 已在上面定义
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-ios"
RELEASE_DIR="$PROJECT_ROOT/release"
CONFIGURATION="Debug"
XCODE_ONLY=false
CLEAN_BUILD=false
INSTALL_APP=false
SIMULATOR_BUILD=false
DEVICE_UDID="$DEFAULT_DEVICE_UDID"
BRAND_NAME=""

# --------------------- 输出命名 ---------------------
# 获取构建日期 (YYYYMMDD 格式)
BUILD_DATE=$(date +%Y%m%d)

# 生成输出文件名: {brand}-{version}-{date}-{platform}.{ext}
generate_output_name() {
    local version="${1:-1.0.0}"
    local ext="${2:-}"
    local brand="${BRAND_NAME:-jingo}"
    local platform="ios"

    if [[ -n "$ext" ]]; then
        echo "${brand}-${version}-${BUILD_DATE}-${platform}.${ext}"
    else
        echo "${brand}-${version}-${BUILD_DATE}-${platform}"
    fi
}

# ============================================================================
# 颜色定义
# ============================================================================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 显示帮助信息
show_help() {
    cat << EOF
JinGoVPN iOS 构建脚本

用法: $0 [选项]

选项:
    -x, --xcode          仅生成 Xcode 项目（不编译）
    -c, --clean          清理构建目录
    -d, --debug          Debug 模式（默认）
    -r, --release        Release 模式
    -i, --install        安装到连接的 iOS 设备
    -s, --simulator      构建模拟器版本（x86_64 + arm64）
    --device UDID        指定设备 UDID
    -b, --brand NAME     应用白标定制（从 white-labeling/<NAME> 加载配置）
    --bundle-id ID       指定 Bundle ID (如 cfd.jingo.vpn)
                         扩展会自动派生: ID.PacketTunnelProvider
    --team-id ID         Apple 开发团队 ID (必需，除非已在脚本中配置)
    --sign-identity ID   代码签名身份 (默认: Apple Development)
    --profile-main NAME      主应用 Provisioning Profile 名称
    --profile-tunnel NAME    PacketTunnel 扩展 Provisioning Profile 名称
    -h, --help           显示帮助信息

环境变量:
    APPLE_DEVELOPMENT_TEAM    Apple 开发团队 ID（必需！）
    APPLE_CODE_SIGN_IDENTITY  代码签名身份（默认: Apple Development）
    QT_IOS_PATH               Qt iOS 安装路径
    APP_BUNDLE_ID             应用 Bundle ID（默认: work.opine.jingo）
    IOS_PROFILE_MAIN          主应用 Provisioning Profile 名称
    IOS_PROFILE_PACKET_TUNNEL PacketTunnel 扩展 Provisioning Profile 名称
    IOS_DEVICE_UDID           默认安装设备 UDID

首次使用:
    1. 编辑脚本顶部的 'Apple 开发者配置' 部分
    2. 设置 TEAM_ID 为您的开发团队 ID
    3. 将 Provisioning Profile 放到 platform/ios/cert/ 目录

示例:
    # 仅生成 Xcode 项目
    $0 --xcode

    # 清理并编译 Debug 版本
    $0 --clean --debug

    # 编译模拟器版本
    $0 --simulator --debug

    # 编译 Release 版本并安装到设备
    $0 --release --install --device 00008030-001238903A90802E

EOF
}

# 解析命令行参数
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -x|--xcode)
                XCODE_ONLY=true
                shift
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -d|--debug)
                CONFIGURATION="Debug"
                shift
                ;;
            -r|--release)
                CONFIGURATION="Release"
                shift
                ;;
            -i|--install)
                INSTALL_APP=true
                shift
                ;;
            -s|--simulator)
                SIMULATOR_BUILD=true
                shift
                ;;
            --device)
                DEVICE_UDID="$2"
                shift 2
                ;;
            -b|--brand)
                if [[ -z "$2" ]] || [[ "$2" == -* ]]; then
                    print_error "--brand 需要指定品牌名称"
                    exit 1
                fi
                BRAND_NAME="$2"
                shift 2
                ;;
            --bundle-id)
                if [[ -z "$2" ]] || [[ "$2" == -* ]]; then
                    print_error "--bundle-id 需要指定 Bundle ID"
                    exit 1
                fi
                APP_BUNDLE_ID="$2"
                shift 2
                ;;
            --team-id)
                if [[ -z "$2" ]] || [[ "$2" == -* ]]; then
                    print_error "--team-id 需要指定开发团队 ID"
                    exit 1
                fi
                TEAM_ID="$2"
                shift 2
                ;;
            --sign-identity)
                if [[ -z "$2" ]] || [[ "$2" == -* ]]; then
                    print_error "--sign-identity 需要指定签名身份"
                    exit 1
                fi
                CODE_SIGN_IDENTITY="$2"
                shift 2
                ;;
            --profile-main)
                if [[ -z "$2" ]] || [[ "$2" == -* ]]; then
                    print_error "--profile-main 需要指定 Profile 名称"
                    exit 1
                fi
                PROFILE_MAIN="$2"
                shift 2
                ;;
            --profile-tunnel)
                if [[ -z "$2" ]] || [[ "$2" == -* ]]; then
                    print_error "--profile-tunnel 需要指定 Profile 名称"
                    exit 1
                fi
                PROFILE_PACKET_TUNNEL="$2"
                shift 2
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# ============================================================================
# 应用白标定制
# ============================================================================
apply_brand_customization() {
    # iOS 平台默认使用品牌 5
    local brand_id="${BRAND_NAME:-5}"

    print_info "应用白标定制: $brand_id"

    # 使用 copy-brand-assets.sh 中的函数复制资源
    if type copy_brand_assets &> /dev/null; then
        if ! copy_brand_assets "$brand_id"; then
            print_error "白标资源复制失败"
            exit 1
        fi
    else
        print_warning "copy_brand_assets 函数未加载，跳过白标资源复制"
    fi

    print_success "白标定制已应用"
}

# 检查必要工具
check_requirements() {
    print_info "检查构建环境..."

    # 检查 macOS
    if [[ "$OSTYPE" != "darwin"* ]]; then
        print_error "此脚本只能在 macOS 上运行"
        exit 1
    fi

    # 检查 TEAM_ID 配置 (iOS 必须签名)
    if [[ "$TEAM_ID" == "YOUR_TEAM_ID_HERE" ]] || [[ -z "$TEAM_ID" ]]; then
        print_error "═══════════════════════════════════════════════════════════════"
        print_error "  iOS 构建需要配置 Apple 开发者团队 ID"
        print_error "═══════════════════════════════════════════════════════════════"
        print_error ""
        print_error "请执行以下步骤："
        print_error ""
        print_error "  1. 打开脚本文件: scripts/build/build-ios.sh"
        print_error "  2. 找到 'Apple 开发者配置' 部分"
        print_error "  3. 将 TEAM_ID 修改为您的开发团队 ID"
        print_error ""
        print_error "  或者通过环境变量设置:"
        print_error "    export APPLE_DEVELOPMENT_TEAM=\"YOUR_TEAM_ID\""
        print_error ""
        print_error "  获取 Team ID 的方法:"
        print_error "    - 登录 https://developer.apple.com"
        print_error "    - 进入 Membership 页面查看 Team ID"
        print_error ""
        exit 1
    fi
    print_success "开发团队 ID: $TEAM_ID"

    # 检查 CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake 未安装。请安装 CMake: brew install cmake"
        exit 1
    fi
    print_success "CMake: $(cmake --version | head -n1)"

    # 检查 Xcode
    if ! command -v xcodebuild &> /dev/null; then
        print_error "Xcode 未安装。请从 Mac App Store 安装 Xcode"
        exit 1
    fi
    XCODE_VERSION=$(xcodebuild -version | head -n1)
    print_success "Xcode: $XCODE_VERSION"

    # 检查 qt-cmake (可选)
    if command -v qt-cmake &> /dev/null; then
        print_success "Qt CMake 已找到"
    else
        print_warning "qt-cmake 未找到，将使用标准 cmake"
    fi

    # 证书检测移至签名阶段，构建前不需要检测
    # 因为 CI 环境的证书在 build.keychain 中，签名时统一处理
    if [ "$SIMULATOR_BUILD" = false ]; then
        print_info "签名: 将在构建完成后进行 (证书检测移至签名阶段)"
    fi

    print_success "构建环境检查完成"
}

# 清理构建目录
clean_build_dir() {
    if [ "$CLEAN_BUILD" = true ]; then
        print_info "清理构建目录: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
        print_success "构建目录已清理"
    fi
}

# ============================================================================
# 刷新 Extension Info.plist 缓存 (确保使用最新版本)
# ============================================================================
refresh_extension_plist() {
    print_info "刷新 Extension Info.plist..."

    local source_plist="$PROJECT_ROOT/src/extensions/PacketTunnelProvider/Info-iOS.plist"
    local cached_plist="$BUILD_DIR/CMakeFiles/PacketTunnelProvider.dir/Info.plist"
    local packet_tunnel_bundle_id="${APP_BUNDLE_ID}.PacketTunnelProvider"

    # 确保缓存目录存在
    mkdir -p "$(dirname "$cached_plist")"

    # 复制最新的 Info.plist 到缓存位置并替换变量
    if [[ -f "$source_plist" ]]; then
        # 复制并替换 Xcode 变量为实际值
        sed -e "s/\$(PRODUCT_BUNDLE_IDENTIFIER)/${packet_tunnel_bundle_id}/g" \
            "$source_plist" > "$cached_plist"

        print_success "已刷新: Info.plist"
        print_info "Bundle ID: $packet_tunnel_bundle_id"

        # 显示版本信息
        local version=$(plutil -extract CFBundleVersion raw "$cached_plist" 2>/dev/null || echo "unknown")
        print_info "Extension 版本: $version"
    else
        print_warning "源 Info.plist 未找到: $source_plist"
    fi

    # 清理旧的 Extension 构建产物 (强制重新构建)
    local extension_artifacts=(
        "$BUILD_DIR/bin/Debug/PacketTunnelProvider.appex"
        "$BUILD_DIR/bin/Release/PacketTunnelProvider.appex"
        "$BUILD_DIR/bin/Debug/JinGo.app/PlugIns"
        "$BUILD_DIR/bin/Release/JinGo.app/PlugIns"
    )

    for artifact in "${extension_artifacts[@]}"; do
        if [[ -e "$artifact" ]]; then
            rm -rf "$artifact"
            print_info "已清理: $(basename "$artifact")"
        fi
    done
}

# 生成 Xcode 项目
generate_xcode_project() {
    print_info "生成 Xcode 项目..."
    print_info "  项目目录: $PROJECT_ROOT"
    print_info "  构建目录: $BUILD_DIR"
    print_info "  Team ID: $TEAM_ID"

    # 根据模拟器/真机选择不同的配置
    if [ "$SIMULATOR_BUILD" = true ]; then
        print_info "  目标平台: iOS 模拟器"
        SDK="iphonesimulator"
        ARCHS="x86_64;arm64"
    else
        print_info "  目标平台: iOS 真机"
        SDK="iphoneos"
        ARCHS="arm64"
    fi

    CMAKE_ARGS=(
        -S "$PROJECT_ROOT"
        -B "$BUILD_DIR"
        -G Xcode
        -DCMAKE_SYSTEM_NAME=iOS
        -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
        -DCMAKE_OSX_ARCHITECTURES="$ARCHS"
        -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO
        -DCMAKE_PREFIX_PATH="$QT_IOS_PATH"
        -DCMAKE_TOOLCHAIN_FILE="$QT_IOS_PATH/lib/cmake/Qt6/qt.toolchain.cmake"
        -DAPP_BUNDLE_ID="$APP_BUNDLE_ID"
    )

    # 模拟器使用不同的 SDK
    if [ "$SIMULATOR_BUILD" = true ]; then
        CMAKE_ARGS+=(-DCMAKE_OSX_SYSROOT=iphonesimulator)
    else
        CMAKE_ARGS+=(-DCMAKE_OSX_SYSROOT=iphoneos)
        # 传递签名配置给CMake
        CMAKE_ARGS+=(-DAPPLE_DEVELOPMENT_TEAM="$TEAM_ID")
        CMAKE_ARGS+=(-DAPPLE_CODE_SIGN_IDENTITY="$CODE_SIGN_IDENTITY")
        CMAKE_ARGS+=(-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="$TEAM_ID")
        CMAKE_ARGS+=(-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="$CODE_SIGN_IDENTITY")
        # 传递 Provisioning Profile 名称给 CMake
        CMAKE_ARGS+=(-DIOS_PROFILE_MAIN="$PROFILE_MAIN")
        CMAKE_ARGS+=(-DIOS_PROFILE_PACKET_TUNNEL="$PROFILE_PACKET_TUNNEL")
        # CI 环境：传递 keychain 路径给 CMake（禁用 Xcode 内置签名）
        if [[ -n "${BUILD_KEYCHAIN:-}" ]] && [[ -f "$BUILD_KEYCHAIN" ]]; then
            CMAKE_ARGS+=(-DBUILD_KEYCHAIN_PATH="$BUILD_KEYCHAIN")
        fi
    fi

    cmake "${CMAKE_ARGS[@]}"

    # 修正 Xcode 项目配置（整合自 fix-xcode-project-ios.sh）
    print_info "修正 Xcode 项目配置..."
    PBXPROJ_FILE="$BUILD_DIR/${APP_NAME}.xcodeproj/project.pbxproj"

    if [[ -f "$PBXPROJ_FILE" ]]; then
        # 备份原始文件
        cp "$PBXPROJ_FILE" "$PBXPROJ_FILE.bak"

        # 1. 将所有 Automatic 签名改为 Manual
        print_info "  - 设置手动签名..."
        sed -i '' 's/CODE_SIGN_STYLE = Automatic;/CODE_SIGN_STYLE = Manual;/g' "$PBXPROJ_FILE"

        # 2. 添加 DEVELOPMENT_TEAM 到所有 Manual 签名后面
        sed -i '' 's/\(CODE_SIGN_STYLE = Manual;\)/\1\
				DEVELOPMENT_TEAM = '"$TEAM_ID"';/g' "$PBXPROJ_FILE"

        # 3. 修复空的 DEVELOPMENT_TEAM
        sed -i '' 's/DEVELOPMENT_TEAM = "";/DEVELOPMENT_TEAM = '"$TEAM_ID"';/g' "$PBXPROJ_FILE"

        # 3.5 设置 CODE_SIGN_IDENTITY
        print_info "  - 设置签名身份: $CODE_SIGN_IDENTITY"
        # 替换任何现有的签名身份
        sed -i '' 's/CODE_SIGN_IDENTITY = "[^"]*";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"
        # 替换 "iPhone Developer" 等旧名称
        sed -i '' 's/CODE_SIGN_IDENTITY = "iPhone Developer";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"
        sed -i '' 's/CODE_SIGN_IDENTITY = "iPhone Distribution";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"
        sed -i '' 's/CODE_SIGN_IDENTITY = "iOS Developer";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"
        sed -i '' 's/CODE_SIGN_IDENTITY = "iOS Distribution";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"

        # 调试：显示 Xcode 项目中的签名配置
        print_info "  - Xcode 项目签名配置:"
        grep "CODE_SIGN_IDENTITY" "$PBXPROJ_FILE" | sort -u | head -5

        # 4. 修复主应用的 Bundle ID
        print_info "  - 修复主应用 Bundle ID..."
        sed -i '' 's/PRODUCT_BUNDLE_IDENTIFIER = "work\.opine\.\$(PRODUCT_NAME:rfc1034identifier)";/PRODUCT_BUNDLE_IDENTIFIER = cfd.jingo.acc;/g' "$PBXPROJ_FILE"

        # 5. 为主应用添加 provisioning profile 和 entitlements
        print_info "  - 配置主应用 provisioning profile: $PROFILE_MAIN"
        awk -v pwd="$PROJECT_ROOT" -v profile="$PROFILE_MAIN" '
/PRODUCT_NAME = JinGo;/ && in_build_settings && !has_provisioning {
    print "\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"" pwd "/platform/ios/JinGo.entitlements\";"
    print "\t\t\t\tPROVISIONING_PROFILE_SPECIFIER = \"" profile "\";"
    has_provisioning = 1
}
/buildSettings = \{/ {
    in_build_settings = 1
    has_provisioning = 0
}
/\};/ && in_build_settings {
    in_build_settings = 0
}
{ print }
' "$PBXPROJ_FILE" > "${PBXPROJ_FILE}.tmp" && mv "${PBXPROJ_FILE}.tmp" "$PBXPROJ_FILE"

        # 6. 为 PacketTunnelProvider 添加 provisioning profile
        print_info "  - 配置 PacketTunnelProvider provisioning profile: $PROFILE_PACKET_TUNNEL"
        awk -v profile="$PROFILE_PACKET_TUNNEL" '
/PRODUCT_NAME = PacketTunnelProvider;/ && in_build_settings && !has_provisioning {
    print "\t\t\t\tPROVISIONING_PROFILE_SPECIFIER = \"" profile "\";"
    has_provisioning = 1
}
/buildSettings = \{/ {
    in_build_settings = 1
    has_provisioning = 0
}
/\};/ && in_build_settings {
    in_build_settings = 0
}
{ print }
' "$PBXPROJ_FILE" > "${PBXPROJ_FILE}.tmp" && mv "${PBXPROJ_FILE}.tmp" "$PBXPROJ_FILE"

        # 7. 移除不兼容的链接器标志 -no_warn_duplicate_libraries
        # CMake 4.0+ 会添加这个标志，但 Xcode 的 clang 不支持
        print_info "  - 移除不兼容的链接器标志..."
        sed -i '' 's/-Xlinker -no_warn_duplicate_libraries//g' "$PBXPROJ_FILE"

        # 8. iOS App Extension 入口点 _NSExtensionMain
        # 注意：入口点现在通过 CMake 的 target_link_options 设置
        # 参见 cmake/Extension-PacketTunnel.cmake
        print_info "  - iOS Extension 入口点: _NSExtensionMain (CMake 配置)"

        print_success "Xcode 项目配置已修正"
        print_info "配置摘要:"
        print_info "  - 签名方式: Manual"
        print_info "  - 开发团队: $TEAM_ID"
        print_info "  - 主应用 Profile: JinGo Accelerator iOS"
        print_info "  - PacketTunnelProvider Profile: $PROFILE_PACKET_TUNNEL"
    else
        print_warning "未找到 project.pbxproj 文件，跳过 Xcode 项目修正"
    fi

    print_success "Xcode 项目生成成功: $BUILD_DIR/${APP_NAME}.xcodeproj"
}

# 编译项目
build_project() {
    print_info "开始编译 iOS 应用..."
    print_info "  配置: $CONFIGURATION"

    # 如果设置了 BUILD_KEYCHAIN 环境变量，确保 keychain 配置正确
    if [[ -n "${BUILD_KEYCHAIN:-}" ]] && [[ -f "$BUILD_KEYCHAIN" ]]; then
        print_info "配置 CI 构建 keychain: $BUILD_KEYCHAIN"
        # 添加到搜索列表
        security list-keychains -d user -s "$BUILD_KEYCHAIN" /Library/Keychains/System.keychain
        # 解锁 keychain (密码从环境变量获取，默认 123456)
        security unlock-keychain -p "${BUILD_KEYCHAIN_PASSWORD:-123456}" "$BUILD_KEYCHAIN" 2>/dev/null || true
        # 设置超时
        security set-keychain-settings -lut 21600 "$BUILD_KEYCHAIN"
        # 设置为默认
        security default-keychain -s "$BUILD_KEYCHAIN" 2>/dev/null || true
        # 允许 codesign 访问私钥 (这是关键！)
        security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "${BUILD_KEYCHAIN_PASSWORD:-123456}" "$BUILD_KEYCHAIN" 2>/dev/null || true
        print_info "Keychain 配置完成，可用证书:"
        security find-identity -v -p codesigning "$BUILD_KEYCHAIN" 2>/dev/null | head -5 || true
    fi

    cd "$BUILD_DIR"

    # 强制CMake重新扫描QML文件和资源文件的更改
    print_info "检查资源文件更新..."
    cmake . > /dev/null 2>&1

    # 再次修复 Xcode 项目设置（cmake . 可能会重新生成）
    PBXPROJ_FILE="$BUILD_DIR/${APP_NAME}.xcodeproj/project.pbxproj"
    if [[ -f "$PBXPROJ_FILE" ]]; then
        # 移除不兼容的链接器标志
        sed -i '' 's/-Xlinker -no_warn_duplicate_libraries//g' "$PBXPROJ_FILE"

        # 重新应用签名配置（cmake 会重置这些设置）
        print_info "重新应用签名配置..."
        sed -i '' 's/CODE_SIGN_STYLE = Automatic;/CODE_SIGN_STYLE = Manual;/g' "$PBXPROJ_FILE"
        sed -i '' 's/DEVELOPMENT_TEAM = "";/DEVELOPMENT_TEAM = '"$TEAM_ID"';/g' "$PBXPROJ_FILE"

        # 设置 CODE_SIGN_IDENTITY
        sed -i '' 's/CODE_SIGN_IDENTITY = "[^"]*";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"
        sed -i '' 's/CODE_SIGN_IDENTITY = "iPhone Developer";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"
        sed -i '' 's/CODE_SIGN_IDENTITY = "iPhone Distribution";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"
        sed -i '' 's/CODE_SIGN_IDENTITY = "iOS Developer";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"
        sed -i '' 's/CODE_SIGN_IDENTITY = "iOS Distribution";/CODE_SIGN_IDENTITY = "'"$CODE_SIGN_IDENTITY"'";/g' "$PBXPROJ_FILE"

        # 验证配置
        print_info "当前签名身份配置:"
        grep "CODE_SIGN_IDENTITY" "$PBXPROJ_FILE" | sort -u | head -5

        # 检查是否还有 iOS Distribution 残留
        if grep -q "iOS Distribution" "$PBXPROJ_FILE"; then
            print_warning "警告: 仍有 'iOS Distribution' 残留!"
            grep -n "iOS Distribution" "$PBXPROJ_FILE" | head -5
        fi
    fi

    # 最终确认签名配置
    print_info "最终签名配置检查:"
    print_info "  CODE_SIGN_IDENTITY 变量: $CODE_SIGN_IDENTITY"
    print_info "  TEAM_ID 变量: $TEAM_ID"

    # 执行构建并保存输出
    local BUILD_LOG="$BUILD_DIR/build.log"
    print_info "开始编译 (日志: $BUILD_LOG)..."

    if ! cmake --build . --config "$CONFIGURATION" -j$(sysctl -n hw.ncpu) 2>&1 | tee "$BUILD_LOG"; then
        print_error "CMake 构建失败！"
        print_info "最后 50 行日志:"
        tail -50 "$BUILD_LOG"
        exit 1
    fi

    # 显示关键构建信息
    grep -E "error:|warning:|BUILD SUCCEEDED|BUILD FAILED" "$BUILD_LOG" || true

    # 查找生成的 .app 包（优先使用配置对应的路径）
    if [[ -d "$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app" ]]; then
        APP_PATH="$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app"
    else
        APP_PATH=$(find "$BUILD_DIR" -path "*/$CONFIGURATION/*" -name "${APP_NAME}.app" -type d | head -n 1)
    fi

    if [[ -n "$APP_PATH" ]] && [[ -d "$APP_PATH" ]]; then
        # 验证 app bundle 包含可执行文件（iOS app 的可执行文件直接在 .app 根目录）
        local EXECUTABLE="$APP_PATH/$APP_NAME"
        if [[ ! -f "$EXECUTABLE" ]]; then
            print_error "编译失败：App bundle 缺少可执行文件"
            print_error "期望位置: $EXECUTABLE"
            print_info "App bundle 内容:"
            ls -la "$APP_PATH"
            exit 1
        fi

        print_success "编译成功！"
        print_info "应用位置: $APP_PATH"
        print_info "可执行文件: $EXECUTABLE"

        # 使用 actool 编译 Assets.xcassets（Qt + CMake 不会自动编译）
        ASSETS_DIR="$PROJECT_ROOT/platform/ios/Assets.xcassets"
        if [ -d "$ASSETS_DIR" ]; then
            print_info "使用 actool 编译应用图标..."

            # 使用 actool 编译 Assets.xcassets 到 APP bundle
            # 注意: actool 可能因模拟器版本警告返回非零退出码，但图标仍会生成
            xcrun actool "$ASSETS_DIR" \
                --compile "$APP_PATH" \
                --platform iphoneos \
                --minimum-deployment-target 14.0 \
                --app-icon AppIcon \
                --output-partial-info-plist /tmp/actool-output.plist \
                --compress-pngs || true

            # 检查图标是否生成
            if ls "$APP_PATH"/AppIcon*.png 1>/dev/null 2>&1; then
                print_success "图标编译成功"
            else
                print_warning "图标编译失败，但构建继续"
            fi
        fi
    else
        print_error "编译失败，请检查错误信息"
        exit 1
    fi
}

# 重新签名应用以包含所有entitlements
resign_app() {
    print_info "重新签名应用以包含完整的 entitlements..."

    # 优先使用配置对应的路径，避免 find 返回错误的 Debug/Release 版本
    if [[ -d "$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app" ]]; then
        APP_PATH="$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app"
    else
        APP_PATH=$(find "$BUILD_DIR" -path "*/$CONFIGURATION/*" -name "${APP_NAME}.app" -type d | head -n 1)
    fi

    if [[ -z "$APP_PATH" ]]; then
        print_error "未找到 ${APP_NAME}.app"
        return 1
    fi

    # 查找签名身份
    # 优先使用环境变量中的完整证书名称 (如 "Apple Distribution: Vi Ngo Tuong (P6H5GHKRFU)")
    if [[ "$CODE_SIGN_IDENTITY" == *":"* ]]; then
        # 环境变量包含完整的证书名称，直接使用
        SIGNING_IDENTITY="$CODE_SIGN_IDENTITY"
        print_info "使用环境变量指定的签名身份: $SIGNING_IDENTITY"
    elif [[ "$CODE_SIGN_IDENTITY" == "Apple Distribution" ]]; then
        SIGNING_IDENTITY=$(security find-identity -v -p codesigning 2>/dev/null | grep -E "Apple Distribution.*$TEAM_ID" | head -1 | awk -F '"' '{print $2}')
        if [[ -z "$SIGNING_IDENTITY" ]]; then
            SIGNING_IDENTITY=$(security find-identity -v -p codesigning 2>/dev/null | grep -E "Apple Distribution" | head -1 | awk -F '"' '{print $2}')
        fi
    else
        SIGNING_IDENTITY=$(security find-identity -v -p codesigning 2>/dev/null | grep -E "Apple Development.*$TEAM_ID|iPhone Developer.*$TEAM_ID" | head -1 | awk -F '"' '{print $2}')
        if [[ -z "$SIGNING_IDENTITY" ]]; then
            SIGNING_IDENTITY=$(security find-identity -v -p codesigning 2>/dev/null | grep -E "Apple Development|iPhone Developer" | head -1 | awk -F '"' '{print $2}')
        fi
    fi

    if [[ -z "$SIGNING_IDENTITY" ]]; then
        print_warning "未找到开发签名身份，跳过重签名"
        return 0
    fi

    print_info "使用签名身份: $SIGNING_IDENTITY"

    # CI 环境：构建 keychain 参数
    local keychain_args=""
    if [[ -n "${BUILD_KEYCHAIN:-}" ]] && [[ -f "$BUILD_KEYCHAIN" ]]; then
        keychain_args="--keychain $BUILD_KEYCHAIN"
        print_info "使用 Keychain: $BUILD_KEYCHAIN"
    fi

    # ============================================
    # 复制 Provisioning Profiles (关键步骤!)
    # ============================================
    CERT_DIR="$PROJECT_ROOT/platform/ios/cert"

    # 复制主应用的 provisioning profile
    MAIN_PROFILE="$CERT_DIR/JinGo_Accelerator_iOS.mobileprovision"
    if [[ -f "$MAIN_PROFILE" ]]; then
        print_info "复制主应用 Provisioning Profile..."
        cp "$MAIN_PROFILE" "$APP_PATH/embedded.mobileprovision"
    else
        print_error "未找到主应用 Provisioning Profile: $MAIN_PROFILE"
        exit 1
    fi

    # 复制 Extensions 的 provisioning profiles
    if [[ -d "$APP_PATH/PlugIns" ]]; then
        print_info "复制 Extensions Provisioning Profiles..."

        # PacketTunnelProvider
        PACKETTUNNEL_PROFILE="$CERT_DIR/JinGo_PacketTunnel_iOS.mobileprovision"
        if [[ -f "$PACKETTUNNEL_PROFILE" ]] && [[ -d "$APP_PATH/PlugIns/PacketTunnelProvider.appex" ]]; then
            cp "$PACKETTUNNEL_PROFILE" "$APP_PATH/PlugIns/PacketTunnelProvider.appex/embedded.mobileprovision"
            print_success "已复制 PacketTunnelProvider profile"
        else
            print_warning "未找到 PacketTunnelProvider profile: $PACKETTUNNEL_PROFILE"
        fi
    fi

    # 重签名所有 Frameworks
    if [[ -d "$APP_PATH/Frameworks" ]]; then
        print_info "重签名 Frameworks..."
        for framework in "$APP_PATH/Frameworks"/*.framework "$APP_PATH/Frameworks"/*.dylib; do
            if [[ -e "$framework" ]]; then
                /usr/bin/codesign --force --sign "$SIGNING_IDENTITY" $keychain_args \
                    --timestamp=none "$framework" 2>/dev/null || true
            fi
        done
    fi

    # 重签名 Extensions
    if [[ -d "$APP_PATH/PlugIns" ]]; then
        print_info "重签名 Extensions..."
        for appex in "$APP_PATH/PlugIns"/*.appex; do
            if [[ -e "$appex" ]]; then
                APPEX_NAME=$(basename "$appex" .appex)
                APPEX_ENTITLEMENTS="$PROJECT_ROOT/platform/ios/${APPEX_NAME}.entitlements"

                if [[ -f "$APPEX_ENTITLEMENTS" ]]; then
                    print_info "重签名 Extension: $APPEX_NAME"
                    if ! /usr/bin/codesign --force --sign "$SIGNING_IDENTITY" $keychain_args \
                        --entitlements "$APPEX_ENTITLEMENTS" \
                        --timestamp=none "$appex"; then
                        print_error "Extension 重签名失败: $APPEX_NAME"
                        exit 1
                    fi
                else
                    print_warning "未找到 Extension entitlements: $APPEX_ENTITLEMENTS"
                fi
            fi
        done
    fi

    # 重签名主 APP，包含完整的 entitlements
    print_info "重签名主 APP (包含 Network Extension 权限)..."
    MAIN_ENTITLEMENTS="$PROJECT_ROOT/platform/ios/JinGo.entitlements"

    # 检查 entitlements 文件是否存在
    if [[ ! -f "$MAIN_ENTITLEMENTS" ]]; then
        print_error "Entitlements 文件不存在: $MAIN_ENTITLEMENTS"
        exit 1
    fi

    print_info "使用 entitlements: $MAIN_ENTITLEMENTS"

    # 执行签名，不隐藏错误输出
    if ! /usr/bin/codesign --force --sign "$SIGNING_IDENTITY" $keychain_args \
        --entitlements "$MAIN_ENTITLEMENTS" \
        --timestamp=none \
        "$APP_PATH"; then
        print_error "重签名失败！"
        print_error "签名身份: $SIGNING_IDENTITY"
        print_error "Entitlements: $MAIN_ENTITLEMENTS"
        print_error "APP 路径: $APP_PATH"
        exit 1
    fi

    # 验证签名是否包含 Network Extension entitlements
    print_info "验证签名中的 entitlements..."
    if ! /usr/bin/codesign -d --entitlements :- "$APP_PATH" 2>/dev/null | grep -q "com.apple.developer.networking.networkextension"; then
        print_error "警告: 签名中未找到 Network Extension 权限！"
        print_error "这将导致 VPN 功能无法使用"
        exit 1
    fi

    print_success "重签名完成，Network Extension 权限已正确应用"
}

# 验证应用程序包
verify_app() {
    print_info "验证应用程序包..."

    # 优先使用配置对应的路径
    if [[ -d "$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app" ]]; then
        APP_PATH="$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app"
    else
        APP_PATH=$(find "$BUILD_DIR" -path "*/$CONFIGURATION/*" -name "${APP_NAME}.app" -type d | head -n 1)
    fi

    if [[ -z "$APP_PATH" ]]; then
        print_error "未找到 ${APP_NAME}.app"
        exit 1
    fi

    # 检查 Frameworks 目录（iOS 使用静态库时可能不存在，这是正常的）
    if [[ ! -d "$APP_PATH/Frameworks" ]]; then
        print_info "Frameworks 目录不存在（使用静态库，正常）"
    else
        FRAMEWORK_COUNT=$(ls -1 "$APP_PATH/Frameworks" 2>/dev/null | wc -l)
        print_success "包含 $FRAMEWORK_COUNT 个框架"
    fi

    # 检查 Info.plist
    if [[ -f "$APP_PATH/Info.plist" ]]; then
        BUNDLE_ID=$(plutil -extract CFBundleIdentifier raw "$APP_PATH/Info.plist" 2>/dev/null || echo "Unknown")
        BUNDLE_VERSION=$(plutil -extract CFBundleShortVersionString raw "$APP_PATH/Info.plist" 2>/dev/null || echo "Unknown")
        print_success "Bundle ID: $BUNDLE_ID"
        print_success "Version: $BUNDLE_VERSION"
    else
        print_warning "Info.plist 未找到"
    fi

    # 验证签名后的 entitlements
    print_info "验证 entitlements..."
    /usr/bin/codesign -d --entitlements :- "$APP_PATH" 2>/dev/null | plutil -p - 2>/dev/null | grep -i "network" && \
        print_success "✓ Network Extension 权限已包含" || \
        print_warning "⚠ Network Extension 权限可能缺失"
}

# 安装到设备或模拟器
install_to_device() {
    if [ "$INSTALL_APP" = false ] && [ "$SIMULATOR_BUILD" = false ]; then
        return
    fi

    # 优先使用配置对应的路径
    if [[ -d "$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app" ]]; then
        APP_PATH="$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app"
    else
        APP_PATH=$(find "$BUILD_DIR" -path "*/$CONFIGURATION/*" -name "${APP_NAME}.app" -type d | head -n 1)
    fi

    if [[ -z "$APP_PATH" ]]; then
        print_error "未找到应用包"
        exit 1
    fi

    # 模拟器安装
    if [ "$SIMULATOR_BUILD" = true ]; then
        print_info "准备安装到 iOS 模拟器..."

        # 列出可用的模拟器
        print_info "查找可用的模拟器..."
        BOOTED_SIM=$(xcrun simctl list devices | grep "Booted" | head -1)

        if [ -z "$BOOTED_SIM" ]; then
            print_warning "没有正在运行的模拟器"
            print_info "可用的模拟器:"
            xcrun simctl list devices | grep -E "iPhone|iPad" | head -5
            echo ""
            print_info "启动模拟器:"
            echo "  open -a Simulator"
            echo "或使用 Xcode 运行模拟器"
            return
        fi

        # 提取模拟器 UDID
        SIM_UDID=$(echo "$BOOTED_SIM" | grep -oE '[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}')

        if [ -n "$SIM_UDID" ]; then
            print_info "安装到模拟器: $SIM_UDID"
            xcrun simctl install "$SIM_UDID" "$APP_PATH"

            # 获取 Bundle ID
            BUNDLE_ID=$(plutil -extract CFBundleIdentifier raw "$APP_PATH/Info.plist" 2>/dev/null || echo "cfd.jingo.acc")

            print_success "应用安装成功！"
            print_info "启动应用:"
            echo "  xcrun simctl launch $SIM_UDID $BUNDLE_ID"
        fi

        return
    fi

    # 真机安装
    if [ "$INSTALL_APP" = false ]; then
        return
    fi

    print_info "准备安装应用到 iOS 设备..."

    # 查找设备
    if [ -z "$DEVICE_UDID" ]; then
        print_info "未指定设备，查找已连接的 iOS 设备..."
        DEVICE_LIST=$(xcrun devicectl list devices 2>/dev/null | grep -E "iPhone|iPad" || true)

        if [ -z "$DEVICE_LIST" ]; then
            print_error "未找到已连接的 iOS 设备"
            exit 1
        fi

        # 提取第一个设备的 UDID
        DEVICE_UDID=$(echo "$DEVICE_LIST" | head -1 | grep -oE '[0-9A-F]{8}-[0-9A-F]{16}' | head -1)

        if [ -z "$DEVICE_UDID" ]; then
            print_error "无法获取设备 UDID"
            exit 1
        fi
    fi

    print_info "目标设备: $DEVICE_UDID"
    print_info "开始安装..."

    xcrun devicectl device install app \
        --device "$DEVICE_UDID" \
        "$APP_PATH"

    print_success "应用安装成功！"
    print_info "您可以在设备上找到 JinGoVPN 应用"
}

# ============================================================================
# 复制到 release 目录
# ============================================================================
copy_to_release() {
    if [[ "$CONFIGURATION" != "Release" ]]; then
        return
    fi

    print_info "复制构建产物到 release 目录..."

    # 创建 release 目录
    mkdir -p "$RELEASE_DIR"

    # 优先使用配置对应的路径
    if [[ -d "$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app" ]]; then
        APP_PATH="$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app"
    else
        APP_PATH=$(find "$BUILD_DIR" -path "*/$CONFIGURATION/*" -name "${APP_NAME}.app" -type d | head -n 1)
    fi

    if [[ -n "$APP_PATH" ]] && [[ -d "$APP_PATH" ]]; then
        # 获取版本号
        local version=$(plutil -extract CFBundleShortVersionString raw "$APP_PATH/Info.plist" 2>/dev/null || echo "1.0.0")

        # 复制 .app 包 (压缩为 zip)
        # 使用统一命名: {brand}-{version}-{date}-{platform}.{ext}
        local app_zip=$(generate_output_name "$version" "zip")
        print_info "创建应用压缩包: $app_zip"
        (cd "$(dirname "$APP_PATH")" && zip -r -q "$RELEASE_DIR/$app_zip" "$(basename "$APP_PATH")")

        if [[ -f "$RELEASE_DIR/$app_zip" ]]; then
            print_success "已复制: $RELEASE_DIR/$app_zip"
        fi

        # 创建 IPA 文件
        # 使用统一命名: {brand}-{version}-{date}-{platform}.{ext}
        local ipa_file=$(generate_output_name "$version" "ipa")
        print_info "创建 IPA 文件: $ipa_file"
        local payload_dir=$(mktemp -d)
        mkdir -p "$payload_dir/Payload"
        cp -R "$APP_PATH" "$payload_dir/Payload/"
        (cd "$payload_dir" && zip -r -q "$RELEASE_DIR/$ipa_file" Payload)
        rm -rf "$payload_dir"

        if [[ -f "$RELEASE_DIR/$ipa_file" ]]; then
            print_success "已创建: $RELEASE_DIR/$ipa_file"
        fi
    fi

    print_success "构建产物已复制到: $RELEASE_DIR"
}

# 主函数
main() {
    echo ""
    echo "=================================================="
    echo "      JinGoVPN iOS 构建脚本"
    echo "=================================================="
    echo ""

    parse_args "$@"
    apply_brand_customization
    check_requirements
    clean_build_dir
    generate_xcode_project

    if [ "$XCODE_ONLY" = true ]; then
        print_success "Xcode 项目已生成，您可以在 Xcode 中打开:"
        print_info "  open $BUILD_DIR/JinGo.xcodeproj"
        exit 0
    fi

    refresh_extension_plist
    build_project
    resign_app
    verify_app
    copy_to_release
    install_to_device

    echo ""
    print_success "=================================================="
    print_success "                全部完成！"
    print_success "=================================================="
    echo ""

    # 显示后续步骤提示
    if [ "$SIMULATOR_BUILD" = true ]; then
        print_info "下一步（模拟器）:"
        echo "  1. 启动 iOS 模拟器"
        echo "  2. 应用已自动安装（如果模拟器正在运行）"
        echo "  3. 在模拟器中查找并运行 JinGo 应用"
    elif [ "$INSTALL_APP" = true ]; then
        print_info "下一步（真机）:"
        echo "  1. 在设备上信任开发者证书（如需要）:"
        echo "     设置 -> 通用 -> VPN与设备管理 -> 开发者App"
        echo "  2. 运行应用程序"
    else
        print_info "下一步:"
        echo "  1. 在 Xcode 中打开项目:"
        echo "     open $BUILD_DIR/${APP_NAME}.xcodeproj"
        echo "  2. 选择设备或模拟器并运行"
    fi
    echo ""
}

# 执行主函数
main "$@"
