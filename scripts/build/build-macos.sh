#!/bin/bash
# ============================================================================
# JinGo VPN - macOS 构建脚本
# ============================================================================
# 描述：用于构建 JinGo VPN macOS 桌面客户端的自动化脚本
#
# 功能：编译 Debug/Release 版本、代码签名、翻译更新、DMG 创建
# 依赖：macOS 12.0+, Xcode 14.0+, CMake 3.21+, Qt 6.5+
# 版本：1.2.0
# ============================================================================

set -e  # 遇到错误立即退出
set -o pipefail  # 管道中的错误也触发退出

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
# Qt macOS 安装路径
# 优先使用环境变量 QT_MACOS_PATH 或 Qt6_DIR，否则使用默认值
# 本地开发请修改下面的默认路径，或设置环境变量
# 示例: "/Users/yourname/Qt/6.8.0/macos" 或 "/opt/Qt/6.8.0/macos"
if [[ -n "${QT_MACOS_PATH:-}" ]]; then
    : # 使用已设置的 QT_MACOS_PATH
elif [[ -n "${Qt6_DIR:-}" ]]; then
    QT_MACOS_PATH="$Qt6_DIR"
else
    QT_MACOS_PATH="/Volumes/mindata/Applications/Qt/6.10.0/macos"
fi

# --------------------- 脚本初始化 ---------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载白标资源复制脚本
if [[ -f "$SCRIPT_DIR/copy-brand-assets.sh" ]]; then
    source "$SCRIPT_DIR/copy-brand-assets.sh"
fi

# --------------------- macOS 配置 ---------------------
# 最低 macOS 版本
MACOS_DEPLOYMENT_TARGET="12.0"

# 目标架构: "arm64", "x86_64", 或 "arm64;x86_64" (Universal)
MACOS_ARCHITECTURES="arm64;x86_64"

# --------------------- Apple 开发者配置 (可选) ---------------------
# macOS 构建默认不需要签名，以管理员模式运行 TUN 设备
# 如需签名分发，请取消下面的注释并填入您的开发者信息
#
# 开发团队 ID (可通过环境变量 APPLE_DEVELOPMENT_TEAM 覆盖)
# TEAM_ID="${APPLE_DEVELOPMENT_TEAM:-YOUR_TEAM_ID}"
TEAM_ID="${APPLE_DEVELOPMENT_TEAM:-}"

# 签名身份 (可通过环境变量 APPLE_CODE_SIGN_IDENTITY 覆盖)
# macOS 分发使用 Developer ID Application
# CODE_SIGN_IDENTITY="${APPLE_CODE_SIGN_IDENTITY:-Developer ID Application}"
CODE_SIGN_IDENTITY="${APPLE_CODE_SIGN_IDENTITY:-}"

# --------------------- 应用信息 ---------------------
APP_NAME="JinGo"
# Bundle ID 默认值 (可通过环境变量 APP_BUNDLE_ID 或 --bundle-id 参数覆盖)
APP_BUNDLE_ID="${APP_BUNDLE_ID:-cfd.jingo.acc}"

# --------------------- 输出命名 ---------------------
# 获取构建日期 (YYYYMMDD 格式)
BUILD_DATE=$(date +%Y%m%d)

# 生成输出文件名: {brand}-{version}-{date}-{platform}.{ext}
# 参数: $1=version, $2=extension (可选，默认无)
generate_output_name() {
    local version="${1:-1.0.0}"
    local ext="${2:-}"
    local brand="${BRAND_NAME:-jingo}"
    local platform="macos"

    if [[ -n "$ext" ]]; then
        echo "${brand}-${version}-${BUILD_DATE}-${platform}.${ext}"
    else
        echo "${brand}-${version}-${BUILD_DATE}-${platform}"
    fi
}

# ============================================================================
# 脚本内部变量 (一般不需要修改)
# ============================================================================
# SCRIPT_DIR 已在上面定义
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-macos"
RELEASE_DIR="$PROJECT_ROOT/release"
CONFIGURATION="Debug"
CLEAN_BUILD=false
OPEN_APP=false
XCODE_ONLY=false
SKIP_SIGN=true   # macOS 默认不签名，使用管理员模式运行
CREATE_DMG=false
UPDATE_TRANSLATIONS=false
VERBOSE=false
BRAND_NAME=""

# ============================================================================
# 颜色定义
# ============================================================================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# 错误处理
trap 'on_error $LINENO' ERR

on_error() {
    local line=$1
    print_error "脚本在第 $line 行发生错误"
    exit 1
}

# ============================================================================
# 辅助函数
# ============================================================================
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

print_step() {
    echo -e "\n${CYAN}${BOLD}>>> $1${NC}\n"
}

# 格式化文件大小
format_size() {
    local size=$1
    if [[ $size -ge 1073741824 ]]; then
        echo "$(echo "scale=2; $size/1073741824" | bc)GB"
    elif [[ $size -ge 1048576 ]]; then
        echo "$(echo "scale=2; $size/1048576" | bc)MB"
    elif [[ $size -ge 1024 ]]; then
        echo "$(echo "scale=2; $size/1024" | bc)KB"
    else
        echo "${size}B"
    fi
}

