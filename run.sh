#!/bin/bash

set -e

PROJECT_NAME="maze"
BUILD_DIR="build"
BIN_DIR="bin"
SRC_DIR="src"
BACKUP_DIR="backup"

# ANSI color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
RESET='\033[0m'

# Function definitions
log_info() {
    echo -e "${GREEN}[INFO]${RESET} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${RESET} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${RESET} $1" >&2
}

# Show help
show_help() {
    echo -e "${BLUE}Usage:${RESET} ./run.sh [dev|release|backup]"
    echo
    echo -e "${YELLOW}Commands:${RESET}"
    echo "  dev     - Build with minimal optimizations for development and run"
    echo "  release - Build with full optimizations and security features and run"
    echo "  backup  - Create a backup of the project"
    echo
}

# Build the project
build_project() {
    local build_type=$1
    local cmake_build_type=""

    if [ "$build_type" == "dev" ]; then
        cmake_build_type="Debug"
        log_info "Building project in development mode"
    elif [ "$build_type" == "release" ]; then
        cmake_build_type="Release"
        log_info "Building project in release mode with optimizations"
    else
        log_error "Unknown build type: $build_type"
        exit 1
    fi

    # Create build directory if it doesn't exist
    mkdir -p $BUILD_DIR

    # Remove existing binary if it exists
    if [ -f "$BIN_DIR/$PROJECT_NAME" ]; then
        log_info "Removing existing binary..."
        rm "$BIN_DIR/$PROJECT_NAME"
    fi

    # Configure with CMake
    log_info "Configuring with CMake..."
    cd $BUILD_DIR
    cmake -DCMAKE_BUILD_TYPE=$cmake_build_type ..

    # Build
    log_info "Building project..."
    cmake --build . -- -j$(nproc)

    # Link to bin directory
    cd ..
    mkdir -p $BIN_DIR

    if [ -f "$BUILD_DIR/bin/$PROJECT_NAME" ]; then
        cp "$BUILD_DIR/bin/$PROJECT_NAME" "$BIN_DIR/"
        log_info "Binary created at $BIN_DIR/$PROJECT_NAME"

        # Additional steps for release builds
        if [ "$build_type" == "release" ]; then
            log_info "Applying additional release optimizations..."
            strip --strip-all "$BIN_DIR/$PROJECT_NAME"

            # Compress binary to reduce size (if upx is installed)
            if command -v upx &> /dev/null; then
                log_info "Compressing binary with UPX..."
                upx --best --lzma "$BIN_DIR/$PROJECT_NAME"
            else
                log_warn "UPX not installed. Binary compression skipped."
                log_warn "Install UPX for better compression: sudo apt install upx"
            fi
        fi

        # Run the program
        log_info "Running the application..."
        "$BIN_DIR/$PROJECT_NAME"
    else
        log_error "Build failed! Binary not found."
        exit 1
    fi
}

# Backup the project
backup_project() {
    log_info "Creating backup..."

    # Create backup directory
    mkdir -p $BACKUP_DIR

    # Get next backup index
    local next_index=1
    if [ -d "$BACKUP_DIR" ]; then
        for dir in $BACKUP_DIR/*; do
            if [ -d "$dir" ]; then
                local dir_name=$(basename "$dir")
                if [[ $dir_name =~ ^[0-9]+$ ]]; then
                    if [ $dir_name -ge $next_index ]; then
                        next_index=$((dir_name + 1))
                    fi
                fi
            fi
        done
    fi

    # Create backup directory with index
    local backup_path="$BACKUP_DIR/$next_index"
    mkdir -p "$backup_path"

    # Copy important files (excluding build artifacts)
    log_info "Copying source files..."
    rsync -av --progress         --exclude="$BUILD_DIR"         --exclude="$BIN_DIR"         --exclude=".git"         --exclude="*.o"         --exclude="*.a"         --exclude="*.so"         --exclude="*.lib"         --exclude="*.dll"         --exclude=".DS_Store"         . "$backup_path/"

    # Create timestamp file
    date > "$backup_path/timestamp.txt"
    echo "$USER" >> "$backup_path/timestamp.txt"

    # Create archive for easy transport
    log_info "Creating compressed archive..."
    tar -czf "$backup_path/${PROJECT_NAME}_backup_$(date +%Y%m%d_%H%M%S).tar.gz" -C "$backup_path" --exclude="*.tar.gz" .

    log_info "Backup completed successfully to $backup_path"
    log_info "Archive created at $backup_path/${PROJECT_NAME}_backup_$(date +%Y%m%d_%H%M%S).tar.gz"
}

# Main execution
if [ "$#" -eq 0 ]; then
    show_help
    exit 0
fi

case "$1" in
    "dev"|"release")
        build_project "$1"
        ;;
    "backup")
        backup_project
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    *)
        log_error "Unknown command: $1"
        show_help
        exit 1
        ;;
esac

exit 0
