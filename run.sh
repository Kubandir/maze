#!/bin/bash

set -e

PROJECT_NAME="mazegen"
BUILD_DIR="build"
SRC_DIR="src"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
RESET='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${RESET} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${RESET} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${RESET} $1" >&2
}

log_step() {
    echo -e "${BLUE}[STEP]${RESET} $1"
}

show_banner() {
    echo -e "${CYAN}"
    echo "███╗   ███╗ █████╗ ███████╗███████╗ ██████╗ ███████╗███╗   ██╗"
    echo "████╗ ████║██╔══██╗╚══███╔╝██╔════╝██╔════╝ ██╔════╝████╗  ██║"
    echo "██╔████╔██║███████║  ███╔╝ █████╗  ██║  ███╗█████╗  ██╔██╗ ██║"
    echo "██║╚██╔╝██║██╔══██║ ███╔╝  ██╔══╝  ██║   ██║██╔══╝  ██║╚██╗██║"
    echo "██║ ╚═╝ ██║██║  ██║███████╗███████╗╚██████╔╝███████╗██║ ╚████║"
    echo "╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝"
    echo -e "${RESET}"
    echo -e "${YELLOW}Installer Script${RESET}"
    echo
}

check_permissions() {
    if [ "$EUID" -ne 0 ]; then
        log_error "This installer requires root privileges."
        echo -e "Please run with ${YELLOW}sudo${RESET}:"
        echo -e "${YELLOW}sudo ./run.sh${RESET}"
        exit 1
    fi
}

detect_os() {
    log_step "Detecting operating system..."

    OS_TYPE="unknown"
    PACKAGE_MANAGER="unknown"
    INSTALL_CMD="unknown"
    BIN_DIR="/usr/local/bin"

    if [ "$(uname)" == "Linux" ]; then
        OS_TYPE="linux"

        if [ -f /etc/os-release ]; then
            source /etc/os-release
            DISTRO=$ID
            log_info "Detected Linux distribution: $DISTRO"

            case $DISTRO in
                ubuntu|debian|linuxmint|pop|elementary|zorin|kali|parrot|deepin)
                    PACKAGE_MANAGER="apt"
                    INSTALL_CMD="apt-get install -y"
                    ;;
                fedora|centos|rhel|redhat|almalinux|rocky)
                    PACKAGE_MANAGER="dnf"
                    INSTALL_CMD="dnf install -y"
                    if ! command -v dnf &>/dev/null && command -v yum &>/dev/null; then
                        PACKAGE_MANAGER="yum"
                        INSTALL_CMD="yum install -y"
                    fi
                    ;;
                arch|manjaro|endeavouros)
                    PACKAGE_MANAGER="pacman"
                    INSTALL_CMD="pacman -S --noconfirm"
                    ;;
                opensuse*|suse|sles)
                    PACKAGE_MANAGER="zypper"
                    INSTALL_CMD="zypper install -y"
                    ;;
                gentoo)
                    PACKAGE_MANAGER="emerge"
                    INSTALL_CMD="emerge -av"
                    ;;
                void)
                    PACKAGE_MANAGER="xbps"
                    INSTALL_CMD="xbps-install -y"
                    ;;
                alpine)
                    PACKAGE_MANAGER="apk"
                    INSTALL_CMD="apk add"
                    ;;
                slackware)
                    PACKAGE_MANAGER="slackpkg"
                    INSTALL_CMD="slackpkg install"
                    ;;
                *)
                    log_warn "Unknown distribution: $DISTRO"
                    log_warn "Installation may fail or be incomplete"
                    ;;
            esac
        else
            log_warn "Could not determine Linux distribution"
            log_warn "Trying to detect package manager..."

            if command -v apt-get &>/dev/null; then
                PACKAGE_MANAGER="apt"
                INSTALL_CMD="apt-get install -y"
            elif command -v dnf &>/dev/null; then
                PACKAGE_MANAGER="dnf"
                INSTALL_CMD="dnf install -y"
            elif command -v yum &>/dev/null; then
                PACKAGE_MANAGER="yum"
                INSTALL_CMD="yum install -y"
            elif command -v pacman &>/dev/null; then
                PACKAGE_MANAGER="pacman"
                INSTALL_CMD="pacman -S --noconfirm"
            elif command -v zypper &>/dev/null; then
                PACKAGE_MANAGER="zypper"
                INSTALL_CMD="zypper install -y"
            elif command -v emerge &>/dev/null; then
                PACKAGE_MANAGER="emerge"
                INSTALL_CMD="emerge -av"
            elif command -v xbps-install &>/dev/null; then
                PACKAGE_MANAGER="xbps"
                INSTALL_CMD="xbps-install -y"
            elif command -v apk &>/dev/null; then
                PACKAGE_MANAGER="apk"
                INSTALL_CMD="apk add"
            else
                log_error "Could not detect package manager"
                log_error "Please install dependencies manually:"
                log_error "  - build-essential/base-devel/build-base (or equivalent)"
                log_error "  - cmake"
                log_error "  - ncurses development libraries"
                log_error "  - upx (optional, for binary compression)"
                exit 1
            fi
        fi
    elif [ "$(uname)" == "Darwin" ]; then
        OS_TYPE="macos"
        PACKAGE_MANAGER="brew"
        INSTALL_CMD="brew install"
        BIN_DIR="/usr/local/bin"
        log_info "Detected macOS"
    else
        log_error "Unsupported operating system: $(uname)"
        exit 1
    fi

    log_info "Using package manager: $PACKAGE_MANAGER"
}