# ============================================================================
# 帮助信息
# ============================================================================
show_help() {
    cat << EOF
${BOLD}JinGoVPN macOS 构建脚本 v1.1.0${NC}

${CYAN}用法:${NC}
    $0 [选项]

${CYAN}构建选项:${NC}
    -c, --clean          清理构建目录后重新构建
    -d, --debug          Debug 模式构建（默认）
    -r, --release        Release 模式构建
    -o, --open           构建完成后自动打开应用
    -x, --xcode          仅生成 Xcode 项目（不编译）

${CYAN}签名与打包:${NC}
    --sign               启用代码签名（需要配置 TEAM_ID 和证书）
    --dmg                创建 DMG 安装镜像（仅 Release 模式）

${CYAN}翻译选项:${NC}
    -t, --translate      更新翻译（运行 Python 翻译脚本）

${CYAN}签名配置 (使用 --sign 时需要):${NC}
    --team-id ID         Apple 开发团队 ID
    --sign-identity ID   代码签名身份 (默认: Developer ID Application)

${CYAN}白标定制:${NC}
    -b, --brand NAME     应用白标定制（从 white-labeling/<NAME> 加载配置）
    --bundle-id ID       指定 Bundle ID (如 cfd.jingo.vpn)
                         扩展会自动派生: ID.PacketTunnelProvider

${CYAN}其他选项:${NC}
    -v, --verbose        显示详细输出
    -h, --help           显示此帮助信息

${CYAN}环境变量:${NC}
    APPLE_DEVELOPMENT_TEAM    Apple 开发团队 ID（签名时必需）
    APPLE_CODE_SIGN_IDENTITY  代码签名身份（签名时必需，如 "Developer ID Application"）
    QT_MACOS_PATH             Qt macOS 安装路径
    APP_BUNDLE_ID             应用 Bundle ID（默认: cfd.jingo.acc）

${CYAN}示例:${NC}
    # 编译 Debug 版本
    $0

    # 清理并编译 Release 版本
    $0 --clean --release

    # 编译并自动打开应用
    $0 --open

    # 仅生成 Xcode 项目
    $0 --xcode

    # Release 版本并创建 DMG
    $0 --release --dmg

    # 更新翻译后编译
    $0 --translate

    # 使用白标定制编译
    $0 --brand jingo --release --dmg

${CYAN}输出目录:${NC}
    Debug:   $PROJECT_ROOT/build/macos/bin/Debug/
    Release: $PROJECT_ROOT/build/macos/bin/Release/
    DMG:     $PROJECT_ROOT/build/macos/

EOF
}

# ============================================================================
# 参数解析
# ============================================================================
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
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
            -o|--open)
                OPEN_APP=true
                shift
                ;;
            -x|--xcode)
                XCODE_ONLY=true
                shift
                ;;
            -s|--sign)
                SKIP_SIGN=false
                shift
                ;;
            --skip-sign)
                # 保留向后兼容（但现在默认就是跳过签名）
                SKIP_SIGN=true
                shift
                ;;
            --dmg)
                CREATE_DMG=true
                shift
                ;;
            -t|--translate)
                UPDATE_TRANSLATIONS=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
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
    # macOS 平台默认使用品牌 2
    local brand_id="${BRAND_NAME:-2}"

    print_step "复制白标资源 (品牌: $brand_id)"

    # 使用 copy-brand-assets.sh 中的函数复制资源
    if type copy_brand_assets &> /dev/null; then
        if ! copy_brand_assets "$brand_id"; then
            print_warning "白标资源复制失败，使用默认资源继续"
        fi
    else
        print_warning "copy_brand_assets 函数未加载，跳过白标资源复制"
    fi
}

# ============================================================================
# 环境检查
# ============================================================================
check_requirements() {
    print_step "检查构建环境"

    # 检查 macOS
    if [[ "$OSTYPE" != "darwin"* ]]; then
        print_error "此脚本只能在 macOS 上运行"
        exit 1
    fi
    local macos_version=$(sw_vers -productVersion)
    print_success "macOS: $macos_version"

    # 检查 CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake 未安装"
        print_info "安装方法: brew install cmake"
        exit 1
    fi
    local cmake_version=$(cmake --version | head -n1 | awk '{print $3}')
    print_success "CMake: $cmake_version"

    # 检查 Xcode
    if ! command -v xcodebuild &> /dev/null; then
        print_error "Xcode 未安装"
        print_info "请从 Mac App Store 安装 Xcode"
        exit 1
    fi
    local xcode_version=$(xcodebuild -version | head -n1)
    print_success "Xcode: $xcode_version"

    # 检查 Qt
    if [[ -d "$QT_MACOS_PATH" ]]; then
        local qt_version=$(basename "$(dirname "$QT_MACOS_PATH")")
        print_success "Qt: $qt_version ($QT_MACOS_PATH)"
    else
        print_error "Qt macOS 未找到: $QT_MACOS_PATH"
        print_info "请设置 QT_MACOS_PATH 环境变量"
        exit 1
    fi

    # 签名模式检查
    if [[ "$SKIP_SIGN" == true ]]; then
        print_info "签名: 已禁用 (默认模式，使用 --sign 启用)"
    else
        # 启用签名时检查 TEAM_ID 配置
        if [[ -z "$TEAM_ID" ]]; then
            print_error "启用签名需要配置 TEAM_ID"
            print_info "使用 --team-id YOUR_TEAM_ID 或设置 APPLE_DEVELOPMENT_TEAM 环境变量"
            exit 1
        fi
        if [[ -z "$CODE_SIGN_IDENTITY" ]]; then
            CODE_SIGN_IDENTITY="Developer ID Application"
        fi
        if [[ -n "${BUILD_KEYCHAIN:-}" ]]; then
            print_info "签名: CI 模式 (Keychain: $BUILD_KEYCHAIN)"
        else
            print_info "签名: 本地开发模式"
        fi
        print_info "开发团队: $TEAM_ID"
        print_info "签名身份: $CODE_SIGN_IDENTITY"
    fi

    echo ""
    print_success "构建环境检查通过"
}

# ============================================================================
# 更新翻译内容（Python 脚本）
# ============================================================================
update_translations() {
    if [[ "$UPDATE_TRANSLATIONS" != true ]]; then
        return
    fi

    print_step "更新翻译内容"

    local translate_script="$SCRIPT_DIR/translate_ts.py"

    if [[ ! -f "$translate_script" ]]; then
        print_warning "翻译脚本不存在: $translate_script"
        return
    fi

    if ! command -v python3 &> /dev/null; then
        print_warning "python3 未安装，跳过翻译更新"
        return
    fi

    print_info "运行翻译脚本..."
    if python3 "$translate_script" 2>&1; then
        print_success "翻译内容已更新"
    else
        print_warning "翻译脚本执行失败"
    fi
}