check_dependencies() {
    log_step "Checking and installing dependencies..."

    local BUILD_TOOLS=""
    local NCURSES_DEV=""
    local UPX_PACKAGE="upx"

    case $PACKAGE_MANAGER in
        apt)
            BUILD_TOOLS="build-essential"
            NCURSES_DEV="libncurses5-dev libncursesw5-dev"
            ;;
        dnf|yum)
            BUILD_TOOLS="gcc gcc-c++ make"
            NCURSES_DEV="ncurses-devel"
            ;;
        pacman)
            BUILD_TOOLS="base-devel"
            NCURSES_DEV="ncurses"
            ;;
        zypper)
            BUILD_TOOLS="gcc gcc-c++ make"
            NCURSES_DEV="ncurses-devel"
            ;;
        emerge)
            BUILD_TOOLS="sys-devel/gcc sys-devel/make"
            NCURSES_DEV="sys-libs/ncurses"
            ;;
        xbps)
            BUILD_TOOLS="base-devel"
            NCURSES_DEV="ncurses-devel"
            ;;
        apk)
            BUILD_TOOLS="build-base"
            NCURSES_DEV="ncurses-dev"
            UPX_PACKAGE="upx"
            ;;
        brew)
            BUILD_TOOLS="gcc make"
            NCURSES_DEV="ncurses"
            ;;
        *)
            log_error "Unsupported package manager: $PACKAGE_MANAGER"
            exit 1
            ;;
    esac

    if ! command -v cmake &>/dev/null; then
        log_info "Installing CMake..."
        eval sudo $INSTALL_CMD cmake
    else
        log_info "CMake is already installed"
    fi

    log_info "Installing build tools..."
    if [ "$PACKAGE_MANAGER" == "apt" ]; then
        if ! dpkg -s build-essential &>/dev/null; then
            eval sudo $INSTALL_CMD $BUILD_TOOLS
        else
            log_info "Build tools are already installed"
        fi
    elif [ "$PACKAGE_MANAGER" == "brew" ]; then
        if ! command -v gcc &>/dev/null; then
            eval $INSTALL_CMD $BUILD_TOOLS
        else
            log_info "Build tools are already installed"
        fi
    else
        eval sudo $INSTALL_CMD $BUILD_TOOLS
    fi

    log_info "Installing ncurses development libraries..."
    if [ "$PACKAGE_MANAGER" == "apt" ]; then
        if ! dpkg -s libncurses5-dev &>/dev/null || ! dpkg -s libncursesw5-dev &>/dev/null; then
            eval sudo $INSTALL_CMD $NCURSES_DEV
        else
            log_info "Ncurses libraries are already installed"
        fi
    else
        eval sudo $INSTALL_CMD $NCURSES_DEV
    fi

    if ! command -v upx &>/dev/null; then
        log_info "Installing UPX (optional for binary compression)..."
        if eval sudo $INSTALL_CMD $UPX_PACKAGE; then
            log_info "UPX installed successfully"
        else
            log_warn "Failed to install UPX. Binary compression will be skipped."
            log_warn "This is not critical for the project."
        fi
    else
        log_info "UPX is already installed"
    fi
}

build_project() {
    log_step "Building project in release mode..."

    mkdir -p "$BUILD_DIR"

    pushd "$BUILD_DIR" >/dev/null
    cmake -DCMAKE_BUILD_TYPE=Release ..

    log_info "Compiling with optimizations enabled..."

    if [ "$OS_TYPE" == "linux" ]; then
        CORES=$(nproc 2>/dev/null || grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 1)
    elif [ "$OS_TYPE" == "macos" ]; then
        CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 1)
    else
        CORES=1
    fi

    cmake --build . -- -j$CORES
    popd >/dev/null

    if [ ! -f "$BUILD_DIR/bin/$PROJECT_NAME" ]; then
        log_error "Build failed! Binary not found."
        exit 1
    fi

    log_info "Build completed successfully."
}

optimize_binary() {
    log_step "Optimizing binary size..."

    if [ "$OS_TYPE" == "linux" ]; then
        strip --strip-all "$BUILD_DIR/bin/$PROJECT_NAME"
    elif [ "$OS_TYPE" == "macos" ]; then
        strip "$BUILD_DIR/bin/$PROJECT_NAME"
    fi

    if command -v upx &>/dev/null; then
        log_info "Compressing binary with UPX..."
        upx --best --lzma "$BUILD_DIR/bin/$PROJECT_NAME" || log_warn "UPX compression failed, but continuing..."
    else
        log_warn "UPX not available. Skipping binary compression."
    fi
}

install_binary() {
    log_step "Installing $PROJECT_NAME to system..."

    mkdir -p "$BIN_DIR"

    cp "$BUILD_DIR/bin/$PROJECT_NAME" "$BIN_DIR/"
    chmod 755 "$BIN_DIR/$PROJECT_NAME"

    if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
        log_warn "$BIN_DIR is not in your PATH"
        log_warn "You may need to run: export PATH=\"$PATH:$BIN_DIR\""
    fi

    log_info "Installation completed successfully."
    echo -e "${GREEN}┌─────────────────────────────────────────────────┐${RESET}"
    echo -e "${GREEN}│                                                 │${RESET}"
    echo -e "${GREEN}│  $PROJECT_NAME has been installed successfully!       │${RESET}"
    echo -e "${GREEN}│                                                 │${RESET}"
    echo -e "${GREEN}│  Run it by typing:${RESET} ${CYAN}$PROJECT_NAME${RESET}                      ${GREEN}│${RESET}"
    echo -e "${GREEN}│                                                 │${RESET}"
    echo -e "${GREEN}└─────────────────────────────────────────────────┘${RESET}"
}

show_banner
check_permissions
detect_os
check_dependencies
build_project
optimize_binary
install_binary

exit 0