# ============================================================================
# 生成翻译文件
# ============================================================================
generate_translations() {
    local translations_dir="$PROJECT_ROOT/resources/translations"
    local lrelease="$QT_MACOS_PATH/bin/lrelease"

    if [[ ! -x "$lrelease" ]]; then
        print_warning "lrelease 未找到，跳过翻译生成"
        return
    fi

    if [[ ! -d "$translations_dir" ]]; then
        print_warning "翻译目录不存在: $translations_dir"
        return
    fi

    # 检查是否需要重新生成翻译文件
    local need_regenerate=false
    local ts_count=0
    local qm_count=0
    local languages=()

    for ts_file in "$translations_dir"/*.ts; do
        if [[ -f "$ts_file" ]]; then
            ts_count=$((ts_count + 1))
            local base_name=$(basename "$ts_file" .ts)
            local qm_file="$translations_dir/$base_name.qm"
            local lang=$(echo "$base_name" | sed 's/jingo_//')
            languages+=("$lang")

            # 检查 .qm 是否存在且比 .ts 新
            if [[ ! -f "$qm_file" ]] || [[ "$ts_file" -nt "$qm_file" ]]; then
                need_regenerate=true
            fi
        fi
    done

    if [[ "$need_regenerate" == false ]] && [[ "$CLEAN_BUILD" != true ]]; then
        print_info "翻译文件已是最新，跳过生成"
        return
    fi

    print_step "生成翻译文件 (.qm)"

    # 编译所有 .ts 文件为 .qm 文件
    for ts_file in "$translations_dir"/*.ts; do
        if [[ -f "$ts_file" ]]; then
            local base_name=$(basename "$ts_file" .ts)

            if [[ "$VERBOSE" == true ]]; then
                print_info "编译: $base_name.ts"
                "$lrelease" "$ts_file" -qm "$translations_dir/$base_name.qm" 2>&1 | grep -E "Generated|Ignored" || true
            else
                "$lrelease" "$ts_file" -qm "$translations_dir/$base_name.qm" > /dev/null 2>&1
            fi

            if [[ -f "$translations_dir/$base_name.qm" ]]; then
                qm_count=$((qm_count + 1))
            fi
        fi
    done

    if [[ $qm_count -gt 0 ]]; then
        print_success "生成 $qm_count 个翻译文件"
        local lang_list=$(IFS=','; echo "${languages[*]}")
        print_info "支持语言: $lang_list"
    else
        print_warning "未生成翻译文件"
    fi
}

# ============================================================================
# 清理构建目录
# ============================================================================
clean_build_dir() {
    if [[ "$CLEAN_BUILD" == true ]]; then
        print_step "清理构建目录"

        if [[ -d "$BUILD_DIR" ]]; then
            print_info "删除: $BUILD_DIR"
            rm -rf "$BUILD_DIR"
        fi

        # 清理 Xcode DerivedData
        local derived_data=~/Library/Developer/Xcode/DerivedData
        local jingo_derived=$(find "$derived_data" -maxdepth 1 -name "JinGo-*" -type d 2>/dev/null || true)
        if [[ -n "$jingo_derived" ]]; then
            print_info "清理 Xcode DerivedData..."
            rm -rf $jingo_derived
        fi

        print_success "构建目录已清理"
    fi
}

# ============================================================================
# 生成 Xcode 项目
# ============================================================================
generate_xcode_project() {
    local xcode_project="$BUILD_DIR/${APP_NAME}.xcodeproj/project.pbxproj"
    local cmake_cache="$BUILD_DIR/CMakeCache.txt"

    # 检查是否需要重新生成 Xcode 项目
    if [[ -f "$xcode_project" ]] && [[ -f "$cmake_cache" ]] && [[ "$CLEAN_BUILD" != true ]]; then
        # 检查 CMakeLists.txt 是否比缓存新
        if [[ "$PROJECT_ROOT/CMakeLists.txt" -ot "$cmake_cache" ]]; then
            print_info "Xcode 项目已存在，跳过生成（使用 --clean 强制重新生成）"
            return
        fi
    fi

    print_step "生成 Xcode 项目"

    print_info "项目根目录: $PROJECT_ROOT"
    print_info "构建目录: $BUILD_DIR"
    print_info "开发团队: $TEAM_ID"
    print_info "签名身份: $CODE_SIGN_IDENTITY"
    print_info "Bundle ID: $APP_BUNDLE_ID"

    mkdir -p "$BUILD_DIR"

    local cmake_args=(
        -S "$PROJECT_ROOT"
        -B "$BUILD_DIR"
        -G Xcode
        -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
        -DCMAKE_PREFIX_PATH="$QT_MACOS_PATH"
        -DAPPLE_DEVELOPMENT_TEAM="$TEAM_ID"
        -DAPPLE_CODE_SIGN_IDENTITY="$CODE_SIGN_IDENTITY"
        -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="$TEAM_ID"
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="$CODE_SIGN_IDENTITY"
        -DAPP_BUNDLE_ID="$APP_BUNDLE_ID"
    )

    # CI 环境：传递 keychain 路径给 CMake (用于 Xcode 签名)
    if [[ -n "${BUILD_KEYCHAIN:-}" ]] && [[ -f "$BUILD_KEYCHAIN" ]]; then
        cmake_args+=(-DBUILD_KEYCHAIN_PATH="$BUILD_KEYCHAIN")
        print_info "Keychain 路径: $BUILD_KEYCHAIN"
    fi

    # 跳过签名：传递给 CMake 禁用 Xcode 签名要求
    # macOS 管理员模式 TUN 不需要 Network Extension，可以无签名运行
    if [[ "$SKIP_SIGN" == true ]]; then
        cmake_args+=(-DSKIP_CODE_SIGNING=ON)
        print_info "CMake: 禁用代码签名 (SKIP_CODE_SIGNING=ON)"
    fi

    echo ""
    cmake "${cmake_args[@]}" 2>&1 | grep -v "^--" | head -20 || true

    if [[ -f "$xcode_project" ]]; then
        print_success "Xcode 项目生成成功"
    else
        print_error "Xcode 项目生成失败"
        exit 1
    fi
}

# ============================================================================
# 刷新 Extension Info.plist 缓存 (确保使用最新版本)
# ============================================================================
refresh_extension_plist() {
    print_step "刷新 Extension Info.plist"

    local source_plist="$PROJECT_ROOT/src/extensions/PacketTunnelProvider/Info.plist"
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
        "$BUILD_DIR/bin/Debug/PacketTunnelProvider.systemextension"
        "$BUILD_DIR/bin/Release/PacketTunnelProvider.systemextension"
        "$BUILD_DIR/bin/Debug/JinGo.app/Contents/Library/SystemExtensions"
        "$BUILD_DIR/bin/Release/JinGo.app/Contents/Library/SystemExtensions"
    )

    for artifact in "${extension_artifacts[@]}"; do
        if [[ -e "$artifact" ]]; then
            rm -rf "$artifact"
            print_info "已清理: $(basename "$artifact")"
        fi
    done
}

# ============================================================================
# 修复 Extension 复制 (确保完整复制到 App Bundle)
# ============================================================================
fix_extension_copy() {
    print_step "修复 Extension 复制"

    local source_ext="$BUILD_DIR/bin/$CONFIGURATION/PacketTunnelProvider.systemextension"
    local dest_ext="$BUILD_DIR/bin/$CONFIGURATION/JinGo.app/Contents/Library/SystemExtensions/${APP_BUNDLE_ID}.PacketTunnelProvider.systemextension"

    if [[ -d "$source_ext" ]]; then
        # 确保目标目录存在
        mkdir -p "$(dirname "$dest_ext")"

        # 完全删除旧的并重新复制
        rm -rf "$dest_ext"
        cp -R "$source_ext" "$dest_ext"

        # 验证复制是否完整
        if [[ -f "$dest_ext/Contents/Info.plist" ]] && [[ -d "$dest_ext/Contents/_CodeSignature" ]]; then
            print_success "Extension 复制完整"
            local bundle_id=$(plutil -extract CFBundleIdentifier raw "$dest_ext/Contents/Info.plist" 2>/dev/null)
            local version=$(plutil -extract CFBundleVersion raw "$dest_ext/Contents/Info.plist" 2>/dev/null)
            print_info "Bundle ID: $bundle_id, Version: $version"
        else
            print_error "Extension 复制不完整！"
            ls -la "$dest_ext/Contents/" 2>/dev/null
            return 1
        fi
    else
        print_warning "源 Extension 不存在: $source_ext"
        return 1
    fi
}

# ============================================================================
# 编译项目
# ============================================================================
build_project() {
    print_step "编译 $CONFIGURATION 版本"

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

    # 检查 QML 和资源文件是否有更新，如有则刷新 CMake 配置
    local need_refresh=false
    local cmake_cache="$BUILD_DIR/CMakeCache.txt"

    if [[ -f "$cmake_cache" ]]; then
        # 检查 QML 文件是否比 CMakeCache 新
        local qml_newer=$(find "$PROJECT_ROOT/resources/qml" -name "*.qml" -newer "$cmake_cache" 2>/dev/null | head -1)
        local qrc_newer=$(find "$PROJECT_ROOT/resources" -name "*.qrc" -newer "$cmake_cache" 2>/dev/null | head -1)
        local qm_newer=$(find "$PROJECT_ROOT/resources/translations" -name "*.qm" -newer "$cmake_cache" 2>/dev/null | head -1)

        if [[ -n "$qml_newer" ]] || [[ -n "$qrc_newer" ]] || [[ -n "$qm_newer" ]]; then
            need_refresh=true
            print_info "检测到资源文件更新，刷新 CMake 配置..."
        fi
    fi

    if [[ "$need_refresh" == true ]]; then
        cmake . > /dev/null 2>&1 || true
    fi

    # 获取 CPU 核心数
    local cpu_count=$(sysctl -n hw.ncpu)
    print_info "使用 $cpu_count 个并行任务编译"

    # 开始计时
    local start_time=$(date +%s)

    # 编译
    echo ""
    local build_log="$BUILD_DIR/build.log"
    cmake --build . --config "$CONFIGURATION" -j$cpu_count 2>&1 | tee "$build_log"
    local build_result=${PIPESTATUS[0]}

    # 检查编译结果
    if [[ $build_result -ne 0 ]] || grep -q "BUILD FAILED" "$build_log"; then
        print_error "编译失败 (exit code: $build_result)"
        echo ""
        echo "=== 错误信息 ==="
        grep -i "error:" "$build_log" | head -30
        echo ""
        echo "=== 最后 50 行日志 ==="
        tail -50 "$build_log"
        exit 1
    fi

    # 计算编译时间
    local end_time=$(date +%s)
    local build_time=$((end_time - start_time))

    # 查找生成的 .app
    APP_PATH="$BUILD_DIR/bin/$CONFIGURATION/${APP_NAME}.app"
    if [[ ! -d "$APP_PATH" ]]; then
        APP_PATH=$(find "$BUILD_DIR" -name "${APP_NAME}.app" -type d 2>/dev/null | head -n 1)
    fi

    # 验证主可执行文件存在
    local main_executable="$APP_PATH/Contents/MacOS/${APP_NAME}"
    if [[ ! -f "$main_executable" ]]; then
        print_error "编译失败: 主可执行文件不存在"
        print_error "期望路径: $main_executable"
        ls -la "$APP_PATH/Contents/MacOS/" 2>/dev/null || true
        exit 1
    fi

    echo ""
    print_success "编译成功！耗时 ${build_time} 秒"

    # 显示应用大小
    local app_size=$(du -sk "$APP_PATH" | awk '{print $1 * 1024}')
    print_info "应用大小: $(format_size $app_size)"

    # 显示架构
    local archs=$(lipo -archs "$main_executable" 2>/dev/null || echo "unknown")
    print_info "架构: $archs"
}

# ============================================================================
# 代码签名
# ============================================================================
sign_app() {
    if [[ "$SKIP_SIGN" == true ]]; then
        print_warning "跳过代码签名"
        return
    fi

    print_step "代码签名"

    APP_PATH=$(find "$BUILD_DIR" -name "${APP_NAME}.app" -type d 2>/dev/null | head -n 1)

    if [[ -z "$APP_PATH" ]]; then
        print_error "未找到应用包"
        return 1
    fi

    # 证书和描述文件路径
    local CERT_DIR="$PROJECT_ROOT/platform/macos/cert"
    local ENTITLEMENTS_DIR="$PROJECT_ROOT/platform/macos"

    # CI 环境：构建 keychain 参数
    local keychain_args=""
    local keychain_search=""
    if [[ -n "${BUILD_KEYCHAIN:-}" ]] && [[ -f "$BUILD_KEYCHAIN" ]]; then
        keychain_args="--keychain $BUILD_KEYCHAIN"
        keychain_search="$BUILD_KEYCHAIN"
        print_info "使用 Keychain: $BUILD_KEYCHAIN"
    fi

    # 获取签名身份
    # 优先使用环境变量中的完整证书名称 (如 "Developer ID Application: Vi Ngo Tuong (P6H5GHKRFU)")
    local signing_identity=""
    if [[ "$CODE_SIGN_IDENTITY" == *":"* ]]; then
        # 环境变量包含完整的证书名称，直接使用
        signing_identity="$CODE_SIGN_IDENTITY"
        print_info "使用环境变量指定的签名身份: $signing_identity"
    else
        # 搜索证书 (CI 模式使用指定 keychain)
        local find_cmd="security find-identity -v -p codesigning"
        if [[ -n "$keychain_search" ]]; then
            find_cmd="$find_cmd $keychain_search"
        fi

        # macOS 分发优先使用 Developer ID Application
        signing_identity=$(eval "$find_cmd" 2>/dev/null | \
            grep "Developer ID Application" | head -1 | \
            awk -F '"' '{print $2}')

        # 如果没有 Developer ID，降级到 Apple Development
        if [[ -z "$signing_identity" ]]; then
            signing_identity=$(eval "$find_cmd" 2>/dev/null | \
                grep "Apple Development" | head -1 | \
                awk -F '"' '{print $2}')
        fi
    fi

    if [[ -z "$signing_identity" ]]; then
        print_warning "未找到有效签名身份，跳过签名"
        if [[ -n "$keychain_search" ]]; then
            print_info "Keychain 中的证书:"
            security find-identity -v -p codesigning "$keychain_search" 2>/dev/null | head -5 || true
        fi
        return
    fi

    print_info "签名身份: $signing_identity"

    # ============================================================================
    # Step 1: 嵌入描述文件 (Provisioning Profiles)
    # ============================================================================
    print_info "嵌入描述文件..."

    # 主应用描述文件
    if [[ -f "$CERT_DIR/JinGo_Accelerator_macOS.provisionprofile" ]]; then
        cp "$CERT_DIR/JinGo_Accelerator_macOS.provisionprofile" "$APP_PATH/Contents/embedded.provisionprofile"
        print_success "  主应用: embedded.provisionprofile"
    else
        print_warning "  主应用描述文件未找到: $CERT_DIR/JinGo_Accelerator_macOS.provisionprofile"
    fi

    # PacketTunnelProvider 描述文件 (支持 System Extension 和 App Extension)
    local PTP_SYSEXT="$APP_PATH/Contents/Library/SystemExtensions/cfd.jingo.acc.PacketTunnelProvider.systemextension"
    local PTP_APPEX="$APP_PATH/Contents/PlugIns/PacketTunnelProvider.appex"
    if [[ -d "$PTP_SYSEXT" && -f "$CERT_DIR/JinGo_PacketTunnel_macOS.provisionprofile" ]]; then
        cp "$CERT_DIR/JinGo_PacketTunnel_macOS.provisionprofile" "$PTP_SYSEXT/Contents/embedded.provisionprofile"
        print_success "  PacketTunnelProvider (SystemExtension): embedded.provisionprofile"
    elif [[ -d "$PTP_APPEX" && -f "$CERT_DIR/JinGo_PacketTunnel_macOS.provisionprofile" ]]; then
        cp "$CERT_DIR/JinGo_PacketTunnel_macOS.provisionprofile" "$PTP_APPEX/Contents/embedded.provisionprofile"
        print_success "  PacketTunnelProvider (AppExtension): embedded.provisionprofile"
    fi

    # ============================================================================
    # Step 2: 修复 Qt 插件 rpath (macdeployqt 设置的路径不正确)
    # ============================================================================
    print_info "修复 Qt 插件 rpath..."
    if [[ -x "$PROJECT_ROOT/scripts/signing/fix_plugins_rpath.sh" ]]; then
        bash "$PROJECT_ROOT/scripts/signing/fix_plugins_rpath.sh" "$APP_PATH"
    else
        # 内联修复
        if [[ -d "$APP_PATH/Contents/PlugIns" ]]; then
            find "$APP_PATH/Contents/PlugIns" -name "*.dylib" | while read dylib; do
                if ! otool -l "$dylib" 2>/dev/null | grep -q "@loader_path/../../Frameworks"; then
                    install_name_tool -add_rpath "@loader_path/../../Frameworks" "$dylib" 2>/dev/null || true
                fi
            done
        fi
    fi

    # ============================================================================
    # Step 2.5: 清理绝对 rpath（移除 CI 构建目录路径）
    # ============================================================================
    print_info "清理绝对 rpath..."
    local main_executable="$APP_PATH/Contents/MacOS/JinGo"
    if [[ -f "$main_executable" ]]; then
        otool -l "$main_executable" 2>/dev/null | grep -A2 "LC_RPATH" | grep "path " | awk '{print $2}' | while read rpath; do
            # 删除不以 @ 开头的绝对路径
            if [[ "$rpath" != @* ]]; then
                install_name_tool -delete_rpath "$rpath" "$main_executable" 2>/dev/null || true
                print_info "  已删除: $rpath"
            fi
        done
    fi
    # 清理 Extensions
    for appex in "$APP_PATH/Contents/PlugIns"/*.appex; do
        if [[ -d "$appex" ]]; then
            local appex_name=$(basename "$appex" .appex)
            local appex_executable="$appex/Contents/MacOS/$appex_name"
            if [[ -f "$appex_executable" ]]; then
                otool -l "$appex_executable" 2>/dev/null | grep -A2 "LC_RPATH" | grep "path " | awk '{print $2}' | while read rpath; do
                    if [[ "$rpath" != @* ]]; then
                        install_name_tool -delete_rpath "$rpath" "$appex_executable" 2>/dev/null || true
                    fi
                done
            fi
        fi
    done

    # ============================================================================
    # Step 3: 确保 Keychain 已解锁 (CI 环境)
    # ============================================================================
    if [[ -n "${BUILD_KEYCHAIN:-}" ]] && [[ -f "$BUILD_KEYCHAIN" ]]; then
        # 尝试解锁 keychain (如果环境变量中有密码)
        if [[ -n "${KEYCHAIN_PASSWORD:-}" ]]; then
            security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$BUILD_KEYCHAIN" 2>/dev/null || true
        fi
        # 设置 keychain 搜索列表，确保 build keychain 在前面
        security list-keychains -d user -s "$BUILD_KEYCHAIN" $(security list-keychains -d user | tr -d '"') 2>/dev/null || true
        # 设置 keychain 为默认
        security default-keychain -s "$BUILD_KEYCHAIN" 2>/dev/null || true
    fi

    # ============================================================================
    # Step 4: 签名 Frameworks (主应用和 Extension 的)
    # ============================================================================
    if [[ -d "$APP_PATH/Contents/Frameworks" ]]; then
        print_info "签名 Frameworks..."
        local sign_failed=false
        for framework in "$APP_PATH/Contents/Frameworks"/*.framework "$APP_PATH/Contents/Frameworks"/*.dylib; do
            if [[ -e "$framework" ]]; then
                if ! codesign --force --sign "$signing_identity" $keychain_args --timestamp=none --options runtime "$framework" 2>&1; then
                    print_warning "  签名失败: $(basename "$framework")"
                    sign_failed=true
                fi
            fi
        done
        if [[ "$sign_failed" == false ]]; then
            print_success "  Frameworks 签名完成"
        fi
    fi

    # ============================================================================
    # Step 5: 签名 Qt 插件 (必须在 Extensions 之前)
    # ============================================================================
    if [[ -d "$APP_PATH/Contents/PlugIns" ]]; then
        print_info "签名 Qt 插件..."
        local plugin_count=0
        local plugin_failed=0

        # 使用 for 循环而非 while read 管道，避免子 shell 问题
        while IFS= read -r -d '' dylib; do
            if ! codesign --force --sign "$signing_identity" $keychain_args --timestamp=none --options runtime "$dylib" 2>&1 | grep -v "replacing existing signature"; then
                ((plugin_failed++)) || true
            fi
            ((plugin_count++)) || true
        done < <(find "$APP_PATH/Contents/PlugIns" -name "*.dylib" -print0 2>/dev/null)

        if [[ $plugin_failed -eq 0 ]]; then
            print_success "  Qt 插件签名完成 ($plugin_count 个)"
        else
            print_warning "  Qt 插件签名: $((plugin_count - plugin_failed))/$plugin_count 成功"
        fi
    fi

    # ============================================================================
    # Step 6: 签名 Extensions (使用 entitlements)
    # ============================================================================
    print_info "签名 Extensions..."

    # PacketTunnelProvider - 优先检查 System Extension，然后 App Extension
    local PTP_EXTENSION=""
    if [[ -d "$PTP_SYSEXT" ]]; then
        PTP_EXTENSION="$PTP_SYSEXT"
        print_info "  使用 System Extension 模式"
    elif [[ -d "$PTP_APPEX" ]]; then
        PTP_EXTENSION="$PTP_APPEX"
        print_info "  使用 App Extension 模式"
    fi

    if [[ -n "$PTP_EXTENSION" ]]; then
        # 签名内部 frameworks
        if [[ -d "$PTP_EXTENSION/Contents/Frameworks" ]]; then
            for framework in "$PTP_EXTENSION/Contents/Frameworks"/*.framework "$PTP_EXTENSION/Contents/Frameworks"/*.dylib; do
                if [[ -e "$framework" ]]; then
                    codesign --force --sign "$signing_identity" $keychain_args --timestamp=none --options runtime "$framework" 2>&1 | grep -v "replacing existing signature" || true
                fi
            done
        fi
        # 使用 entitlements 签名 extension
        local PTP_ENTITLEMENTS="$ENTITLEMENTS_DIR/PacketTunnelProvider.entitlements"
        if [[ -f "$PTP_ENTITLEMENTS" ]]; then
            if codesign --force --sign "$signing_identity" $keychain_args --entitlements "$PTP_ENTITLEMENTS" --timestamp=none --options runtime "$PTP_EXTENSION" 2>&1; then
                print_success "  PacketTunnelProvider: 已签名 (with entitlements)"
            else
                print_error "  PacketTunnelProvider: 签名失败"
            fi
        else
            codesign --force --sign "$signing_identity" $keychain_args --timestamp=none --options runtime "$PTP_EXTENSION" 2>&1 || true
        fi
    fi

    # 其他 appex plugins (不包括 PacketTunnelProvider)
    if [[ -d "$APP_PATH/Contents/PlugIns" ]]; then
        for appex in "$APP_PATH/Contents/PlugIns"/*.appex; do
            if [[ -d "$appex" ]]; then
                local appex_name=$(basename "$appex")
                if [[ "$appex_name" != "PacketTunnelProvider.appex" ]]; then
                    codesign --force --sign "$signing_identity" $keychain_args --timestamp=none --options runtime "$appex" 2>&1 | grep -v "replacing existing signature" || true
                fi
            fi
        done
    fi

    # ============================================================================
    # Step 7: 签名主应用 (使用 entitlements)
    # ============================================================================
    print_info "签名主应用..."
    local MAIN_ENTITLEMENTS="$ENTITLEMENTS_DIR/JinGo.entitlements"
    if [[ -f "$MAIN_ENTITLEMENTS" ]]; then
        if codesign --force --sign "$signing_identity" $keychain_args --entitlements "$MAIN_ENTITLEMENTS" --timestamp=none --options runtime "$APP_PATH" 2>&1; then
            print_success "代码签名完成 (with entitlements)"
        else
            print_warning "代码签名失败，尝试不带 entitlements..."
            if ! codesign --force --sign "$signing_identity" $keychain_args --timestamp=none --options runtime --deep "$APP_PATH" 2>&1; then
                print_error "代码签名彻底失败"
            fi
        fi
    else
        if codesign --force --sign "$signing_identity" $keychain_args --timestamp=none --options runtime --deep "$APP_PATH" 2>&1; then
            print_success "代码签名完成"
        else
            print_warning "代码签名失败，但构建继续"
        fi
    fi
}

# ============================================================================
# 验证应用
# ============================================================================
verify_app() {
    print_step "验证应用"

    APP_PATH=$(find "$BUILD_DIR" -name "${APP_NAME}.app" -type d 2>/dev/null | head -n 1)

    if [[ -z "$APP_PATH" ]]; then
        print_error "未找到应用包"
        exit 1
    fi

    # 检查 Info.plist
    if [[ -f "$APP_PATH/Contents/Info.plist" ]]; then
        local bundle_id=$(plutil -extract CFBundleIdentifier raw "$APP_PATH/Contents/Info.plist" 2>/dev/null || echo "Unknown")
        local bundle_version=$(plutil -extract CFBundleShortVersionString raw "$APP_PATH/Contents/Info.plist" 2>/dev/null || echo "Unknown")
        local build_number=$(plutil -extract CFBundleVersion raw "$APP_PATH/Contents/Info.plist" 2>/dev/null || echo "Unknown")

        print_success "Bundle ID: $bundle_id"
        print_success "Version: $bundle_version ($build_number)"
    fi

    # 检查架构
    local archs=$(lipo -archs "$APP_PATH/Contents/MacOS/$APP_NAME" 2>/dev/null || echo "unknown")
    print_success "架构: $archs"

    # 验证签名
    print_info "验证代码签名..."
    if codesign -v "$APP_PATH" 2>/dev/null; then
        print_success "代码签名有效"

        # 显示签名信息
        local signer=$(codesign -dv "$APP_PATH" 2>&1 | grep "Authority=" | head -1 | sed 's/Authority=//')
        if [[ -n "$signer" ]]; then
            print_info "签名者: $signer"
        fi
    else
        print_warning "代码签名无效（本地开发版本）"
    fi

    # 检查 Extensions (System Extensions 和 App Extensions)
    print_info "Extensions:"

    # System Extensions
    local sysext_dir="$APP_PATH/Contents/Library/SystemExtensions"
    if [[ -d "$sysext_dir" ]]; then
        for ext in "$sysext_dir"/*.systemextension; do
            if [[ -d "$ext" ]]; then
                local ext_name=$(basename "$ext" .systemextension)
                if codesign -v "$ext" 2>/dev/null; then
                    print_success "  $ext_name (SystemExt): 已签名"
                else
                    print_warning "  $ext_name (SystemExt): 未签名"
                fi
            fi
        done
    fi

    # App Extensions
    if [[ -d "$APP_PATH/Contents/PlugIns" ]]; then
        for ext in "$APP_PATH/Contents/PlugIns"/*.appex; do
            if [[ -d "$ext" ]]; then
                local ext_name=$(basename "$ext" .appex)
                if codesign -v "$ext" 2>/dev/null; then
                    print_success "  $ext_name: 已签名"
                else
                    print_warning "  $ext_name: 未签名"
                fi
            fi
        done
    fi
}

# ============================================================================
# 创建 DMG 镜像
# ============================================================================
create_dmg() {
    if [[ "$CREATE_DMG" != true ]]; then
        return
    fi

    if [[ "$CONFIGURATION" != "Release" ]]; then
        print_warning "DMG 只在 Release 模式下创建，跳过"
        return
    fi

    print_step "创建 DMG 安装镜像"

    APP_PATH=$(find "$BUILD_DIR" -name "${APP_NAME}.app" -type d 2>/dev/null | head -n 1)

    if [[ -z "$APP_PATH" ]]; then
        print_error "未找到应用包"
        return 1
    fi

    # 获取版本号
    local version=$(plutil -extract CFBundleShortVersionString raw "$APP_PATH/Contents/Info.plist" 2>/dev/null || echo "1.0.0")
    # 使用新命名格式: {brand}-{version}-{date}-{platform}.dmg
    local dmg_name=$(generate_output_name "$version")
    local dmg_path="$BUILD_DIR/${dmg_name}.dmg"
    local dmg_temp="$BUILD_DIR/${dmg_name}-temp.dmg"
    local mount_point="/Volumes/${APP_NAME}"

    # 清理旧文件
    rm -f "$dmg_path" "$dmg_temp"

    # 创建临时 DMG
    print_info "创建 DMG 镜像..."

    local app_size=$(du -sm "$APP_PATH" | awk '{print $1}')
    local dmg_size=$((app_size * 150 / 100))  # 预留 50% 额外空间
    print_info "应用大小: ${app_size}MB, DMG 预留: ${dmg_size}MB"

    hdiutil create -size ${dmg_size}m -fs HFS+ -volname "$APP_NAME" "$dmg_temp" > /dev/null 2>&1

    # 挂载
    hdiutil attach "$dmg_temp" -mountpoint "$mount_point" > /dev/null 2>&1

    # 复制应用 (使用 ditto 保留所有属性和资源分支)
    print_info "复制应用到 DMG..."
    if ! ditto "$APP_PATH" "$mount_point/${APP_NAME}.app"; then
        print_error "复制应用到 DMG 失败"
        hdiutil detach "$mount_point" > /dev/null 2>&1 || true
        rm -f "$dmg_temp"
        return 1
    fi

    # 验证复制结果
    if [[ ! -f "$mount_point/${APP_NAME}.app/Contents/MacOS/${APP_NAME}" ]]; then
        print_error "复制验证失败: 主可执行文件不存在"
        ls -la "$mount_point/${APP_NAME}.app/Contents/MacOS/" 2>/dev/null || true
        hdiutil detach "$mount_point" > /dev/null 2>&1 || true
        rm -f "$dmg_temp"
        return 1
    fi
    print_success "应用复制完成，主可执行文件已验证"

    # 创建 Applications 软链接
    ln -s /Applications "$mount_point/Applications"

    # 设置窗口布局（简单方式）
    osascript << EOF > /dev/null 2>&1 || true
tell application "Finder"
    tell disk "$APP_NAME"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set bounds of container window to {100, 100, 600, 400}
        set viewOptions to icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 128
        close
    end tell
end tell
EOF

    sync

    # 卸载
    hdiutil detach "$mount_point" > /dev/null 2>&1 || true

    # 压缩
    print_info "压缩 DMG..."
    hdiutil convert "$dmg_temp" -format UDZO -o "$dmg_path" > /dev/null 2>&1

    # 清理临时文件
    rm -f "$dmg_temp"

    if [[ -f "$dmg_path" ]]; then
        local dmg_size_mb=$(ls -lh "$dmg_path" | awk '{print $5}')
        print_success "DMG 创建成功: $dmg_path"
        print_info "文件大小: $dmg_size_mb"
    else
        print_error "DMG 创建失败"
    fi
}

# ============================================================================
# 复制到 release 目录
# ============================================================================
copy_to_release() {
    if [[ "$CONFIGURATION" != "Release" ]]; then
        return
    fi

    print_step "复制构建产物到 release 目录"

    # 创建 release 目录
    mkdir -p "$RELEASE_DIR"

    APP_PATH=$(find "$BUILD_DIR" -name "${APP_NAME}.app" -type d 2>/dev/null | head -n 1)

    if [[ -n "$APP_PATH" ]] && [[ -d "$APP_PATH" ]]; then
        # 获取版本号
        local version=$(plutil -extract CFBundleShortVersionString raw "$APP_PATH/Contents/Info.plist" 2>/dev/null || echo "1.0.0")

        # 复制 .app 包 (压缩为 zip) - 使用新命名格式
        local app_zip="$(generate_output_name "$version" "zip")"
        print_info "创建应用压缩包: $app_zip"
        (cd "$(dirname "$APP_PATH")" && zip -r -q "$RELEASE_DIR/$app_zip" "$(basename "$APP_PATH")")

        if [[ -f "$RELEASE_DIR/$app_zip" ]]; then
            print_success "已复制: $RELEASE_DIR/$app_zip"
        fi
    fi

    # 复制 DMG (如果存在)
    local dmg_file=$(find "$BUILD_DIR" -name "*.dmg" -type f 2>/dev/null | head -n 1)
    if [[ -n "$dmg_file" ]] && [[ -f "$dmg_file" ]]; then
        cp "$dmg_file" "$RELEASE_DIR/"
        print_success "已复制: $RELEASE_DIR/$(basename "$dmg_file")"
    fi

    print_success "构建产物已复制到: $RELEASE_DIR"
}

# ============================================================================
# 打开应用
# ============================================================================
open_app() {
    if [[ "$OPEN_APP" == true ]]; then
        APP_PATH=$(find "$BUILD_DIR" -name "${APP_NAME}.app" -type d 2>/dev/null | head -n 1)

        if [[ -n "$APP_PATH" ]]; then
            print_info "启动应用..."
            open "$APP_PATH"
        fi
    fi
}

# ============================================================================
# 显示构建摘要
# ============================================================================
show_summary() {
    APP_PATH=$(find "$BUILD_DIR" -name "${APP_NAME}.app" -type d 2>/dev/null | head -n 1)

    echo ""
    echo -e "${GREEN}${BOLD}=================================================="
    echo "              构建完成！"
    echo "==================================================${NC}"
    echo ""

    if [[ -n "$APP_PATH" ]]; then
        echo -e "${CYAN}应用路径:${NC}"
        echo "  $APP_PATH"
        echo ""
        echo -e "${CYAN}运行应用:${NC}"
        echo "  open \"$APP_PATH\""
        echo ""
        echo -e "${CYAN}Xcode 项目:${NC}"
        echo "  open \"$BUILD_DIR/${APP_NAME}.xcodeproj\""
    fi
    echo ""
}

# ============================================================================
# 主函数
# ============================================================================
main() {
    echo ""
    echo -e "${BOLD}=================================================="
    echo "      JinGoVPN macOS 构建脚本 v1.1.0"
    echo "==================================================${NC}"
    echo ""

    parse_args "$@"

    # 应用白标定制 (如果指定了品牌)
    apply_brand_customization

    # 记录开始时间
    local start_time=$(date +%s)

    print_info "构建配置: $CONFIGURATION"
    if [[ -n "$BRAND_NAME" ]]; then
        print_info "品牌定制: $BRAND_NAME"
    fi
    print_info "时间: $(date '+%Y-%m-%d %H:%M:%S')"
    if [[ "$VERBOSE" == true ]]; then
        print_info "详细模式: 开启"
    fi
    if [[ "$UPDATE_TRANSLATIONS" == true ]]; then
        print_info "翻译更新: 开启"
    fi
    if [[ "$CREATE_DMG" == true ]]; then
        print_info "创建 DMG: 开启"
    fi

    check_requirements
    clean_build_dir
    update_translations
    generate_translations
    generate_xcode_project

    if [[ "$XCODE_ONLY" == true ]]; then
        echo ""
        print_success "Xcode 项目已生成"
        print_info "打开项目: open \"$BUILD_DIR/${APP_NAME}.xcodeproj\""
        exit 0
    fi

    refresh_extension_plist
    build_project

    # 仅在需要签名时执行Extension复制和签名
    if [[ "$SKIP_SIGN" != true ]]; then
        fix_extension_copy
        sign_app
    else
        print_info "跳过签名模式：跳过 Extension 复制和代码签名"
    fi

    verify_app
    create_dmg
    copy_to_release
    open_app
    show_summary

    # 显示总耗时
    local end_time=$(date +%s)
    local total_time=$((end_time - start_time))
    print_info "总耗时: ${total_time} 秒"
}

# ============================================================================
# 执行主函数
# ============================================================================
main "$@"
